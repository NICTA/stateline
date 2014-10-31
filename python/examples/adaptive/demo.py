"""
Sampling using a custom adapter and proposal function
=====================================================

This demo shows how to create a custom sigma adapter and a corresponding
proposal function that samples from a bivariate normal distribution.

The new proposal function proposes in one dimension at a time, one by one, so
the result is a chain that moves in axis-aligned steps. The custom sigma
adapter uses a SlidingWindowSigmaAdapter for each dimension, so each dimension
can have different step sizes.
"""

import stateline.mcmc as mcmc
import stateline.logging as logging
import numpy as np
import matplotlib.pyplot as pl

logging.no_logging()

# ------------------------------------------------------------------------------
# Settings for the demo
# ------------------------------------------------------------------------------
nstacks = 2  # Dimensions of the chain array
nsamples = 5000  # Number of samples to collect
sx, sy = 3.0, 1.0  # The standard deviation in each dimension

# ------------------------------------------------------------------------------
# Custom blockwise sigma adapter
# ------------------------------------------------------------------------------
class BlockSigmaAdapter():
    def __init__(self, nstacks, nchains, ndims, *args, **kwargs):
        # Create a separate adapter for each dimension
        self._adapters = [mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, ndims,
                                                         *args, **kwargs)
                          for _ in range(ndims)]

        self._ndims = ndims
        self._cur_dim = [0 for i in range(nstacks * nchains)]

    def update(self, i, state):
        self._adapters[self._cur_dim[i]].update(i, state)
        self._cur_dim[i] = (self._cur_dim[i] + 1) % self._ndims

    def sigmas(self):
        return [self._adapters[(dim + 1) % self._ndims].sigmas()[i]
                for i, dim in enumerate(self._cur_dim)]
        
# ------------------------------------------------------------------------------
# Custom blockwise proposal function
# ------------------------------------------------------------------------------
class BlockProposal():
    def __init__(self, nstacks, nchains, ndims):
        self._cur_dim = [0 for i in range(nstacks * nchains)]
        self._ndims = ndims

    def update(self, i, state):
        self._cur_dim[i] = (self._cur_dim[i] + 1) % self._ndims
        
    def __call__(self, i, sample, sigma):
        proposal = np.copy(sample)
        proposal[self._cur_dim[i]] += np.random.randn(1) * sigma
        return proposal
        

# ------------------------------------------------------------------------------
# Run MCMC
# ------------------------------------------------------------------------------
ndims = 2  # Sampling from a bivariate normal
nchains = 1  # Disable swaps so that there's only orthogonal moves

print('Waiting for workers...')

# Create a worker interface to communicate with workers
workers = mcmc.WorkerInterface(5555, (sx, sy))

# Use a gaussian proposal and custom block adapter
proposal = BlockProposal(nstacks, nchains, ndims)
sigma_adapter = BlockSigmaAdapter(nstacks, nchains, ndims, steps_per_adapt=10)
beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains)

# Initialise the chains with random samples
chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                         db_path='testChainDB')
mcmc.init(chains, ndims, workers,
          sigma_adapter.sigmas(), beta_adapter.betas())

# Create a sampler
sampler = mcmc.Sampler(workers, chains, proposal, swap_interval=10)

# Run the sampler
print('Running MCMC for {} samples...'.format(nsamples * nstacks))
while chains.length(0) < nsamples:
    i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

    sigma_adapter.update(i, state)
    beta_adapter.update(i, state)

    proposal.update(i, state)

sampler.flush()  # makes sure all outstanding jobs are finished

# ------------------------------------------------------------------------------
# Plot the path of the chains
# ------------------------------------------------------------------------------
for samples in chains.cold_samples():
    pl.plot(samples[:, 0], samples[:, 1], color='k')

pl.axis('equal')
pl.show()
