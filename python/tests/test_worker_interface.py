import stateline.comms as comms
import stateline.mcmc as mcmc
import stateline.logging as logging
import numpy as np
import pickle


logging.no_logging()


def simple_job_construct_fn(x):
    job_data = comms.JobData(0, "Global Data", x)
    return [job_data]


def simple_energy_result_fn(x):
    return x[0].data
  

def test_simple_job_construct_fn():
    x = np.array([1, 2, 3])
    job = simple_job_construct_fn(x)

    assert len(job) == 1
    assert job[0].job_type == 0
    assert job[0].global_data == "Global Data"
    assert (job[0].job_data == x).all()


def test_simple_energy_result_fn():
    results = [comms.ResultData(0, 666.0)]
    energy = simple_energy_result_fn(results)

    assert energy == 666.0


def test_worker_interface_constructor():
    worker_interface = mcmc.WorkerInterface(5555, "Hello!", {5: "foo"},
                                            simple_job_construct_fn,
                                            simple_energy_result_fn)


def test_worker_interface_submit_retrieve():
    worker_interface = mcmc.WorkerInterface(5555, "Global Spec",
                                            {0: "Job Spec"},
                                            simple_job_construct_fn,
                                            simple_energy_result_fn)

    # Create a worker and check the specs
    worker = comms.Worker("localhost:5555", job_types=[0])

    assert worker.global_spec == "Global Spec"
    assert worker.job_specs[0] == "Job Spec"

    # Submit a batch
    x = np.array([1, 2, 3])
    worker_interface.submit(12, x)

    # Create a minion to receive jobs
    minion = comms.Minion(worker, 0)
    global_data, job_data = minion.next_job()

    assert global_data == "Global Data"
    assert (job_data == x).all()

    # Submit a result
    minion.submit(555.0)

    # Check the result on the server end
    batch_id, result = worker_interface.retrieve()

    assert batch_id == 12
    assert result == 555.0
