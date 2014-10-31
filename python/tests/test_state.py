import stateline.mcmc as mcmc
import numpy as np



def test_state_constructor_full():
    state = mcmc.State([1, 2, 3], 10, 1.0, 1.0, False, mcmc.SwapType.ACCEPT)
    assert (state.sample == np.array([1, 2, 3])).all()
    assert state.energy == 10
    assert state.sigma == 1.0
    assert state.beta == 1.0
    assert state.accepted is False
    assert state.swap_type == mcmc.SwapType.ACCEPT


def test_state_set_sample():
    state = mcmc.State([1, 2, 3], 10, 1.0, 1.0, False, mcmc.SwapType.ACCEPT)
    state.sample = [10, 11, 12]
    assert (state.sample == np.array([10, 11, 12])).all()
