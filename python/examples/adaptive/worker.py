import stateline.logging as logging
import stateline.comms as comms
import numpy as np
import matplotlib.mlab as mlab


def main():
    logging.no_logging()
    worker = comms.Worker("localhost:5555")

    # Get the parameters of the distribution
    sx, sy = worker.global_spec

    def nll(x, y):
        return -np.log(mlab.bivariate_normal(x, y, sx, sy))

    comms.run_minion(worker, nll, unpack=True)

if __name__ == "__main__":
    main()
