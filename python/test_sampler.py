import stateline.mcmc as mcmc
import stateline.logging as logging
import stateline.comms as comms
import numpy as np
import shutil
from multiprocessing import Process
import sys


def simple_job_construct_fn(x):
    job_data = comms.JobData(0, "Global Data", x)
    return [job_data]


def simple_energy_result_fn(x):
    return x[0].data

def proposal_fn(i, sample, energy):
    return sample

def run_worker():
    print("Worker process running, initialising objects")
    worker = comms.Worker("localhost:5555", job_types=[0])
    minion = comms.Minion(worker, 0)
    print("Worker and minion initialised. Waiting for data")
    for global_data, job_data in minion.jobs():
        print("got new job from minion")
        minion.submit(555.0)
    print("Worker finished. Exiting...")
    print("deleting minion..")
    del minion
    print("minion deleted")
    print("deleting worker")
    del worker
    print("worker deleted")
            

def run_sampler():
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
    for s in range(2):
        result = sampler.step(sigmas, betas)
        print(result)
    sampler.flush() # makes sure all outstanding jobs are finished
    print("deleting sampler")
    del sampler
    print("sampler deleted")
    print("deleting chainarray")
    del chain
    print("chainarray deleted")
    print("deleting worker interface")
    del worker_interface
    print("worker interface deleted")

def test_sampler_constructor():
    try:
      shutil.rmtree('testChainDB')
    except:
      pass

    print("Starting worker process")
    p = Process(target=run_worker)
    p.start()
    print("Worker process started")

    run_sampler() 
    
    print("finished... waiting for worker to close")
    p.join()
    print("worker is gone, that should be it")

if __name__ == "__main__":
    logging.initialise(-1, True, ".")
    test_sampler_constructor()


