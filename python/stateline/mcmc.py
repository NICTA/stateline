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
    sample : array-like
        The sample from the target distribution which this state represents.
        This is a 1d-array whose length corresponds to the dimensionality of
        the target distribution.
    energy : float
        The negative log likelihood of this sample.
    sigma : array-like
        The step size vector of the chain when this state was evaluated.
    beta : float
        The inverse temperature of the chain when this state was evaluated.
    accepted : boolean
        If this is True, then this state was an accepted proposed state.
        Otherwise, this state is the same as the previous state in the chain
        because it was rejected.
    swap_type : int
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

    @property
    def sigma(self):
        return super().get_sigma()

    @sigma.setter
    def sigma(self, val):
        super().set_sigma(np.asarray(val, dtype=float))


class WorkerInterface(_sl.WorkerInterface):
    """"""

    def __init__(self, port, global_spec=None, job_specs=None,
                 job_construct_fn=None, result_energy_fn=None):
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
            # TODO better way to do this?
            x = [comms.ResultData(r.job_type, pickle.loads(r.get_data())) for r in results]
            return result_energy_fn(x)

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
        return super().retrieve()


class ChainArray(_sl.ChainArray):
    def __init__(self, nstacks, nchains, recover=False, overwrite=False,
                 db_path="chainDB", cache_length=1000, cache_size=10):
        if overwrite:
            shutil.rmtree(db_path, ignore_errors=True)

        settings = _sl.ChainSettings()
        settings.recover_from_disk = recover
        settings.database_path = db_path
        settings.database_cache_length = cache_length
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

    def initialise(self, i, sample, energy, sigma, beta):
        super().initialise(i,
                           np.asarray(sample, dtype=float), energy,
                           np.asarray(sigma, dtype=float), beta)

    def length(self, i):
        return super().length(i)

    def states(self, i):
        return [State(s.get_sample(), s.energy, s.get_sigma(), s.beta,
                      s.accepted, s.swap_type) for s in super().states(i)]

    def samples(self, i, burnin=0):
        return np.array([s.sample for s in self.states(i)[burnin:]])

    def flat_samples(self, burnin=0):
        return np.vstack(self.samples(i * self.nchains, burnin)
                         for i in range(self.nstacks))

    def sigma(self, i):
        return super().sigma(i)

    def set_sigma(self, i, value):
        super().set_sigma(i, np.asarray(value, dtype=float))

    def beta(self, i):
        return super().beta(i)

    def set_beta(self, i, value):
        super().set_beta(i, value)

    def last_state(self, i):
        state = super().last_state(i)
        return State(state.get_sample(), state.energy, state.get_sigma(),
                     state.beta, state.accepted, state.swap_type)

    def append(self, i, sample, energy):
        super().append(i, np.asarray(sample, dtype=float), energy)


class Sampler(_sl.Sampler):
    """Represents a Markov-Chain Monte Carlo sampler."""

    def __init__(self, worker_interface, chain, prop_fn, swap_interval):
        super().__init__(worker_interface, chain, prop_fn, swap_interval)

    def step(self, sigmas, betas):
        i, state = super().step(sigmas, betas)
        return i, State(state.get_sample(), state.energy, state.get_sigma(),
                        state.beta, state.accepted, state.swap_type)

    def flush(self):
        super().flush()


def gaussian_proposal(i, sample, sigma):
    return _sl.gaussian_proposal(i, sample, sigma)


def gaussian_cov_proposal(i, sample, cov):
    ndim = np.sqrt(cov.shape[0])
    cov = np.reshape(cov, (ndim, ndim))
    #Jf i == 0:
        #Jrint('proposal cov:', cov)
        #print('proposed:', np.random.multivariate_normal(sample, cov))
    return np.random.multivariate_normal(sample, cov)



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


class SigmaCovarianceAdapter:
    def __init__(self, ndims, sigma_adapter):
        self._sigma_adapter = sigma_adapter
        self._lengths = np.zeros(len(sigma_adapter.sigmas()), dtype=int)
        self._covs = [np.eye(ndims) for _ in sigma_adapter.sigmas()]

        self._a = [np.zeros((ndims, ndims)) for _ in sigma_adapter.sigmas()]
        self._u = [np.zeros((ndims, 1)) for _ in sigma_adapter.sigmas()]

    def update(self, i, state):
        self._sigma_adapter.update(i, state)

        n = float(self._lengths[i])
        x = state.sample[np.newaxis].T

        self._a[i] = self._a[i] * (n / (n + 1)) + (x * x.T) / (n + 1)
        self._u[i] = self._u[i] * (n / (n + 1)) + x / (n + 1)

        self._covs[i] = self._a[i] - (self._u[i] * self._u[i].T)# / (n + 1)
        self._lengths[i] += 1

    def sigmas(self):
        return [np.ndarray.flatten(c * s[0])
                for c, s in zip(self._covs, self._sigma_adapter.sigmas())]

    def accept_rates(self):
        return self._sigma_adapter.accept_rates()

    def sample_cov(self, i):
        return self._covs[i]


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
