import stateline.comms as sl
import scipy.stats as stats

# Create a worker
worker = sl.Worker("localhost:5555")

# Set up the distribution to sample from
loc, scale = worker.global_spec
normal_rv = stats.norm(loc=loc, scale=scale)

# Run minion to reply with likelihoods
sl.run_minion(worker, normal_rv.logpdf)