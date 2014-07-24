import stateline.comms as sl
import scipy.stats as stats


# Create a worker
worker = sl.Worker("localhost:5555")

# Set up the distribution to sample from
loc, scale = worker.global_spec()
normal_rv = stats.norm(loc=loc, scale=scale)

# Run threaded minion pool
pool = sl.MinionPool(worker)
pool.run(normal_rv.pdf, nthreads=4)
pool.wait()