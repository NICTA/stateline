"""This module contains code for Markov Chain Monte Carlo (MCMC) simulations."""


import _stateline as _sl
import stateline.comms as comms
import numpy as np
import pickle
import shutil


class SwapType(_sl.SwapType):
    pass


class State(_sl.State):
    """Represents the state of a Markov chain.

    Attributes
    ----------
    sample : array-like [n]
        The sample from the target distribution which this state represents.
        This is a 1d-array whose length corresponds to the dimensionality of
        the target distribution.
    energy : float
        The negative log likelihood of this sample.
    sigma : float
        The step size of the chain when this state was evaluated.
    beta : float
        The inverse temperature of the chain when this state was evaluated.
    accepted : boolean
        If this is True, then this state was an accepted proposed state.
        Otherwise, this state is the same as the previous state in the chain
        because it was rejected.
    swap_type : instance of SwapType
        The type of swap performed on this state.
    """

    def __init__(self, sample, energy, sigma, beta, accepted, swap_type):
        super().__init__()
        self.sample = sample
        self.energy = energy
        self.sigma = sigma
        self.beta = beta
        self.accepted = accepted
        self.swap_type = swap_type

    @property
    def sample(self):
        return super().get_sample()

    @sample.setter
    def sample(self, val):
        super().set_sample(np.asarray(val, dtype=float))


class WorkerInterface(_sl.WorkerInterface):
    """Class used to submit MCMC jobs and retrieve results from workers."""

    def __init__(self, port, global_spec=None, job_specs=None,
                 job_construct_fn=None, result_energy_fn=None):
        """Create a new interface to the worker.

        Parameters
        ----------
        port : int
            The port to the run the server on. All workers will connect on this
            port.
        global_spec : object, optional
            A picklable object to send to all workers in the beginning. Defaults
            to None.
        job_specs : dict, optional
            A dictionary of picklable objects to send to workers for a specific
            type of job. Each key is a job type ID and each corresponding object
            is sent to workers with that job type. Defaults to no job specs.
        job_construct_fn : callable, optional
            A function that takes a 1D numpy parameter array and converts it
            into a list of JobData objects representing jobs to be evaluated.
        result_energy_fn : callable, optional
            A function that takes a list of JobResult objects and converts it
            into a scalar, representing the energy of the parameter vector that
            constructed those JobResult objects.
        """
        p_global_spec = pickle.dumps(global_spec)

        if job_specs is None:
            job_specs = {}

        p_job_specs = dict((k, pickle.dumps(v)) for k, v in job_specs.items())

        # Default construct and energy function
        if job_construct_fn is None:
            job_construct_fn = lambda x: [comms.JobData(0, "", x)]

        if result_energy_fn is None:
            result_energy_fn = lambda x: x[0].data

        # We need to wrap the result energy function because it is passed a
        # list of raw C++ wrapper of ResultData, instead of the python class.
        def wrapped_result_energy_fn(results):
            x = [comms.ResultData(r.job_type, pickle.loads(r.get_data())) for r in results]
            try:
                return float(result_energy_fn(x))
            except (TypeError, ValueError):
                raise TypeError('The energy function must return a float')

        super().__init__(p_global_spec, p_job_specs,
                         job_construct_fn, wrapped_result_energy_fn, port)

    def submit(self, i, x):
        """Submit an MCMC sample to be evaluated by workers.

        This method sends the sample to the workers asynchronously.

        Parameters
        ----------
        i : int
            The ID of the chain that contains this sample.
        x : array-like
            The MCMC sample to evaluate.
        """
        super().submit(i, np.asarray(x, dtype=float))

    def retrieve(self):
        """Retrieve the energy of an evaluated sample.

        This method retrieves the energy of sample that has been submitted
        asynchronously and has not been retrieved yet.

        Returns
        -------
        i : int
            The ID of the chain that this sample is from.
        x : float
            The energy of the sample.
        """
        return super().retrieve()


