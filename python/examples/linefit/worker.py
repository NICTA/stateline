import stateline.logging as logging
import stateline.comms as comms
import numpy as np


def main():
    logging.nolog()
    worker = comms.Worker("localhost:5555")

    # Get the observation data and noise
    x, y, noise = worker.global_spec

    def nll(slope, intercept):
        y_p = slope * x + intercept
        inv_s2 = 1.0 / (noise**2)
        return 0.5 * np.sum((y - y_p)**2 * inv_s2 - np.log(inv_s2))

    comms.run_minion(worker, nll, unpack=True)

if __name__ == "__main__":
    main()
