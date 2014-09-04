import stateline.comms as comms
import stateline.mcmc as mcmc
import numpy as np


def test_worker_interface_constructor():
    job_construct_fn = lambda x: x
    result_energy_fn = lambda x: x

    worker_interface = mcmc.WorkerInterface(5555, "Hello!", {5: "foo"},
                                            job_construct_fn,
                                            result_energy_fn)


def test_worker_interface_can_connect_worker():
    job_construct_fn = lambda x: x
    result_energy_fn = lambda x: x

    worker_interface = mcmc.WorkerInterface(5555, "Hello!", {0: "foo"},
                                            job_construct_fn,
                                            result_energy_fn)

    worker = comms.Worker("localhost:5555")

    assert worker.global_spec == 'Hello!'
    assert worker.job_specs[5] == "foo"
