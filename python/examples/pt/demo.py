"""
Sampling from a multimodal Gaussian mixture
===========================================

This demo shows the parallel tempering features of Stateline. Samples are
collected from a multimodal Gaussian mixture at different temperatures. The
plots show that higher temperature chains move freely around the distribution.
Lower temperature chains can escape local minima by swapping with higher
temperature chains.
"""

import stateline.mcmc as mcmc
import stateline.logging as logging
import numpy as np
import matplotlib.pyplot as pl

logging.no_logging()

# ------------------------------------------------------------------------------
# Settings for the demo
# ------------------------------------------------------------------------------
swap_rate = 5  # How often to swap between chains
sigma = 0.1  # The standard deviation of each peak
nstacks, nchains = 1, 8  # Dimensions of the chain array
nsamples = 5000  # Number of samples to collect (excluding burnin)
nburn = 5000  # Number of samples to discard from the beginning

assert nchains % 2 == 0, "Number of chains must be even"

# ------------------------------------------------------------------------------
# Run MCMC at different temperatures
# ------------------------------------------------------------------------------
print('Waiting for workers...')

# Create a worker interface to communicate with workers
workers = mcmc.WorkerInterface(5555, sigma)

# Use a gaussian proposal and sliding window adapter
proposal = mcmc.gaussian_proposal
sigma_adapter = mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, 2,
                                               steps_per_adapt=10)

# Create a beta adapter
beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains,
                                             steps_per_adapt=nsamples + nburn,
                                             beta_factor=1.4)

# Create the chain array
chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                         db_path='testChainDB')

# Initialise the chains with uniform random samples
mcmc.init(chains, 2, workers, sigma_adapter.sigmas(), beta_adapter.betas())

# Create the sampler
sampler = mcmc.Sampler(workers, chains, proposal, swap_interval=swap_rate)

# Create a logger
logger = mcmc.TableLogger(nstacks, nchains, 1000)

# Store the samples at each temperature manually
samples = [[] for _ in range(chains.nchains)]

# Run the sampler until we've collected enough samples
print('Running chain 0 for {} samples...'.format(nsamples + nburn))
while chains.length(0) < nsamples + nburn:
    i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

    sigma_adapter.update(i, state)
    beta_adapter.update(i, state)

    logger.update(i, state,
                  sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                  beta_adapter.betas(), beta_adapter.swap_rates())

    samples[i].append(state.sample)

# Flush any cached samples into the database
sampler.flush()

# ------------------------------------------------------------------------------
# Visualise the path of the MCMC at each temperature
# ------------------------------------------------------------------------------
f, axes = pl.subplots(int(chains.nchains / 2), 2, sharex=True, sharey=True)
for ax, chain, beta in zip(np.ravel(axes), samples, beta_adapter.betas()):
    flat_samples = np.array(chain[nburn:])
    ax.plot(flat_samples[:, 0], flat_samples[:, 1], color='k')
    ax.set_xlim((-2.5, 2.5))
    ax.set_ylim((-2.5, 2.5))
    ax.set_title(r'$\beta = {:.2f}$'.format(beta))

pl.show()
