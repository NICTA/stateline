"""This module contains Markov Chain Monte Carlo (MCMC) simulation code."""


import _stateline as _sl
from . import comms as sl
import numpy as np
from collections import namedtuple


class AsyncPolicy(object):
    def submit(self, req, batch_id, x):
        raise NotImplementedError()

    def retrieve(self, req):
        raise NotImplementedError()


class SingleTaskAsync(AsyncPolicy):
    def submit(self, req, batch_id, x):
        req.submit(batch_id, [(0, x)])

    def retrieve(self, req):
        i, result = req.retrieve()
        return i, result[0][1]


class State(object):
    """Represents the state of a Markov chain.

    Attributes
    ----------
    sample : NumPy array
        The sample from the target distribution which this state represents.
        This is a 1d-array whose length corresponds to the dimensionality of the
        target distribution.
    logl : float, optional
        The log likelihood of this sample. Defaults to None.
    beta : float, optional
        The inverse temperature of the chain when this state was evaluated.
        Defaults to None.
    """
    def __init__(self, sample, logl=None, beta=None):
        self.sample = sample
        self.logl = logl
        self.beta = beta


class Chain(object):
    """A lightweight representation of an MCMC chain.

    This class only stores some indexing information and useful attributes that
    can be used by filters. The information stored in this class are all
    constants.

    Attributes
    ----------
    index : int
        A unique number identifying this chain. This number is guaranteed to be
        between 0 and (total number of chains - 1) inclusive.
    stack : int
        The index of the stack that this chain is in. This number is between 0
        and (number of stacks - 1) inclusive.
    chain : int
        The index of the chain within the stack that this chain is in. This
        number is between 0 and (number of chains per stack - 1) inclusive.
    is_coldest : bool
        Whether this chain is the coldest chain in its stack.
    is_hottest : bool
        Whether this chain is the hottest chain in its stack.
    """
    def __init__(self, index, stack, chain):
        # Urgh (TODO: will change this into a cython struct for speed)
        self.index = index
        self.stack = stack
        self.chain = chain
        self.is_coldest = False
        self.is_hottest = False

class McmcInstance(object):
    """Represents the state of an MCMC simulation.

    This is used to store the state of an MCMC simulation so that it can be
    saved to disk and resumed later. Do not modify the attributes of an instance
    of this class. This class is mainly for internal use.

    Attributes
    ----------
    requester : `comms.Requester` instance
        The requester used to send jobs to workers.
    async : instance of class derived from `AsyncPolicy`
        The asynchronous policy used to convert a chain state to
    nstacks : int
        The number of parallel tempering stacks (see `init()`).
    nchains : int
        The number of chains each stack.
    states : list of `State` instances
        The current state of each chain.
    """
    def __init__(self, requester, async, nstacks, nchains):
        self.requester = requester
        self.async = async
        self.nstacks = nstacks
        self.nchains = nchains

        self._dbSettings = _sl.DBSettings()
        self._dbSettings.directory = "chainDB"
        self._dbSettings.recover = False
        self._dbSettings.cacheSizeMB = 10.0
        print("Created DBSettings object!")
        self._base = _sl.ChainArray(nstacks,nchains,
                1.1, 0.001, 2.0, self._dbSettings, 1000)
        print("Created chainarray object!")

        # Set default initial states
        self.states = [None] * self.nstacks * self.nchains

        # Set default values for other attributes
        self._attr = np.zeros((2,), dtype=[('lengths', int), ('betas', float)])

    def __getitem__(self, item):
        """Get the value of a particular attribute.

        All attributes are one-dimensional NumPy arrays, with each element
        corresponding to a chain.

        Available attributes include:
            lengths : int
                The length of each chain.
            betas : float
                The inverse temperature of each chain.
        """
        return self._attr[item]


