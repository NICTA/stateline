"""This module contains code for Markov Chain Monte Carlo (MCMC) simulations."""


import _stateline as _sl
import stateline.comms as comms
import numpy as np
import pickle
import shutil


class SwapType(_sl.SwapType):
    """Represents a type of swap performed between two chains for a sample.

    Attributes
    ----------
    NoAttempt
        Represents the case where no swap was attempted for this sample.
    Accept
        Represents the case where this sample is the result of a swap.
    Reject
        Represents the case where a swap was rejected for this sample.
    """

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
    """Stores an MCMC chain persistently with transactional guarantees.

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
        """Get the number of stacks."""
        return self._nstacks

    @property
    def nchains(self):
        """Get the number of chains per stack."""
        return self._nchains

    @property
    def ntotal(self):
        """Get the total number of chains in all stacks."""
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

        Raises
        ------
        AssertionError
            If the ID is invalid.
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

        Raises
        ------
        AssertionError
            If the ID is invalid.
        """
        assert 0 <= i < self.ntotal
        return super().length(i)

    def states(self, s, nburn=0, nthin=0):
        """Get the all the states of the coldest chain of a stack.

        Parameters
        ----------
        s : int
            The ID of the stack.
        nburn : int, optional
            The number of states to discard from the beginning. Defaults to 0.
        nthin : int, optional
            The number of state to discard for every state used. Defaults to 0.

        Returns
        -------
        states : list of State objects.
            The states in the chain.

        Raises
        ------
        AssertionError
            If stack ID is invalid, or the amount of burn-in or thinning is
            negative.
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
        s : int
            The ID of the stack.
        nburn : int, optional
            The number of samples to discard from the beginning. Defaults to 0.
        nthin : int, optional
            The number of samples to discard for every sample used.
            Defaults to 0.

        Returns
        -------
        samples : 2D numpy array [nsamples x ndims]
            The samples in the chain. Each row is a sample.

        Raises
        ------
        AssertionError
            If stack ID is invalid, or the amount of burn-in or thinning is
            negative.
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

        Raises
        ------
        AssertionError
            If the amount of burn-in or thinning is negative.
        """
        assert nburn >= 0
        assert nthin >= 0
        return (self.samples(s, nburn, nthin) for s in range(self.nstacks))

    def sigma(self, i):
        """Get the sigma of a chain.
        
        Parameters
        ----------
        i : int
            The ID of the chain.

        Raises
        ------
        AssertionError
            If the ID is invalid.
        """
        assert 0 <= i < self.ntotal
        return super().sigma(i)

    def set_sigma(self, i, value):
        """Set the sigma of a chain.
        
        Parameters
        ----------
        i : int
            The ID of the chain.
        value : float
            The new sigma of this chain.

        Raises
        ------
        AssertionError
            If the ID is invalid.
        """
        assert 0 <= i < self.ntotal
        super().set_sigma(i, value)

    def beta(self, i):
        """Get the beta of a chain.
        
        Parameters
        ----------
        i : int
            The ID of the chain.

        Raises
        ------
        AssertionError
            If the ID is invalid.
        """
        assert 0 <= i < self.ntotal
        return super().beta(i)

    def set_beta(self, i, value):
        """Set the beta of a chain.
        
        Parameters
        ----------
        i : int
            The ID of the chain.
        value : float
            The new beta of this chain.

        Raises
        ------
        AssertionError
            If the ID is invalid.
        """
        assert 0 <= i < self.ntotal
        super().set_beta(i, value)

    def last_state(self, i):
        """Get the last state of a chain.
        
        Parameters
        ----------
        i : int
            The ID of the chain.

        Raises
        ------
        AssertionError
            If ID is invalid or the chain is empty.
        """
        assert 0 <= i < self.ntotal
        assert self.length(i) > 0
        state = super().last_state(i)
        return State(state.get_sample(), state.energy, state.sigma,
                     state.beta, state.accepted, state.swap_type)

    def append(self, i, sample, energy):
        """Append a new state into a chain.

        The sigma and beta of the new state is obtained from `sigma(i)` and
        `beta(i)`, respectively.
        
        Parameters
        ----------
        i : int
            The ID of the chain.
        sample : array-like [n]
            The sample of the new state.
        energy : float
            The energy of the new state.
        """
        super().append(i, np.asarray(sample, dtype=float), energy)


class Sampler(_sl.Sampler):
    """Represents a Markov-Chain Monte Carlo sampler."""

    def __init__(self, worker_interface, chain, prop_fn, swap_interval):
        """Initialise the sampler by proposing a new sample for each chain.

        Parameters
        ----------
        worker_interface : instance of `WorkerInterface`
            The interface to the workers used to evaluate samples.
        chain : instance of `ChainArray`
            The chain array to store samples.
        prop_fn : callable
            A function which takes three arguments (i, sample, sigma) and
            returns a proposed sample for chain `i`, given that the previous
            sample was `sample` and the step size is `sigma`.
        swap_interval : int
            The number of steps between each swap attempt.
        """
        super().__init__(worker_interface, chain, prop_fn, swap_interval)

    def step(self, sigmas, betas):
        """Propose a new state for a chain and return the current state.

        Parameters
        ----------
        sigmas : list of floats
            A list containing the sigma values for each chain.
        betas : list of floats
            A list containing the beta values for each chain.

        Returns
        -------
        i, state : int, instance of `State`
            The ID of the chain and its current state.
        """
        i, state = super().step(sigmas, betas)
        return i, State(state.get_sample(), state.energy, state.sigma,
                        state.beta, state.accepted, state.swap_type)

    def flush(self):
        """Wait for any outstanding states that have not been evaluated yet."""
        super().flush()


