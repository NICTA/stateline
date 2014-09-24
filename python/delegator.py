import stateline.mcmc as mcmc
import stateline.logging as logging
import matplotlib.pyplot as pl
import numpy as np
import triangle
import signal
import sys
import time

ctrlc = [False]

def signal_handler(signal, frame):
    ctrlc[0] = True


def main():
    signal.signal(signal.SIGINT, signal_handler)
    logging.initialise(0, True, ".")

    run_time = 60 # time in seconds to run
    nstacks, nchains, ndims = 2, 10, 3
    means = [[-5, -5, -5], [5, 5, 5]]

    worker_interface = mcmc.WorkerInterface(5555, means)

    sigma_adapter = mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, ndims,
                                                   steps_per_adapt=500,
                                                   cold_sigma=10.0)

    beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains,
                                                 steps_per_adapt=500)

    chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                             db_path='testChainDB')

    # Initialise the chains with random samples
    for i, sigma, beta in zip(range(nstacks * nchains),
                              sigma_adapter.sigmas(), beta_adapter.betas()):
        sample = np.random.randn(ndims)
        worker_interface.submit(i, sample)

        _, energy = worker_interface.retrieve()
        chains.initialise(i, sample, energy, sigma, beta)

    # Run the sampler
    sampler = mcmc.Sampler(worker_interface, chains, mcmc.gaussian_proposal, 10)
    logger = mcmc.TableLogger(nstacks, nchains, 500)

    start_time = time.clock()
    while time.clock() - start_time < run_time:
        i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())
        sigma_adapter.update(i, state)
        beta_adapter.update(i, state)
        logger.update(i, state,
                      sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                      beta_adapter.betas(), beta_adapter.swap_rates())
        if ctrlc[0] is True:
            sampler.flush()
            sys.exit()
    
    sampler.flush()  # makes sure all outstanding jobs are finished
    # Visualise the result
    triangle.corner(chains.flat_samples())
    pl.show()


if __name__ == "__main__":
    main()
