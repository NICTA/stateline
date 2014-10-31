import stateline.logging as logging
import stateline.comms as comms
import numpy as np


def main():
    logging.no_logging()
    worker = comms.Worker("localhost:5555")

    # Get the variance of each of the components
    s2 = worker.global_spec**2
    inv_s2 = -1.0 / (2 * s2)

    def logl(mean, x):
        return inv_s2 * np.sum((x - mean)**2)

    comms.run_minion(worker, logl, use_global_data=True)

if __name__ == "__main__":
    main()