def gaussian_proposal(i, sample, sigma):
    """Sample from a spherical gaussian proposal distribution.

    Parameters
    ----------
    i : int
        The ID of the chain.
    sample : array-like [ndims]
        The sample used to generate the next proposal.
    sigma : float
        The step size.

    Returns
    -------
    proposal : array-like [ndims]
        A new sample drawn from a gaussian distribution centered at `sample`
        with standard deviation `sigma`.
    """
    return _sl.gaussian_proposal(i, sample, sigma)


class GaussianCovProposal(_sl.GaussianCovProposal):
    """A multivariate gaussian proposal distribution."""

    def __init__(self, nstacks, nchains, ndims):
        """Initialise the proposal function.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        ndims : int
            The number of dimensions in the problem.
        """
        super().__init__(nstacks, nchains, ndims)

    def update(self, i, cov):
        """Set the covariance matrix of the proposal distribution of a chain.

        Parameters
        ----------
        i : int
            The ID of the chain.
        cov : array-like [ndims x ndims]
            The new covariance matrix.
        """
        super().update(i, cov)

    def __call__(self, i, sample, sigma):
        """Sample from the proposal distribution.

        Parameters
        ----------
        i : int
            The ID of the chain.
        sample : array-like [ndims]
            The sample used to generate the next proposal.
        sigma : float
            The step size.

        Returns
        -------
        proposal : array-like [ndims]
            A new sample drawn from a gaussian distribution centered at `sample`
            with the covariance matrix of the chain.
        """
        return super().propose(i, sample, sigma)