class ChainArray(_sl.ChainArray):
    """Store an MCMC chain persistently.

    Attributes
    ----------
    nstacks : int
        The number of stacks in the chain array.
    nchains : int
        The number of chains per stack.
    ntotal : int
        The total number of chains in all stacks.
    """

    def __init__(self, nstacks, nchains, db_path="chainDB", recover=False,
                 overwrite=False, cache_length=1000, cache_size=10):
        """Create a new chain array.

        Parameters
        ----------
        nstacks : int
            The number of stacks in the chain array.
        nchains : int
            The number of chains per stack.
        db_path : string
            The path to the database used to store the chain array.
        recover : bool, optional
            If set to True, the contents of the chainarray are recovered from
            the database specified by `db_path`. If set to False, a new database
            is created if none exists. Defaults to False.
        overwrite : bool, optional
            If set to True, the file specified by `db_path` is removed. Defaults
            to False.
        cache_length : int, optional
            The maximum number of samples in each chain which are stored in
            memory. If the number of samples exceeds this value, the chain
            is flushed to disk. Defaults to 1000.
        cache_size : int, optional
            The database cache size in MB. Defaults to 10MB.
        """
        if overwrite:
            shutil.rmtree(db_path, ignore_errors=True)

        settings = _sl.ChainSettings()
        settings.recover_from_disk = recover
        settings.database_path = db_path
        settings.chain_cache_length = cache_length
        settings.database_cache_size_mb = cache_size

        super().__init__(nstacks, nchains, settings)

        self._nstacks = nstacks
        self._nchains = nchains

    @property
    def nstacks(self):
        return self._nstacks

    @property
    def nchains(self):
        return self._nchains

    @property
    def ntotal(self):
        return self._nstacks * self._nchains

    def initialise(self, i, sample, energy, sigma, beta):
        """Initialise a chain with initial conditions.

        Parameters
        ----------
        i : int
            The ID of the chain to intialise.
        sample : array-like [n]
            The first sample to be added to the chain.
        energy : float
            The energy of `sample`.
        sigma : float
            The initial step size of the chain.
        beta : float
            The initial inverse temperature of the chain.
        """
        assert 0 <= i < self.ntotal
        super().initialise(i, np.asarray(sample, dtype=float), energy,
                           sigma, beta)

    def length(self, i):
        """Get the length of a chain.

        Parameters
        ----------
        i : int
            The ID of the chain.

        Returns
        -------
        length : int
            The length of the chain.
        """
        assert 0 <= i < self.ntotal
        return super().length(i)

    def states(self, s, burnin=0):
        """Get the all the states of the coldest chain of a stack.

        Parameters
        ----------
        s : int
            The ID of the stack.
        burnin : int, optional
            The number of states to discard from the beginning. Defaults to 0.

        Returns
        -------
        states : list of State objects.
            The states in the chain.
        """
        assert 0 <= s < self.nstacks
        assert burnin >= 0
        return [State(s.get_sample(), s.energy, s.sigma, s.beta,
                      s.accepted, s.swap_type)
                for s in super().states(s * self.nchains)[burnin:]]

    def samples(self, s, nburn=0, nthin=0):
        """Get the all the samples in the coldest chain of a stack.

        Parameters
        ----------
        i : int
            The ID of the chain.
        nburn : int, optional
            The number of samples to discard from the beginning. Defaults to 0.
        nthin : int, optional
            The number of samples to discard for every sample used.
            Defaults to 0.

        Returns
        -------
        samples : 2D numpy array [nsamples x ndims]
            The samples in the chain. Each row is a sample.
        """
        assert 0 <= s < self.nstacks
        assert nburn >= 0
        assert nthin >= 0
        return np.array([s.get_sample()
                         for s in super().states(s * self.nchains, nburn, nthin)])

    def cold_samples(self, nburn=0, nthin=0):
        """Get the samples in the coldest chains of all the stacks.

        Parameters
        ----------
        nburn : int, optional
            The number of samples to discard from the beginning. Defaults to 0.
        nthin : int, optional
            The number of samples to discard for every sample used.
            Defaults to 0.

        Returns
        -------
        samples : iterable of 2D numpy arrays [nsamples x ndims]
            List of the samples in the chains. Each row is a sample.
        """
        assert nburn >= 0
        assert nthin >= 0
        return (self.samples(s, nburn, nthin) for s in range(self.nstacks))

    def sigma(self, i):
        """Get the sigma of a chain."""
        assert 0 <= i < self.ntotal
        return super().sigma(i)

    def set_sigma(self, i, value):
        """Set the sigma of a chain."""
        assert 0 <= i < self.ntotal
        super().set_sigma(i, value)

    def beta(self, i):
        """Get the beta of a chain."""
        assert 0 <= i < self.ntotal
        return super().beta(i)

    def set_beta(self, i, value):
        """Set the beta of a chain."""
        assert 0 <= i < self.ntotal
        super().set_beta(i, value)

    def last_state(self, i):
        """Get the last state of a chain."""
        state = super().last_state(i)
        return State(state.get_sample(), state.energy, state.sigma,
                     state.beta, state.accepted, state.swap_type)

    def append(self, i, sample, energy):
        """Append a new state into a chain."""
        super().append(i, np.asarray(sample, dtype=float), energy)


