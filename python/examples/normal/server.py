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

# Create a delegator and send the distribution parameters
delegator = sl.Delegator(5555, global_spec=(loc, scale))

# Initialise the chains with samples drawn from a uniform prior
initial = np.random.random((nstacks * nchains, 1))
mc = mcmc.init(delegator, nstacks, nchains, initial)

# Run MCMC
samples = mcmc.run(mc, samplers.RWM(np.ones(1,)),
                   mcmc.thinning(10),
                   mcmc.until(nsamples=5000))

# Plot the histogram along with the true distribution
fig, (ax1, ax2) = plt.subplots(2)
ax1.hist(samples, 50, normed=True, alpha=0.5)

x = np.linspace(-3.0, 3.0, 100)
ax1.plot(x, stats.norm.pdf(x), color='r')

# Plot a trace to show mixing behaviour
ax2.plot(range(samples.shape[0]), samples)

plt.show()
