import stateline.comms as comms
import numpy as np

worker = comms.Worker("localhost:5555")
x_data, y_data, y_err = worker.global_spec


def log_prior(x):
    m, b, lnf = x
    if -5.0 < m < 0.5 and 0.0 < b < 10.0 and -10.0 < lnf < 1.0:
        return 0.0
    return -np.inf


def log_posterior(x):
    m, b, lnf = x
    y = m * x_data + b
    inv_sigma2 = 1.0 / (y_err**2 + y**2 * np.exp(2 * lnf))
    return -0.5 * (np.sum((y_data - y)**2 * inv_sigma2 - np.log(inv_sigma2)))


def log_likelihood(x):
    lp = log_prior(x)
    if not np.isfinite(lp):
        return -np.inf
    return lp + log_posterior(x)

comms.run_minion(worker, log_likelihood)