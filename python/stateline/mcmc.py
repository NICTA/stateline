"""This module contains code for Markov Chain Monte Carlo (MCMC) simulations."""


import _stateline as _sl
import stateline.comms as comms
import numpy as np
import pickle


class State(_sl.State):
    """Represents the state of a Markov chain.

    Attributes
    ----------
    sample : NumPy array
        The sample from the target distribution which this state represents.
        This is a 1d-array whose length corresponds to the dimensionality of
        the target distribution.
    energy : float
        The negative log likelihood of this sample.
    sigma : float, optional
        The step size of the chain when this state was evaluated.
    beta : float
        The inverse temperature of the chain when this state was evaluated.
    accepted : boolean
        If this is True, then this state was an accepted proposed state.
        Otherwise, this state is the same as the previous state in the chain
        because it was rejected.
    """

    def __init__(self, sample, energy, sigma, beta, accepted):
        super().__init__()
        self.sample = sample
        self.energy = energy
        self.sigma = sigma
        self.beta = beta
        self.accepted = accepted

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
    _BASE = _sl.WorkerInterface

    def __init__(self, port, global_spec, job_specs,
                 job_construct_fn, result_energy_fn):
        p_global_spec = pickle.dumps(global_spec)
        p_job_specs = dict((k, pickle.dumps(v)) for k, v in job_specs.items())

        # We need to wrap the result energy function because it is passed a
        # list of raw C++ wrapper of ResultData, instead of the python class.
        def wrapped_result_energy_fn(results):
            # TODO better way to do this?
            x = [comms.ResultData(r.job_type, pickle.loads(r.get_data())) for r in results]
            return result_energy_fn(x)

        self._BASE.__init__(self, p_global_spec, p_job_specs,
                            job_construct_fn, wrapped_result_energy_fn, port)

    def submit(self, i, x):
        self._BASE.submit(self, i, np.asarray(x, dtype=float))

    def retrieve(self):
        return self._BASE.retrieve(self)


class ChainArray(_sl.ChainArray):
    _BASE = _sl.ChainArray

    def __init__(self, nstacks, nchains, recover=False, db_path="chainDB",
                 cache_length=1000, cache_size=10):
        settings = _sl.ChainSettings()
        settings.recover_from_disk = recover
        settings.database_path = db_path
        settings.database_cache_length = cache_length
        settings.database_cache_size_mb = cache_size

        self._BASE.__init__(self, nstacks, nchains, settings)
        self._nstacks = nstacks
        self._nchains = nchains

    @property
    def nstacks(self):
        return self._nstacks

    @property
    def nchains(self):
        return self._nchains

    def initialise(self, i, sample, energy, sigma, beta):
        self._BASE.initialise(self, i,
                              np.asarray(sample, dtype=float), energy,
                              np.asarray(sigma, dtype=float), beta)

    def length(self, i):
        return self._BASE.length(self, i)

    def states(self, i):
        return self._BASE.states(self, i)

    def sigma(self, i):
        print('getting')
        self._BASE.sigma(self, i)
        print('done')

    def set_sigma(self, i, value):
        self._BASE.set_sigma(self, i, np.asarray(value, dtype=float))

    def beta(self, i):
        return self._BASE.beta(self, i)

    def set_beta(self, i, value):
        self._BASE.set_beta(self, i, value)

    def last_state(self, i):
        state = self._BASE.last_state(self, i)
        return State(state.get_sample(), state.energy, state.get_sigma(), state.beta,
                     state.accepted)

    def append(self, i, sample, energy):
        super().append(i, np.asarray(sample, dtype=float), energy)

class Sampler(_sl.Sampler):
    """Represents a Markov-Chain Monte Carlo sampler."""

    def __init__(self, worker_interface, chain, prop_fn, swap_interval):
        super().__init__(worker_interface, chain, prop_fn, swap_interval)

    def step(self, sigmas, betas):
        return super().step(sigmas, betas)

    def flush(self):
        super().flush()


####class SlidingWindowSigmaAdapter(_sl.SlidingWindowSigmaAdapter):
    #""""""
    #_BASE = _sl.SlidingWindowSigmaAdapter
#
    #def __init__(self):
        #pass
#
    #def update(self, i, state):
        ##self._BASE.update(self, i, state)
#
    #def sigmas(self):
        #self._BASE.sigmas(self)
#
    #def accept_rates(self):
        #self._BASE.accept_rates(self)


#class SlidingWindowBetaAdapter(_sl.SlidingWindowBetaAdapter):
#    """"""
#    _BASE = _sl.SlidingWindowBetaAdapter
#
#    def __init__(self):
#        pass
#
#    def update(self, i, state):
#        self._BASE.update(self, i, state)
#
#    def betas(self):
#        return self._BASE.betas(self)
#
#    def swap_rates(self):
#        return self._BASE.swap_rates(self)
#
#
#class EPSRDiagnostic(_sl.EPSRDiagnostic):
#    """"""
#    _BASE = _sl.EPSRDiagnostic
#
#    def __init__(self):
#        pass
#
#    def update(self, i, state):
##        self._BASE.update(self, i, state)
#
#    def r_hat(self):
#        return self._BASE.rHat(self)
#
#    def has_converged(self):
##        return self._BASE.has_converged(self)
