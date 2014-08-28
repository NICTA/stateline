"""This module contains Markov Chain Monte Carlo (MCMC) simulation code."""


import _stateline as _sl
from . import comms as sl
import numpy as np


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
        self._BASE.setSample(sample)
        self._BASE.energy = energy
        self._BASE.setSigma(sigma)
        self._BASE.energy = energy
        self._BASE.beta = beta
        self._BASE.accepted = accepted

    @property
    def sample(self):
        return self._BASE.getSample(self)

    @sample.setter
    def sample(self, val):
        self._BASE.setSample(self, val)

    @property
    def sigma(self):
        return self._BASE.getSigma(self)

    @sigma.setter
    def sigma(self, val):
        self._BASE.setSigma(self, val)

    @property
    def energy(self):
        return self._BASE.energy

    @property
    def beta(self):
        return self._BASE.beta

    @property
    def accepted(self):
        return self._BASE.accepted


class Sampler(_sl.Sampler):
    """Represents a Markov-Chain Monte Carlo sampler."""

    _BASE = _sl.Sampler


class SlidingWindowSigmaAdapter(_sl.SlidingWindowSigmaAdapter)
    """"""
    _BASE = _sl.SlidingWindowSigmaAdapter

    def __init__(self, chainarray?)
        pass
