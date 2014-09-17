import stateline.mcmc as mcmc
import stateline.logging as logging
import stateline.comms as comms
import numpy as np
import sys


def main():
    logging.initialise(-1, True, ".")
    worker = comms.Worker("localhost:5555", job_types=[0])
    minion = comms.Minion(worker, 0)
    while True:
        try:
            global_data, job_data = minion.next_job()
            minion.submit(555.0)
        except:
            print("Exception in run_worker")
            break;
            
if __name__ == "__main__":
    main()


