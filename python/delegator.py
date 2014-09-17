import stateline.mcmc as mcmc
import stateline.logging as logging
import stateline.comms as comms
import numpy as np
import shutil
import sys


def simple_job_construct_fn(x):
    job_data = comms.JobData(0, "Global Data", x)
    return [job_data]


def simple_energy_result_fn(x):
    return x[0].data

def proposal_fn(i, sample, energy):
    return sample


def main():
    logging.initialise(-1, True, ".")
    try:
      shutil.rmtree('testChainDB')
    except:
      pass

    worker_interface = mcmc.WorkerInterface(5555, "Global Spec",
          {0: "Job Spec"},
          simple_job_construct_fn,
          simple_energy_result_fn)
    print("Initialising chain array")
    chain = mcmc.ChainArray(1, 1, recover=False, db_path='testChainDB')
    chain.initialise(0, [1, 2, 3], 1.0, [1], 666.0)
    sigmas = [np.array([1.0])]
    betas = [1.0]
    print("Initialising sampler")
    sampler = mcmc.Sampler(worker_interface, chain, proposal_fn, 10)
    for s in range(200):
        result = sampler.step(sigmas, betas)
        print(result)
    sampler.flush() # makes sure all outstanding jobs are finished


if __name__ == "__main__":
    main()


