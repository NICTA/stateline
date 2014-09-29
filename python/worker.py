import stateline.logging as logging
import stateline.comms as comms
import numpy as np


def logpdf(x, mu, cov):
    return 0.5 * np.sum(np.square(x - mu) / cov)


def main():
    logging.initialise(-1, True, ".")
    worker = comms.Worker("localhost:5555")

    # Get the mean and cov of the distribution
    mean, cov = worker.global_spec

    comms.run_minion(worker, lambda x: logpdf(x, mean, cov))

if __name__ == "__main__":
    main()