"""This module contains communications and networking wrappers."""

import _stateline as _sl
import pickle


class JobData(_sl.JobData):
    def __init__(self, job_type, global_data, job_data):
        super().__init__(job_type, pickle.dumps(global_data),
                         pickle.dumps(job_data))
        self._job_type = job_type

    @property
    def job_type(self):
        return self._job_type

    @property
    def global_data(self):
        return pickle.loads(self._BASE.get_global_data(self))

    @property
    def job_data(self):
        return pickle.loads(self._BASE.get_job_data(self))


class ResultData(_sl.ResultData):
    _BASE = _sl.ResultData

    def __init__(self, job_type, data):
        self._BASE.__init__(self, job_type, pickle.dumps(data))
        self._job_type = job_type

    @property
    def job_type(self):
        return self._BASE.job_type

    @property
    def data(self):
        return pickle.loads(self._BASE.get_data(self))


class Worker(_sl.Worker):
    """Class used to store distribute jobs to `Minion` instances.

    A worker acts as an interface between the delegator (who sends jobs) and
    minions (who do jobs). A worker can have multiple `Minion` instances
    connecting to it. Each minion can only accept one job type from the worker.

    Notes
    -----
    Only one instance of this class should exist, but there can be multiple
    `Minion` instances that connect to a worker.

    Examples
    --------
    Create a worker with default job type.
    >>> worker = Worker("tcp://localhost:5555")
    >>> worker.global_spec()
    "Global Spec"
    >>> worker.job_specs()
    {0: "Spec for job 0"}

    Create a worker with multiple job types and missing specs.
    >>> worker = Worker("tcp://localhost:5555", job_types=[0, 1, 2])
    >>> worker.global_spec()
    "Global Spec"
    >>> worker.job_specs() # Assuming delegator has no spec for job type 1
    {0: "Spec for job 0", 1: None, 2: "Spec for job 2"}
    """

    _BASE = _sl.Worker

    def __init__(self, addr, job_types=None, poll_rate=100):
        """Create a new worker.

        This call will block until the worker has connected to the delegator.

        Parameters
        ----------
        addr : string
            The address of the delegator to connect to.
        job_types : list, optional
            A list of integers containing the job types that this worker can do.
            By default, the worker will only accept job type 0.
        poll_rate : int, optional
            The number of milliseconds before polls for delegator messages.
            Defaults to 100ms.
        """
        # Create settings structs for the super constructor
        hb_s = _sl.HeartbeatSettings()
        hb_s.rate, hb_s.poll_rate, hb_s.timeout = 1000, 500, 3000

        wkr_s = _sl.WorkerSettings()
        wkr_s.address, wkr_s.poll_rate, wkr_s.heartbeat = addr, poll_rate, hb_s

        if job_types is None:
            # Default to accept only job type 0
            job_types = [0]

        self._BASE.__init__(self, job_types, wkr_s)

        # Set python friendly properties
        self._addr = addr
        self._poll_rate = poll_rate
        self._job_types = job_types
        self._global_spec = pickle.loads(self._BASE.global_spec(self))
        self._job_specs = dict()
        for job_type in job_types:
            spec = self._BASE.job_spec(self, job_type)
            if len(spec) == 0:
                # Empty spec means that the delegator did not have a spec for
                # this job type
                self._job_specs[job_type] = None
            else:
                self._job_specs[job_type] = pickle.loads(spec)

    @property
    def addr(self):
        """Get the address of the delegator that this worker is connected to."""
        return self._addr

    @property
    def poll_rate(self):
        """Get the rate at which the worker is polling in milliseconds."""
        return self._poll_rate

    @property
    def job_types(self):
        """Get the job types that this worker is accepting."""
        return self._job_types

    @property
    def global_spec(self):
        """Get the global specification object sent from the delegator.

        Returns
        -------
        global_spec : object
            The global specification sent from the delegator, or `None` if the
            delegator did not send any.

        See Also
        --------
        Delegator.__init__
        """
        return self._global_spec

    @property
    def job_specs(self):
        """Get the job specification objects sent from the delegator.

        Returns
        -------
        job_spec : dict
            The job specs for each job type in a dictionary, or `None` if the
            delegator did not send any.

        See Also
        --------
        Delegator.__init__
        """
        return self._job_specs


