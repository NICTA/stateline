"""
Sampling from a Gaussian mixture
================================

This demo shows how to split the evaluation of a sample into many independent
jobs which can be done in parallel. Stateline is used to sample from a
multivariate Gaussian. To evaluate the energy of a sample, a job is created
for each dimension that computes the likelihood for that particular dimension.
Then, when all the jobs for a particular sample have finished, the results are
combined into a single energy value.

This is particularly useful for likelihoods that can be divided into
independent factors. Each factor can then be evaluated in parallel.
"""

import stateline.mcmc as mcmc
import stateline.comms as comms
import stateline.logging as logging
import numpy as np
import matplotlib.pyplot as pl
from scipy.stats import gaussian_kde
from scipy.misc import logsumexp

logging.no_logging()

# ------------------------------------------------------------------------------
# Settings for the demo
# ------------------------------------------------------------------------------
ncomp = 10  # Number of components in the gaussian mixture
spacing = 3.0  # How far apart components are from each other
sigma = 1.0  # The standard deviation of each component
nstacks, nchains = 2, 2  # Dimensions of the chain array
nsamples = 3000  # Number of samples to collect (excluding burnin)
nburn = 1000  # Number of samples to discard from the beginning

# ------------------------------------------------------------------------------
# Randomly generate a Gaussian mixture
# ------------------------------------------------------------------------------
ndims = 2
means = np.random.randn(ncomp, ndims) * spacing

# ------------------------------------------------------------------------------
# Custom job construct and energy result functions
# ------------------------------------------------------------------------------
def job_construct(x):
    # The job construct function creates a job for each component.
    return [comms.JobData(0, m, x) for m in means]

def result_energy(results):
    # The result energy function adds together the different components.
    return -logsumexp([r.data for r in results])

# ------------------------------------------------------------------------------
# Run MCMC
# ------------------------------------------------------------------------------
print('Waiting for workers...')

# Create a worker interface to communicate with workers
workers = mcmc.WorkerInterface(5555, sigma,
                               job_construct_fn=job_construct,
                               result_energy_fn=result_energy)

# Use a gaussian proposal and sliding window sigma adapter
proposal = mcmc.gaussian_proposal
sigma_adapter = mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, 2,
                                               steps_per_adapt=10)

# Create a beta adapter
beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains,
                                             steps_per_adapt=nsamples + nburn,
                                             beta_factor=1.25)

# Create the chain array
chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                         db_path='testChainDB')

# Initialise the chains with uniform random samples
mcmc.init(chains, 2, workers, sigma_adapter.sigmas(), beta_adapter.betas())

# Create the sampler
sampler = mcmc.Sampler(workers, chains, proposal, swap_interval=10)

# Create a logger
logger = mcmc.TableLogger(nstacks, nchains, 1000)

# Run the sampler until we've collected enough samples
print('Running chain 0 for {} samples...'.format(nsamples + nburn))
while chains.length(0) < nsamples + nburn:
    i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

    sigma_adapter.update(i, state)
    beta_adapter.update(i, state)

    logger.update(i, state,
                  sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                  beta_adapter.betas(), beta_adapter.swap_rates())

# Flush any cached samples into the database
sampler.flush()

# ------------------------------------------------------------------------------
# Visualise the samples as a heatmap
# ------------------------------------------------------------------------------
samples = np.vstack(chains.cold_samples())

# Use a kernel density estimator to get a smoothed plot
kernel = gaussian_kde(samples.T)
x = np.arange(-3 * spacing, 3 * spacing, 0.5)
y = np.arange(-3 * spacing, 3 * spacing, 0.5)
X, Y = np.meshgrid(x, y)
Z = np.reshape(kernel(np.vstack([X.ravel(), Y.ravel()])).T, X.shape)

pl.contourf(X, Y, Z)
pl.autoscale(False)
pl.scatter(means[:, 0], means[:, 1], zorder=1, facecolors='none', edgecolors='k')
pl.show()
