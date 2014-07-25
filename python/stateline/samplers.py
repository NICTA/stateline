"""Contains common MCMC samplers."""

import stateline.mcmc as mcmc
import numpy as np
import math


class Sampler(object):
    def next_state(self, i, old_state, proposed_state):
        raise NotImplementedError()

    def next_job(self, i, state):
        raise NotImplementedError()


class RWM(object):
    """A random-walk Metropolis-Hastings sampler.

    This sampler uses a multivariate normal distribution with independent
    dimensions as its proposal distribution.

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

    def next_state(self, _, old_state, proposed_state):
        delta_logl = proposed_state.logl - old_state.logl
        should_accept = np.random.random() < math.exp(delta_logl)
        return proposed_state if should_accept else old_state

    def next_job(self, _, state):
        step = np.random.normal(size=self.step_sizes.shape) * self.step_sizes
        return mcmc.State(state.sample + step)