def init(delegator, nstacks, nchains, initial, async=None):
    """Initialise a Parallel Tempering MCMC simulation.

    Parameters
    ----------
    delegator : `comms.Delegator` instance
        Used to communicate with the workers to evaluate log likelihoods.
    nstacks : int
        The number of independent chain stacks to run. A chain stack consists
        of an ordered sequence of Markov chains whose temperatures vary and
        whose states may swap. Essentially, this is an instance of a parallel
        tempering MCMC, independent of the other stacks.
    nchains : int
        The number of chains in each stack.
    initial : iterable
        An iterable containing the initial states for the chains. Each element
        should be a NumPy array containing a sample from the distribution. Which
        chains are given which initial states is currently unspecified (but this
        may change in the future).
    async : instance of a class derived from `AsyncPolicy`, optional
        An asynchronous policy which describes how states should be sent by a
        requester. The default is a `SingleTaskAsync` policy, which simply
        sends the sample vector in its own batch using job type 0.

    Returns
    -------
    mc : `McmcInstance` instance
        The initialised state of an MCMC simulation.
    """
    ntotal = nstacks * nchains

    # Create a requester for the async policy
    delegator.start()
    requester = sl.Requester(delegator)
    if async is None:
        async = SingleTaskAsync()

    # Evaluate initial states for the chain
    for i, state in enumerate(initial):
        async.submit(requester, i, state)

    # Initialise the MCMC instance with chain indexing information
    mc = McmcInstance(requester, async, nstacks, nchains)
    mc.chains, index = [], 0
    for i in range(nstacks):
        for j in range(0, nchains):
            mc.chains.append(Chain(index, i, j))
            index += 1

        # Label with coldest or hottest
        mc.chains[i * nchains].is_coldest = True
        mc.chains[i * nchains + nchains - 1].is_hottest = True

    # Set the initial states
    for _ in range(ntotal):
        i, logl = async.retrieve(requester)
        mc.states[i] = State(initial[i], logl, 1.0)  # TODO: temperature

    mc['lengths'].fill(1)
    mc['betas'].fill(1.0)  # TODO: temperature ladder

    return mc


def _generate_samples(mc, sampler):
    # Generate a infinite stream of samples from the sampler
    for chain, next_state in sampler.step(mc):
        # TODO: perform parallel tempering swaps

        # Update our chain to use the next state given by the sampler
        mc.states[chain.index] = next_state

        yield chain, next_state


def run(mc, sampler, *filters):
    """Resume a Parallel Tempering MCMC simulation.

    Parameters
    ----------
    mc : `McmcState` instance
        The current state of the MCMC.
    sampler : instance of `Sampler` or one of its derived classes.
        The sampler used to generate new samples for the Markov chains.
    *filters
        Any number of filter functions which perform actions on the states of
        the chains. The filters are run in the order specified by the arguments.

        A filter function must have the signature: `f(mc, gen)`, where `mc` is
        an `McmcState`, and `gen` is a generator. The generator will yield a
        pair `(chain, state)` where `state` is the newest state of the Markov
        chain `chain`.
    """
    # Chain together the filters
    gen = _generate_samples(mc, sampler)
    for f in filters:
        gen = f(mc, gen)

    # Generate samples until the last filter runs out.
    # TODO: work out just how to pass around state information for chains
    # TODO: and how to incorporate database persistence. Currently this is just
    # TODO: a quick hack to get things going.
    samples = []
    for chain, state in gen:
        if chain.is_coldest:
            samples.append(state.sample)

    # Discard any outstanding jobs
    for _ in mc.requester.retrieve_all():
        pass

    return np.array(samples)


def burnin(n):
    """Create a burn-in filter which discards the first n samples.

    Parameters
    ----------
    n : int
        The number of samples to discard from each chain in the beginning.
    """
    def filter_func(mc, gen):
        # Keep track of how long each chains has been burning in
        burnin_time = np.zeros((mc.nstacks * mc.nchains,))
        for chain, state in gen:
            if burnin_time[chain.index] == n:
                yield chain, state
            else:
                burnin_time[chain.index] += 1

    return filter_func


def thinning(n):
    """Create a thinning filter which discards n samples for every sample drawn.

    Parameters
    ----------
    n : int
        The number of samples to discard for every sample that is drawn.
    """
    def filter_func(mc, gen):
        # Keep track of how many samples left to discard
        to_discard = np.zeros((mc.nstacks * mc.nchains,))
        for chain, state in gen:
            if to_discard[chain.index] == 0:
                yield chain, state
                to_discard[chain.index] = n  # Need to discard n more samples
            else:
                to_discard[chain.index] -= 1

    return filter_func


def until(nsamples=None):
    """Create a filter which stops the MCMC when a certain condition is met.

    This function takes several different stopping criteria, but only one can
    be used. Chain together multiple `until` filters to have different stopping
    criteria.

    Parameters
    ----------
    nsamples : int, optional
        The number of samples to collect from each of the coldest MCMC chains.
    """
    def filter_length_func(mc, gen):
        # Only need to keep track of the coldest chains
        lengths = np.zeros((mc.nstacks,), dtype=int)
        total_length, max_length = 0, nsamples * mc.nstacks

        for chain, state in gen:
            if chain.is_coldest and lengths[chain.stack] < nsamples:
                lengths[chain.stack] += 1
                total_length += 1

            yield chain, state
            if total_length >= max_length:
                break  # Stop the MCMC

    return filter_length_func
