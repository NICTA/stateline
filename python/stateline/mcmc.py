"""This module contains code for Markov Chain Monte Carlo (MCMC) simulations."""


import _stateline as _sl
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

    _BASE = _sl.State

    def __init__(self, sample, energy=None, sigma=None, beta=None,
                 accepted=True):
        self._BASE.__init__(self)
        self._sample = np.asarray(sample, dtype=float)
        self._BASE.set_sample(self, self._sample)

        if sigma is not None:
            self._sigma = np.asarray(sigma, dtype=float)
            self._BASE.set_sigma(self, self._sigma)

        self._BASE.energy = energy
        self._BASE.beta = beta
        self._BASE.accepted = accepted

    @property
    def sample(self):
        return self._sample

    @sample.setter
    def sample(self, val):
        self._sample = np.asarray(val, dtype=float)
        self._BASE.set_sample(self, self._sample)

    @property
    def sigma(self):
        return self._sigma

    @sigma.setter
    def sigma(self, val):
        self._sigma = np.asarray(val, dtype=float)
        self._BASE.set_sample(self, self._sigma)

    @property
    def energy(self):
        return self._BASE.energy

    @property
    def beta(self):
        return self._BASE.beta

    @property
    def accepted(self):
        return self._BASE.accepted


class WorkerInterface(_sl.WorkerInterface):
    """"""
    _BASE = _sl.WorkerInterface

    def __init__(self, port, global_spec, job_specs,
                 job_construct_fn, result_energy_fn):
        p_global_spec = pickle.dumps(global_spec)
        p_job_specs = dict((k, pickle.dumps(v)) for k, v in job_specs.items())
        self._BASE.__init__(self, p_global_spec, p_job_specs,
                            job_construct_fn, result_energy_fn, port)

    def submit(self, i, x):
        self._BASE.submit(self, i, np.asarray(x, dtype=float))

    def retrieve(self):
        return self._BASE.retrieve(self)

#class Sampler(_sl.Sampler):
#    """Represents a Markov-Chain Monte Carlo sampler."""
#
#    _BASE = _sl.Sampler
#

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