class SlidingWindowSigmaAdapter(_sl.SlidingWindowSigmaAdapter):
    """Adapts sigma values according to the acceptance rate."""

    def __init__(self, nstacks, nchains, ndims,
                 window_size=10000, cold_sigma=1.0, sigma_factor=1.5,
                 adapt_length=100000, steps_per_adapt=2500,
                 optimal_accept_rate=0.24, adapt_rate=0.2,
                 min_adapt_factor=0.8, max_adapt_factor=1.25):
        """Initialise a sliding window adapter that adapts the step size.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        ndims : int
            The number of dimensions in the problem.
        window_size : int, optional
            The number of states in the sliding window used to calculate the
            acceptance rate. Defaults to 10000.
        cold_sigma : float, optional
            The value of sigma for the coldest (beta=1) chain. Defaults to 1.0.
        sigma_factor : float, optional
            The geometric factor used to calculate the sigma for higher
            temperature chains in a stack. The sigma increase exponentially
            within a stack. Defaults to 1.5.
        adaption_length : int, optional
            How many samples to collect until the effect of adaption becomes
            miniscule. Defaults to 100000.
        steps_per_adapt : int, optional
            The number of steps between each change in sigma due to adaption.
            Defaults to 2500.
        optimal_accept_rate : float, optional
            The accept rate that the adapter is aiming for. Defaults to 0.24.
        adapt_rate : float, optional
            The multiplicative rate at which sigma changes.
        min_adapt_factor: float, optional
            The minimum multiplicative factor by which sigma can change in a
            single adaption. Defaults to 0.8.
        max_adapt_factor: float, optional
            The maximum multiplicative factor by which sigma can change in a
            single adaption. Defaults to 1.25.
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
        """Update the adapter with a new state.

        Parameters
        ----------
        i : int
            The ID of the chain.
        state : instance of `State`
            The new state in the chain.
        """
        super().update(i, state)

    def sigmas(self):
        """Get the current sigma values computed by the adapter.

        Returns
        -------
        sigmas : list of floats
            A list containing the sigma value for each chain.
        """
        return super().sigmas()

    def accept_rates(self):
        """Get the acceptance rates computed by the adapter.

        Returns
        -------
        accept_rates : list of floats
            A list containing the accept rates for each chain.
        """
        return super().accept_rates()


class SlidingWindowBetaAdapter(_sl.SlidingWindowBetaAdapter):
    """Adapts temperature values according to the swap rate."""

    def __init__(self, nstacks, nchains, window_size=10000, beta_factor=1.5,
                 adapt_length=100000, steps_per_adapt=2500,
                 optimal_swap_rate=0.24, adapt_rate=0.2,
                 min_adapt_factor=0.8, max_adapt_factor=1.25):
        """Initialise a sliding window adapter that adapts the temperature.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        ndims : int
            The number of dimensions in the problem.
        window_size : int, optional
            The number of states in the sliding window used to calculate the
            swap rate. Defaults to 10000.
        beta_factor : float, optional
            The geometric factor used to calculate the temperature for higher
            temperature chains in a stack. Beta decreases exponentially
            within a stack. Defaults to 1.5.
        adaption_length : int, optional
            How many samples to collect until the effect of adaption becomes
            miniscule. Defaults to 100000.
        steps_per_adapt : int, optional
            The number of steps between each change in beta due to adaption.
            Defaults to 2500.
        optimal_swap_rate : float, optional
            The swap rate that the adapter is aiming for. Defaults to 0.24.
        adapt_rate : float, optional
            The multiplicative rate at which beta changes.
        min_adapt_factor: float, optional
            The minimum multiplicative factor by which beta can change in a
            single adaption. Defaults to 0.8.
        max_adapt_factor: float, optional
            The maximum multiplicative factor by which beta can change in a
            single adaption. Defaults to 1.25.
        """
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
        """Update the adapter with a new state.

        Parameters
        ----------
        i : int
            The ID of the chain.
        state : instance of `State`
            The new state in the chain.
        """
        super().update(i, state)

    def betas(self):
        """Get the current beta values computed by the adapter.

        Returns
        -------
        betas : list of floats
            A list containing the beta value for each chain.
        """
        return super().betas()

    def swap_rates(self):
        """Get the swap rates computed by the adapter.

        Returns
        -------
        swap_rates : list of floats
            A list containing the swap rates for each chain.
        """
        return super().swap_rates()


class CovarianceEstimator(_sl.CovarianceEstimator):
    """Estimates the sample covariance of a set of chains."""

    def __init__(self, nstacks, nchains, ndims):
        """Initialise the covariance estimator.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        ndims : int
            The number of dimensions in the problem.
        """
        super().__init__(nstacks, nchains, ndims)

    def update(self, i, sample):
        """Update the estimator with a new sample from a chain.

        Parameters
        ----------
        i : int
            The ID of the chain.
        sample : array-like [ndims]
            The new sample in the chain.
        """
        super().update(i, sample)

    def cov(self, i):
        """Get the biased sample covariance of a chain.

        Parameters
        ----------
        i : int
            The ID of the chain.
        """
        return super().cov(i)


class TableLogger(_sl.TableLogger):
    """A logger that periodicaly outputs chain statistics in a table format."""

    def __init__(self, nstacks, nchains, refresh):
        """Initialise the table logger.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        refresh : int
            The number of milliseconds between each output.
        """
        super().__init__(nstacks, nchains, refresh)

    def update(self, i, state, sigmas, accept_rates, betas, swap_rates):
        """Update the logger with new information.

        Parameters
        ----------
        i : int
            The ID of the chain.
        state : instance of `State`
            The new state of the chain.
        sigmas : list of floats
            A list containing the sigma of each chain.
        accept_rates : list of floats
            A list containing the acceptance rate of each chain.
        betas : list of floats
            A list containing the beta of each chain.
        swap_rates : list of floats
            A list containing the swap rate of each chain.
        """
        super().update(i, state, sigmas, accept_rates, betas, swap_rates)


class EPSRDiagnostic(_sl.EPSRDiagnostic):
    """Convergence test using Estimated Potential Scale Reduction (EPSR)."""

    def __init__(self, nstacks, nchains, ndims, threshold=1.1):
        """Initialise the convergence test.

        Parameters
        ----------
        nstacks : int
            The number of stacks.
        nchains : int
            The number of chains per stack.
        ndims : int
            The number of dimensions in the problem.
        threshold : float, optional
            The threshold value for convergence. All chains are considered to
            have converged if the estimated potential scale factor falls below
            this threshold value. Defaults to 1.1.
        """
        super().__init__(nstacks, nchains, ndims, threshold)

    def update(self, i, state):
        """Update the convergence test with a new state.

        Parameters
        ----------
        i : int
            The ID of the chain.
        state : instance of `State`
            The new state in the chain.
        """
        super().update(i, state)

    def r_hat(self):
        """Get the estimated potential scale factor.

        A low value indicates that the chains are converging.
        """
        return super().r_hat()

    def has_converged(self):
        """Check if the estimated potential scale factor is below threshold."""
        return super().has_converged()


def init(chains, ndims, worker_interface, sigmas, betas, prior=None):
    """Initialise a chain array with initial samples drawn from a prior.

    Parameters
    ----------
    chains : instance of `ChainArray`
        The chain array to initialise.
    ndims : int
        The number of dimensions in the problem.
    worker_interface : instance of `WorkerInterface`
        The interface used to communicate to workers.
    sigmas : list of floats
        A list containing the sigma values for each chain.
    betas : list of floats
        A list containing the beta values for each chain.
    prior : callable, optional
        A function which takes a single argument `i` and returns the initial
        sample for chain `i`. Defaults to a function that draws samples from
        the standard normal.
    """
    if prior is None:
        prior = lambda _: np.random.randn(ndims)

    samples = [prior(i) for i in range(chains.nstacks * chains.nchains)]

    for i in range(chains.ntotal):
        worker_interface.submit(i, samples[i])
    
    for _ in range(chains.ntotal):
        i, energy = worker_interface.retrieve()
        chains.initialise(i, samples[i], energy, sigmas[i], betas[i])
