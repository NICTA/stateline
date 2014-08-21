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
        This is a 1d-array whose length corresponds to the dimensionality of the
        target distribution.
    logl : float, optional
        The log likelihood of this sample. Defaults to None.
    beta : float, optional
        The inverse temperature of the chain when this state was evaluated.
        Defaults to None.
    """

    _BASE = _sl.State

    def __init__(self, sample, logl=None, beta=None):
        self.sample = sample
        self.logl = logl
        self.beta = beta


class Sampler(_sl.Sampler):
    _BASE = _sl.Sampler

    def __init__(self):
        pass

    def step(self, sigmas, betas):
        return self._BASE.step(self, sigmas, betas)

