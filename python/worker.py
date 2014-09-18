import stateline.logging as logging
import stateline.comms as comms
import numpy as np

def logpdf(x, mu):
    return -0.5 * np.sum(np.square(x - mu))


def main():
    logging.initialise(-1, True, ".")
    worker = comms.Worker("localhost:5555")

    # Get the means of each of the components
    means = worker.global_spec

    # Define the energy function
    def mixture_energy(x):
        return -np.log(sum(np.exp(logpdf(x, m)) for m in means))

    comms.run_minion(worker, mixture_energy)

if __name__ == "__main__":
    main()