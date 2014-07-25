import stateline.comms as comms
import stateline.mcmc as mcmc
import stateline.samplers as samplers
import matplotlib.pyplot as plt
import numpy as np
import scipy.optimize as op
import triangle

def logl(x, x_data, y_data, y_err):
    m, b, lnf = x
    y = m * x_data + b
    inv_sigma2 = 1.0 / (y_err**2 + y**2 * np.exp(2 * lnf))
    return -0.5 * (np.sum((y_data - y)**2 * inv_sigma2 - np.log(inv_sigma2)))

# Choose 'true' parameters
m_true = -0.9594
b_true = 4.294
f_true = 0.534

# Generate noisy data points
x_max = 10
N = 100
x_data = np.sort(x_max * np.random.random(N))
y_err = 0.1 + 0.5 * np.random.rand(N)
y_data = m_true * x_data + b_true
y_data += np.abs(f_true * y_data) * np.random.randn(N)
y_data += y_err * np.random.randn(N)

# Compute the maximum likelihood estimate
nll = lambda *args: -logl(*args)
result = op.minimize(nll, [m_true, b_true, np.log(f_true)], args=(x_data, y_data, y_err))

# Create a delegator for the MCMC
delegator = comms.Delegator(5555, global_spec=(x_data, y_data, y_err))

# Initialise the chains with samples drawn from a Gaussian prior
nstacks, nchains = 1, 5
initial = [result['x'] + 1e-4 * np.random.randn(3) for i in range(nchains)]
mc = mcmc.init(delegator, nstacks, nchains, initial)

# Run MCMC
samples = mcmc.run(mc, samplers.MH(np.ones(3,) * 0.01),
                   mcmc.burnin(100),
                   mcmc.thinning(20),
                   mcmc.until(nsamples=500))

# Corner plot samples
fig = triangle.corner(samples, labels=["m", "b", "\ln\,f"],
                      truths=[m_true, b_true, np.log(f_true)])
plt.show()

# Plot the true line and the noisy observations
fig, ax = plt.subplots()

x = np.linspace(0, x_max, 10)
ax.plot(x, m_true * x + b_true, color='r', alpha=0.8, lw=2)
ax.scatter(x_data, y_data, color='k', s=1)
ax.set_xlim((0, x_max))

# Plot some random samples
for m, b, _ in samples[np.random.randint(len(samples), size=100)]:
    plt.plot(x, m * x + b, color='k', alpha=0.1)

plt.show()