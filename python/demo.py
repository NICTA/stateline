import stateline.mcmc as mcmc
import stateline.logging as logging
import matplotlib.pyplot as pl
import matplotlib.lines as mlines
import numpy as np
import triangle
import signal
import sys
import subprocess
from collections import deque

def subplane_rotation(dimension, index_1, index_2, theta):
    assert not (index_1 == index_2)
    i1 = min(index_1,index_2)
    i2 = max(index_1, index_2)
    R = np.eye(dimension)
    cos_theta = np.cos(theta)
    sin_theta = np.sin(theta)
    R[i1,i1] = cos_theta
    R[i2,i2] = cos_theta
    R[i1,i2] = -sin_theta
    R[i2,i1] = sin_theta
    return R

def random_nd_rotation(dimension, theta_max):
    R = np.eye(dimension)
    for i in range(dimension):
        for j in range(0, i):
            theta = np.random.random() * theta_max
            R_i = subplane_rotation(dimension, i, j, theta)
            R = np.dot(R_i, R)
    return R

###############################################################################
# Settings for the experiment
###############################################################################

# The chain array set-up
nstacks, nchains = 1, 5

# We want to sample from a high-dimensional normal distribution
ndims = 20

# Generate a stretched covariance matrix
true_cov = np.identity(ndims)
true_cov[0, 0] = 20.0 # Stretch this axis
rotation = random_nd_rotation(ndims, np.pi / 2)
true_cov = np.dot(np.dot(rotation, true_cov), rotation.T)

# Number of samples to collect
nsamples = 10000

# Number of samples to burn-in
nburnin = 30000

# Number of random trials to perform for each adaption method
ntrials = 1

# Number of samples to average for the distance calculation
window_size = 200

###############################################################################
# Running the experiment
###############################################################################
ctrlc = False
def signal_handler(signal, frame):
    ctrlc = True

def run_mcmc(proposal, sigma_adapter, label, colour):
    # Start the worker interface
    worker_interface = mcmc.WorkerInterface(5555, true_cov)

    beta_adapter = mcmc.SlidingWindowBetaAdapter(nstacks, nchains)

    chains = mcmc.ChainArray(nstacks, nchains, recover=False, overwrite=True,
                             db_path='testChainDB')

    # Initialise the chains with random samples
    mcmc.init(chains, ndims, worker_interface,
              sigma_adapter.sigmas(), beta_adapter.betas())

    sampler = mcmc.Sampler(worker_interface, chains, proposal, swap_interval=10)
    logger = mcmc.TableLogger(nstacks, nchains, 1000)

    # Record the distance between each sample
    dists, cur_dist = [], 0
    window = deque()
    prev_sample = chains.last_state(0).sample

    while chains.length(0) < nburnin + nsamples:
        i, state = sampler.step(sigma_adapter.sigmas(), beta_adapter.betas())

        sigma_adapter.update(i, state)
        beta_adapter.update(i, state)

        logger.update(i, state,
                      sigma_adapter.sigmas(), sigma_adapter.accept_rates(),
                      beta_adapter.betas(), beta_adapter.swap_rates())

        if ctrlc is True:
            sampler.flush()
            sys.exit()

        if i == 0 and chains.length(0) > nburnin:
            window.append(np.linalg.norm(state.sample - prev_sample))
            cur_dist += window[-1]
            if len(window) > window_size:
                cur_dist -= window.popleft()
            dists.append(cur_dist * 1.0 / window_size)
            prev_sample = state.sample

    sampler.flush()  # makes sure all outstanding jobs are finished

    # Plot the distance traveled for this run
    pl.plot(dists[window_size:], label=label, c=colour)

    return np.cov(chains.samples(0)[-nburnin:].T, bias=1)


def main():
    signal.signal(signal.SIGINT, signal_handler)
    logging.initialise(0, True, ".")

    for i in range(ntrials):
        print('Running sliding window adapter ({0}/{1})...'.format(i + 1, ntrials))
        p = subprocess.Popen(['python', 'worker.py'])
        cov_scalar = run_mcmc(mcmc.gaussian_proposal,
                              mcmc.SlidingWindowSigmaAdapter(nstacks, nchains, ndims,
                                                             steps_per_adapt=500,
                                                             window_size=1000,
                                                             adapt_rate=0.1,
                                                             cold_sigma=0.53),
                              'scalar', 'blue')

    for i in range(ntrials):
        print('Running covariance adapter ({0}/{1})...'.format(i + 1, ntrials))
        p.wait(); p = subprocess.Popen(['python', 'worker.py'])
        cov_cov = run_mcmc(mcmc.gaussian_cov_proposal,
                           mcmc.SigmaCovarianceAdapter(nstacks, nchains, ndims,
                                                       steps_per_adapt=500,
                                                       window_size=1000,
                                                       adapt_rate=0.1,
                                                       cold_sigma=0.58),
                              'cov', 'red')

    for i in range(ntrials):
        print('Running block adapter ({0}/{1})...'.format(i + 1, ntrials))
        p.wait(); p = subprocess.Popen(['python', 'worker.py'])
        cov_block = run_mcmc(mcmc.gaussian_proposal,
                             mcmc.BlockSigmaAdapter(nstacks, nchains, ndims,
                                                    range(ndims),
                                                    steps_per_adapt=int(500 / ndims),
                                                    window_size=1000,
                                                    adapt_rate=0.1,
                                                    cold_sigma=5.0),
                              'block', 'green')

    print('Cleaning up...')
    p.wait()

    print('Plotting distance...')
    pl.title('Average distance between samples over time using three different adaptive methods')
    pl.legend([mlines.Line2D([], [], color='blue'),
               mlines.Line2D([], [], color='red'),
               mlines.Line2D([], [], color='green')],
               ['scalar', 'cov', 'block'])
    pl.xlabel('Samples')
    pl.ylabel('Average distance over {0} samples'.format(window_size))
    pl.show()

    print('Plotting covariance matrix comparison...')
    f, axes = pl.subplots(2, 2)
    covs = [true_cov, cov_scalar, cov_cov, cov_block]
    titles = ['True covariance', 'Scalar adapter', 'Covariance adapter', 'Block adapter']

    # Use the same colour scale
    vmin = min(np.min(cov) for cov in covs)
    vmax = max(np.max(cov) for cov in covs)

    for ax, cov, title in zip(np.ravel(axes), covs, titles):
        ax.imshow(cov, origin='lower', vmin=vmin, vmax=vmax, interpolation='none')
        ax.set_title(title)

    pl.show()

if __name__ == "__main__":
    main()
