import stateline.logging as logging
import stateline.comms as comms
import numpy as np


def nlogpdf(x, cov):
    x = x[np.newaxis].T
    return 0.5 * float((x.T.dot(np.linalg.pinv(cov))).dot(x))

def main():
    logging.initialise(-1, True, ".")
    worker = comms.Worker("localhost:5555")

    # Get the cov of the distribution
    cov = worker.global_spec

    comms.run_minion(worker, lambda x: nlogpdf(x, cov))

if __name__ == "__main__":
    main()
