import stateline.mcmc as mcmc
import numpy as np
import shutil
import threading

def simple_job_construct_fn(x):
    job_data = comms.JobData(0, "Global Data", x)
    return [job_data]


def simple_energy_result_fn(x):
    return x[0].data


def proposal_fn(i, sample, energy):
    return sample

def run_worker():
    worker = comms.Worker("localhost:5555", job_types=[0])

    minion = comms.Minion(worker, 0)
    for global_data, job_data in minion.jobs():
        minion.submit(555.0)

def test_sampler_constructor():
    shutil.rmtree('testChainDB')

    worker_interface = mcmc.WorkerInterface(5555, "Global Spec",
                                            {0: "Job Spec"},
                                            simple_job_construct_fn,
                                            simple_energy_result_fn)

    t = threading.Thread(run_worker)
    t.start()

    chain = mcmc.ChainArray(1, 1, recover=False, db_path='testChainDB')
    chain.initialise(0, [1, 2, 3], 1.0, [1], 666.0)

    sampler = mcmc.Sampler(worker_interface, chain, proposal_fn, 10)
