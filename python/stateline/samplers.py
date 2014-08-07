"""Contains common MCMC samplers."""

import stateline.mcmc as mcmc
import numpy as np
import math


class Sampler(object):
    """Base sampler class for all MCMC samplers.

    This base class represents an abstract MCMC sampler. In this library, a
    sampler refers to an object which has a method called `step()` which returns
    the next state of a chain. Due to the asynchronous nature of the simulation,
    the states returned by the method can be from any chain.
    """
    def step(self, mc):
        """Return the next state of a chain.

        This is an abstract method that must be implemented by all samplers.

        Parameters
        ----------
        mc : `McmcInstance` instance
            An instance of an MCMC simulation.

        Returns
        -------
        chain, state : (`Chain` instance, `State` instance)
            The first element is the chain that this new state belongs to, the
            second element is the new state itself.
        """
        raise NotImplementedError()


class AcceptReject(Sampler):
    """Base class for all samplers that are based on rejection sampling."""

    def __init__(self, tuner=None):
        self.tuner = tuner

    def _propose(self, chain, cur_state):
        """An abstract method for proposing a new state for a chain.

        This method is called when making a new step in a chain.

        Parameters
        ----------
        chain : `Chain` instance
            The chain that that the proposed state belongs to.
        cur_state : `State` instance
            The current state of the chain.

        Returns
        -------
        sample : array-like
            A 1d-array representing the proposal sample. This is then converted
            into a `State` instance by the `step()` method.
        """
        raise NotImplementedError()

    def _accept(self, chain, cur_state, proposed_state):
        """An abstract method for choosing to accept or reject a proposed state.

        This method is called when making a new step in a chain.

        Parameters
        ----------
        chain : `Chain` instance
            The chain that that the proposed state belongs to.
        cur_state : `State` instance
            The current state of the chain.
        proposed_state : `State` instance
            The proposed state of the chain.

        Returns
        -------
        accept : bool
            If this is `True`, the proposed state is used as the new state of
            the chain. If this is `False`, the current state is used instead.
        """
        raise NotImplementedError()

    def step(self, mc):
        async, req = mc.async, mc.requester  # Shorthand

        proposals = []
        for chain, state in zip(mc.chains, mc.states):
            job = mcmc.State(self._propose(chain, state))
            async.submit(req, chain.index, job.sample)
            proposals.append(job)

        while True:
            i, logl = async.retrieve(req)
            chain = mc.chains[i]

            # Update the log likelihood on the proposed new state
            new_state = proposals[i]
            new_state.logl = logl

            # Accept or reject the proposal
            if not self._accept(chain, mc.states[i], new_state):
                new_state = mc.states[i]

            # Submit the proposal to be evaluated
            job = mcmc.State(self._propose(chain, new_state))
            async.submit(req, i, job.sample)
            proposals[i] = job

            yield chain, new_state


class MH(AcceptReject):
    """A random-walk Metropolis-Hastings sampler.

    This sampler uses an isotropic multivariate normal distribution  as its
    proposal distribution.

    Attributes
    ----------
    step_sizes : array-like
        A 1d-array containing the standard deviations of the proposal
        distribution in each dimension. The length of the array is the
        number of dimensions in the problem.
    """
    def __init__(self, step_sizes):
        """Create a new random-walk Metropolis-Hastings sampler.

        Parameters
        ----------
        step_sizes : array-like
            The standard deviation of the size of each Metropolis step (i.e.
            of the normal distribution).
        """
        self.step_sizes = step_sizes

    def _propose(self, chain, cur_state):
        step = np.random.normal(size=self.step_sizes.shape) * self.step_sizes
        return cur_state.sample + step

    def _accept(self, chain, cur_state, proposed_state):
        delta_logl = proposed_state.logl - cur_state.logl
        return np.log(np.random.random()) < delta_logl
