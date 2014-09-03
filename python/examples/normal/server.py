import stateline.comms as sl
import stateline.mcmc as mcmc
import stateline.samplers as samplers
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np

# Parameters of the normal distribution
loc, scale = 0.0, 1.0

# Parameters of the MCMC set up
nstacks, nchains = 1, 1

# Create a problem instance with the distribution parameters as global spec
problem = sl.ProblemInstance((loc, scale))

# Initialise default MCMC settings
settings = sl.SamplerSettings(nstacks, nchains)

# Initialise the chains with samples drawn from a uniform prior
initial = np.random.random((nstacks * nchains, 1))

# Run MCMC
sampler = sl.Sampler()

for _ in range(2000):
    id, state = sampler.step(sigmas, betas)

samples = sampler.chains.samples(0)

# Plot the histogram along with the true distribution
fig, (ax1, ax2) = plt.subplots(2)
ax1.hist(samples, 50, normed=True, alpha=0.5)

x = np.linspace(-3.0, 3.0, 100)
ax1.plot(x, stats.norm.pdf(x), color='r')

# Plot a trace to show mixing behaviour
ax2.plot(range(samples.shape[0]), samples, color='k')

plt.show()
