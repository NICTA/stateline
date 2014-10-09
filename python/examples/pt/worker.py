import stateline.logging as logging
import stateline.comms as comms
import numpy as np


def main():
    logging.nolog()
    worker = comms.Worker("localhost:5555")

    # Get the variance of each peak
    s2 = worker.global_spec**2
    inv_s2 = -1.0 / (2 * s2)

    # The coordinates of the peaks
    xs = [0, -1, 1, -1, 1]
    ys = [0, -1, 1, 1, -1]

    def nll(x, y):
        return -np.log(np.sum(np.exp(inv_s2 * ((x - px)**2 + (y - py)**2))
                              for px, py in zip(xs, ys)))

    comms.run_minion(worker, nll, unpack=True)

if __name__ == "__main__":
    main()
