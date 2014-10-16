import stateline.mcmc as mcmc
import numpy as np


def test_sliding_window_sigma_adapter_initial_sigma():
    sigma_adapt = mcmc.SlidingWindowSigmaAdapter(1, 5, 1, cold_sigma=2.0, sigma_factor=1.5)

    expected_sigmas = [
        np.array([2.0]),
        np.array([3.0]),
        np.array([4.5]),
        np.array([6.75]),
        np.array([10.125])
    ]

    assert sigma_adapt.sigmas() == expected_sigmas


def test_sliding_window_sigma_adapter_accept_rates():
    sigma_adapt = mcmc.SlidingWindowSigmaAdapter(1, 1, 1)

    # Update the adapter with some states
    sigma_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.NO_ATTEMPT))
    assert sigma_adapt.accept_rates() == [1.0]

    sigma_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.NO_ATTEMPT))
    assert sigma_adapt.accept_rates() == [1.0]

    sigma_adapt.update(0, mcmc.State([0], 0, 0, 1.0, False, mcmc.SwapType.NO_ATTEMPT))
    assert sigma_adapt.accept_rates() == [0.75]


def test_sliding_window_beta_adapter_initial_beta():
    beta_adapt = mcmc.SlidingWindowBetaAdapter(1, 5, beta_factor=2.0)

    expected_betas = [
        np.array([1.0]),
        np.array([0.5]),
        np.array([0.25]),
        np.array([0.125]),
        np.array([0.0625])
    ]

    assert beta_adapt.betas() == expected_betas


def test_sliding_window_beta_adapter_swap_rates():
    beta_adapt = mcmc.SlidingWindowBetaAdapter(1, 1)

    # Update the adapter with some states
    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.NO_ATTEMPT))
    assert beta_adapt.swap_rates() == [0.0]

    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.REJECT))
    assert beta_adapt.swap_rates() == [0.0]

    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.REJECT))
    assert beta_adapt.swap_rates() == [0.0]

    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, False, mcmc.SwapType.ACCEPT))
    assert beta_adapt.swap_rates() == [0.25]

    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, True, mcmc.SwapType.NO_ATTEMPT))
    assert beta_adapt.swap_rates() == [0.25]

    beta_adapt.update(0, mcmc.State([0], 0, 0, 1.0, False, mcmc.SwapType.ACCEPT))
    assert beta_adapt.swap_rates() == [0.4]
