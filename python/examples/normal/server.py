import stateline.comms as sl
import stateline.mcmc as mcmc
import stateline.samplers as samplers
import scipy.stats as stats
import matplotlib.pyplot as plt
import numpy as np

# Parameters of the normal distribution
loc, scale = 0.0, 1.0

# Parameters of the MCMC set up
nstacks, nchains = 2, 1

# Create a delegator and send the distribution parameters
delegator = sl.Delegator(5555, global_spec=(loc, scale))

# Initialise the chains with samples drawn from a uniform prior
initial = np.random.random((nstacks * nchains, 1))
mc = mcmc.init(delegator, nstacks, nchains, initial)

# Run MCMC
samples = mcmc.run(mc, samplers.RWM(),
                   mcmc.until(nsamples=1000),
                   mcmc.store_as_list())

# Plot the histogram along with the true distribution
fig, ax = plt.subplots()
ax.hist(samples, 50, normed=True)

x = np.linspace(-3.0, 3.0, 100)
ax.plot(x, stats.norm.pdf(x), alpha=0.5)
