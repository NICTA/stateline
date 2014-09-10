import stateline.comms as comms
import stateline.mcmc as mcmc
import stateline.logging as logging
import numpy as np

logging.initialise(-3, True, ".")


def test_worker_interface_constructor():
    job_construct_fn = lambda x: x
    result_energy_fn = lambda x: x

    worker_interface = mcmc.WorkerInterface(5555, "Hello!", {5: "foo"},
                                            job_construct_fn,
                                            result_energy_fn)


def test_worker_interface_can_connect_worker():
    job_construct_fn = lambda x: x
    result_energy_fn = lambda x: x

    worker_interface = mcmc.WorkerInterface(5555, "Hello!",
        {0: "foo", 1: "bar", 2: "kitty"},
                                            job_construct_fn,
                                            result_energy_fn)

    worker = comms.Worker("localhost:5555", job_types=[0, 1, 2])
    #worker_interface.submit(0, np.array([1, 2, 3]))

    assert worker.global_spec == 'Hello!'
    assert worker.job_specs[0] == "foo"
    assert worker.job_specs[1] == "bar"
    assert worker.job_specs[2] == "kitty"

