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

    run_time = 60  # time in seconds to run
    nstacks, nchains, ndims = 2, 10, 3
    mean = [0, 0, 0]
    cov = [10.0, 1, 1]

    worker_interface = mcmc.WorkerInterface(5555, (mean, cov))

    if len(sys.argv) == 1:
        proposal = mcmc.gaussian_proposal
        sigma_adapter = mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, ndims,
                                                       steps_per_adapt=500,
                                                       cold_sigma=1.0)
    elif len(sys.argv) == 2 and sys.argv[1] == 'cov':
        proposal = mcmc.gaussian_cov_proposal
        sigma_adapter = mcmc.SigmaCovarianceAdapter(nstacks, nchains, ndims,
                                                    steps_per_adapt=500,
                                                    cold_sigma=1.0)
    elif len(sys.argv) == 2 and sys.argv[1] == 'block':
        pass

    beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains,
                                                 steps_per_adapt=1000)

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
    sampler = mcmc.Sampler(worker_interface, chains, proposal, 10)
    logger = mcmc.TableLogger(nstacks, nchains, 500)
    diagnostic = mcmc.EPSRDiagnostic(nstacks, nchains, ndims, 1.0)

    start_time = time.clock()
    while time.clock() - start_time < run_time:
        i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())
        sigma_adapter.update(i, state)
        beta_adapter.update(i, state)

        logger.update(i, state,
                      sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                      beta_adapter.betas(), beta_adapter.swap_rates())
        diagnostic.update(i, state)

        if ctrlc[0] is True:
            sampler.flush()
            sys.exit()

    sampler.flush()  # makes sure all outstanding jobs are finished

    # Get the samples with burn in
    samples = chains.flat_samples(burnin=1000)

    print('estimated cov: ', sigma_adapter.sample_cov(0))
    print('sample covariance', np.cov(samples.T, bias=1))

    # Visualise the histograms
    triangle.corner(samples)
    pl.show()


if __name__ == "__main__":
    main()
