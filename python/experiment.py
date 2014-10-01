import stateline.mcmc as mcmc
import stateline.logging as logging
import matplotlib.pyplot as pl
import numpy as np
import triangle
import signal
import sys
import subprocess

ctrlc = [False]

nsteps = 100000
burnin = 100000
nstacks, nchains, ndims = 1, 5, 5
mean = [0, 0, 0, 0, 0]
cov = [10.0, 1, 1, 1, 1]


def signal_handler(signal, frame):
    ctrlc[0] = True


def run_mcmc(proposal, sigma_adapter):
    worker_interface = mcmc.WorkerInterface(5555, (mean, cov))

    # Hack: Use a covariance adapter to find the covariance quickly
    cov_adapter = mcmc.SigmaCovarianceAdapter(nstacks, nchains, ndims)

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
    covs = []

    while chains.length(0) < burnin + nsteps:
        i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

        sigma_adapter.update(i, state)
        beta_adapter.update(i, state)
        cov_adapter.update(i, state)

        logger.update(i, state,
                      sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                      beta_adapter.betas(), beta_adapter.swap_rates())
        diagnostic.update(i, state)

        if ctrlc[0] is True:
            sampler.flush()
            sys.exit()

        if chains.length(0) % 1000 == 0 and i == 0 and chains.length(0) > burnin:
            covs.append(cov_adapter.sample_cov(0)[0, 0])

    sampler.flush()  # makes sure all outstanding jobs are finished
    pl.plot(range(len(covs)), covs)

def main():
    signal.signal(signal.SIGINT, signal_handler)
    logging.initialise(0, True, ".")

    print('Running sliding window adapter...')
    p = subprocess.Popen(['python', 'worker.py'])
    run_mcmc(mcmc.gaussian_proposal,
             mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, ndims,
                                            steps_per_adapt=500,
                                            window_size=1000))

    print('Running covariance adapter...')
    p.wait(); p = subprocess.Popen(['python', 'worker.py'])
    run_mcmc(mcmc.gaussian_proposal,
             mcmc.SigmaCovarianceAdapter(nstacks, nchains, ndims,
                                         steps_per_adapt=500,
                                         window_size=1000))

    print('Running block adapter...')
    p.wait(); p = subprocess.Popen(['python', 'worker.py'])
    run_mcmc(mcmc.gaussian_proposal,
             mcmc.BlockSigmaAdapter(nstacks, nchains, ndims,
                                    range(ndims),
                                    steps_per_adapt=500,
                                    window_size=1000))
    print('Cleaning up...')
    p.wait()

    print('Plotting...')
    pl.show()

if __name__ == "__main__":
    main()
