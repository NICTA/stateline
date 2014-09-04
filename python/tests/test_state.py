import stateline.mcmc as mcmc
import numpy as np


def test_state_constructor_sample_only():
    state = mcmc.State([1, 2, 3])
    assert (state.sample == np.array([1, 2, 3])).all()


def test_state_constructor_full():
    state = mcmc.State([1, 2, 3], 10, [4, 5, 6], 1.0, False)
    assert (state.sample == np.array([1, 2, 3])).all()
    assert state.energy == 10
    assert (state.sigma == np.array([4, 5, 6])).all()
    assert state.beta == 1.0
    assert state.accepted is False


def test_state_set_sample():
    state = mcmc.State([1, 2, 3])
    state.sample = [10, 11, 12]
    assert (state.sample == np.array([10, 11, 12])).all()


def test_state_set_sigma():
    state = mcmc.State([1, 2, 3])
    state.sigma = [10, 11, 12]
    assert (state.sigma == np.array([10, 11, 12])).all()
