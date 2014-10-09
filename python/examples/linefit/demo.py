"""
Fitting a straight line
=======================

Some background description...
"""

import stateline.mcmc as mcmc
import stateline.logging as logging
import numpy as np
import matplotlib.pyplot as pl

logging.nolog()

# ------------------------------------------------------------------------------
# Settings for the demo
# ------------------------------------------------------------------------------
slope = -1.5  # Slope of the line
intercept = 4  # y-intercept of the line
xmax = 10  # The maximum x coordinate of the generated data points
N = 30  # Number of data points to generate
noise = 1.0  # Standard deviation of noise
nstacks, nchains = 20, 2  # Dimensions of the chain array
nsamples = 1000  # Number of samples to collect (excluding burnin)
nburn = 1000  # Number of samples to discard from the beginning

# ------------------------------------------------------------------------------
# Generate some synthetic data (true data and noisy observations)
# ------------------------------------------------------------------------------
true_x = np.random.random(N) * xmax
true_y = slope * true_x + intercept
data_x = true_x
data_y = np.random.normal(0, noise, N) + true_y

# ------------------------------------------------------------------------------
# Plot the true line and noisy observations
# ------------------------------------------------------------------------------
xs = np.array([0, xmax])

pl.figure()
pl.plot(xs, slope * xs + intercept, color='k', lw=2, alpha=0.5)
pl.errorbar(data_x, data_y, yerr=noise, fmt='o', color='k')
pl.xlim((0, xmax))
pl.xlabel('$x$')
pl.ylabel('$y$')
pl.show()

# ------------------------------------------------------------------------------
# Run MCMC to find distributions over the parameters
# ------------------------------------------------------------------------------
print('Waiting for workers...')

# Create a worker interface to communicate with workers
workers = mcmc.WorkerInterface(5555, (data_x, data_y, noise))

# Use a covariance gaussian proposal and covariance adapter
proposal = mcmc.gaussian_cov_proposal
sigma_adapter = mcmc.SigmaCovarianceAdapter(nstacks, nchains, 2,
                                            steps_per_adapt=10)

# Create a beta adapter
beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains,
                                             steps_per_adapt=1000)

# Create the chain array
chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                         db_path='testChainDB')

# Initialise the chains with uniform random samples
mcmc.init(chains, 2, workers, sigma_adapter.sigmas(), beta_adapter.betas())

# Create the sampler
sampler = mcmc.Sampler(workers, chains, proposal, swap_interval=10)

# Run the sampler until we've collected enough samples
print('Running for sampler for {} samples...'.format(nsamples * chains.nstacks))
while chains.length(0) < nsamples + nburn:
    i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

    sigma_adapter.update(i, state)
    beta_adapter.update(i, state)

# Flush any cached samples into the database
sampler.flush()

# ------------------------------------------------------------------------------
# Visualise the samples
# ------------------------------------------------------------------------------
samples = list(chains.cold_samples(burnin=nburn))

# Plot trace
f, (ax_m, ax_b) = pl.subplots(2, sharex=True)
for s in samples:
    ax_m.plot(s[:, 0], color='k', alpha=0.4)
    ax_b.plot(s[:, 1], color='k', alpha=0.4)

ax_m.set_ylabel('$m$')
ax_m.axhline(y=slope, color='r')
ax_m.set_xlim(0, nsamples - nburn)
ax_b.set_xlabel('step number')
ax_b.set_ylabel('$b$')
ax_b.axhline(y=intercept, color='r')
ax_b.set_xlim(0, nsamples - nburn)
pl.show()

# Plot histogram
flat_samples = np.vstack(samples)

try:
    import triangle
    triangle.corner(flat_samples, labels=['$m$', '$b$'],
                    truths=[slope, intercept])
    pl.show()
except ImportError:
    pass

# Plot samples as lines
for m, b in flat_samples[np.random.randint(len(flat_samples), size=100)]:
    pl.plot(xs, m * xs + b, color='k', alpha=0.1)

pl.plot(xs, slope * xs + intercept, color='r', lw=2, alpha=0.8)
pl.errorbar(data_x, data_y, yerr=noise, fmt='o', color='k')
pl.xlim((0, xmax))
pl.xlabel('$x$')
pl.ylabel('$y$')
pl.show()