class Minion(_sl.Minion):
    """Class used to submit job results.

    Notes
    -----
    There can be multiple `Minion` instances that connect to a worker.

    Examples
    --------
    Create a minion which handles jobs that involve finding the sum of a list.
    >>> worker = Worker("tcp://localhost:5555")
    >>> minion = Minion(worker)
    >>> minion.job_type()
    0
    >>> global_data, job_data = minion.next_job()
    >>> global_data
    "Please sum this list for me"
    >>> job_data
    [1, 1, 2, 3, 5, 8, 13]
    >>> minion.submit(sum(job_data))

    Create minions with custom job types.
    >>> worker = Worker("tcp://localhost:5555", job_types=[1, 3])
    >>> minion1 = Minion(worker, 1)
    >>> minion3 = Minion(worker, 3)
    """

    _BASE = _sl.Minion

    def __init__(self, worker, job_type=0):
        """Create a new minion.

        Parameters
        ----------
        worker : `Worker` instance
            An instance of `Worker` to connect to.
        job_type : int, optional
            The type of job that this minion can submit results for. By default,
            the minion can only submit results for job type 0.
        """
        self._BASE.__init__(self, worker, job_type)
        self._worker = worker
        self._job_type = job_type
        self._got_job = False

    @property
    def worker(self):
        """Get the worker that this minion is connected to."""
        return self._worker

    @property
    def job_type(self):
        """Get the type of job that this minion can submit results for."""
        return self._job_type

    def next_job(self):
        """Get the next job for this minion to do.

        Returns
        -------
        job : (object, object)
            A pair containing the global and job-specific data sent by the
            delegator respectively. The global data object may be shared with
            other minions doing different job types. The job-specific data is
            unique to this minion. The global data may be `None` if it was not
            specified by the delegator.
        """
        if self._got_job is True:
            raise RuntimeError('Minion must submit result before asking for a'
                               'new job.')

        raw = self._BASE.next_job(self)
        self._got_job = True

        # Unpack the raw JobData struct (we can ignore job type because minions
        # can only do one type of job).
        return (pickle.loads(raw.get_global_data()),
                pickle.loads(raw.get_job_data()))

    def jobs(self):
        """A generator that yields jobs received by the minion.

        For each job yielded by this generator, a corresponding submit() must
        be called for that job.

        Yields
        ------
        job : (object, object)
            The result of calling `next_job()`.

        See Also
        --------
        Minion.next_job()
        """
        while True:
            yield self.next_job()

    def submit(self, result):
        """Submit the result of the current job.

        The result must correspond to the job that was returned by the last call
        to `next_job()`.

        Parameters
        ----------
        result : object
            A picklable object that represents the result of the job.
        """
        if self._got_job is False:
            raise RuntimeError('Minion must ask for a job before submitting'
                               'a job result.')
        self._got_job = False

        # Submit the result
        self._BASE.submit_result(self, ResultData(self.job_type, result))


def run_minion(worker, func, job_type=0, unpack=False):
    """Create a Minion to run a work function.

    Parameters
    ----------
    worker : `Worker` instance
        An instance of `Worker` to connect to.
    func : callable
        The work function for the Minions to perform. The input of this
        function is the job data received by calling `Minion`.`next_job()`.
        The output must be a picklable object that represents the result
        of the job. The output is passed to `Minion`.`submit()`.
    job_type : int, optional
        The type of job these new Minions should handle. Defaults to 0.
    unpack : boolean, optional
        If this is True, then the job data is unpacked as arguments to `func`.
        Defaults to False.
    """
    if unpack:
        m = Minion(worker, job_type)
        for _, job_data in m.jobs():
            m.submit(func(*job_data))
    else:
        m = Minion(worker, job_type)
        for _, job_data in m.jobs():
            m.submit(func(job_data))