class Sampler(_sl.Sampler):
    """Represents a Markov-Chain Monte Carlo sampler."""

    def __init__(self, worker_interface, chain, prop_fn, swap_interval):
        super().__init__(worker_interface, chain, prop_fn, swap_interval)

    def step(self, sigmas, betas):
        i, state = super().step(sigmas, betas)
        return i, State(state.get_sample(), state.energy, state.sigma,
                        state.beta, state.accepted, state.swap_type)

    def flush(self):
        super().flush()


def gaussian_proposal(i, sample, sigma):
    return _sl.gaussian_proposal(i, sample, sigma)


class GaussianCovProposal(_sl.GaussianCovProposal):
    def __init__(self, nstacks, nchains, ndims):
        super().__init__(nstacks, nchains, ndims)

    def update(self, i, sample):
        super().update(i, sample)

    def propose(self, i, sample, sigma):
        return super().propose(i, sample, sigma)

    def __call__(self, i, sample, sigma):
        return self.propose(i, sample, sigma)


class SlidingWindowSigmaAdapter(_sl.SlidingWindowSigmaAdapter):
    """"""

    def __init__(self, nstacks, nchains, ndims,
                 window_size=10000, cold_sigma=1.0, sigma_factor=1.5,
                 adapt_length=100000, steps_per_adapt=2500,
                 optimal_accept_rate=0.24, adapt_rate=0.2,
                 min_adapt_factor=0.8, max_adapt_factor=1.25):
        """Initialise a sliding window adapter that adapts the step size.

        Parameters
        ----------
        nstacks : int
        nchains : int
        ndims : int
        window_size : int
            ...
        """
        settings = _sl.SlidingWindowSigmaSettings()
        settings.window_size = window_size
        settings.cold_sigma = cold_sigma
        settings.sigma_factor = sigma_factor
        settings.adaption_length = adapt_length
        settings.nsteps_per_adapt = steps_per_adapt
        settings.optimal_accept_rate = optimal_accept_rate
        settings.adapt_rate = adapt_rate
        settings.min_adapt_factor = min_adapt_factor
        settings.max_adapt_factor = max_adapt_factor

        super().__init__(nstacks, nchains, ndims, settings)

    def update(self, i, state):
        super().update(i, state)

    def sigmas(self):
        return super().sigmas()

    def accept_rates(self):
        return super().accept_rates()


class SlidingWindowBetaAdapter(_sl.SlidingWindowBetaAdapter):
    """"""

    def __init__(self, nstacks, nchains, window_size=10000, beta_factor=1.5,
                 adapt_length=100000, steps_per_adapt=2500,
                 optimal_swap_rate=0.24, adapt_rate=0.2,
                 min_adapt_factor=0.8, max_adapt_factor=1.25):
        settings = _sl.SlidingWindowBetaSettings()
        settings.window_size = window_size
        settings.beta_factor = beta_factor
        settings.adaption_length = adapt_length
        settings.nsteps_per_adapt = steps_per_adapt
        settings.optimal_swap_rate = optimal_swap_rate
        settings.adapt_rate = adapt_rate
        settings.min_adapt_factor = min_adapt_factor
        settings.max_adapt_factor = max_adapt_factor

        super().__init__(nstacks, nchains, settings)

    def update(self, i, state):
        super().update(i, state)

    def betas(self):
        return super().betas()

    def swap_rates(self):
        return super().swap_rates()


class CovarianceEstimator(_sl.CovarianceEstimator):
    """"""

    def __init__(self, nstacks, nchains, ndims):
        super().__init__(nstacks, nchains, ndims)

    def update(self, i, sample):
        super().update(i, sample)

    def cov(self, i):
        return super().cov(i)


class TableLogger(_sl.TableLogger):
    def __init__(self, nstacks, nchains, refresh):
        super().__init__(nstacks, nchains, refresh)

    def update(self, i, state, sigmas, accept_rates, betas, swap_rates):
        super().update(i, state, sigmas, accept_rates, betas, swap_rates)


class EPSRDiagnostic(_sl.EPSRDiagnostic):
    """"""

    def __init__(self, nstacks, nchains, ndims, threshold=1.1):
        super().__init__(nstacks, nchains, ndims, threshold)

    def update(self, i, state):
        super().update(i, state)

    def r_hat(self):
        return super().r_hat()

    def has_converged(self):
        return super().has_converged()


def init(chains, ndims, worker_interface, sigmas, betas, prior=None):
    if prior is None:
        prior = lambda _: np.random.randn(ndims)

    samples = [prior(i) for i in range(chains.nstacks * chains.nchains)]

    for i in range(chains.ntotal):
        worker_interface.submit(i, samples[i])
    
    for _ in range(chains.ntotal):
        i, energy = worker_interface.retrieve()
        chains.initialise(i, samples[i], energy, sigmas[i], betas[i])
