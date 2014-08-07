"""This module contains communications and networking classes."""

import _stateline as _sl
import cPickle


class Delegator(_sl.Delegator):
    """Class used to allocate jobs to workers.

    An instance of this class acts as a server that workers can connect to.
    Instances of the `Requester` class can be created from a `Delegator`

    instance, which can be used to submit jobs to workers and retrieve the
    corresponding results.

    Notes
    -----
    Only one instance of this class should exist, but there can be multiple
    `Requester` instances that connect to a delegator.
    """

    _BASE = _sl.Delegator

    def __init__(self, port, global_spec=None, job_specs=None, poll_rate=100):
        """Create a new delegator.

        Parameters
        ----------
        port : int
            The port number for the delegator to listen on.
        global_spec : object, optional
            A picklable object to send to a worker when it first connects.
            This object is sent to all workers (hence 'global'). By default,
            `None` is sent as the global specification object.
        job_specs : dict, optional
            A dictionary containing picklable objects to send to workers who
            request specific job types. Each key in the dictionary specifies a
            job type (job types must be integers). When a worker connects to the
            server, the objects corresponding to the job types that the worker
            is willing to do is sent along with the global spec. By default,
            the delegator sends `None` for all job types.
        poll_rate : int, optional
            The number of milliseconds before polls for worker messages.
            Defaults to 100ms.

        Examples
        --------
        Create and start a delegator on port 5555 with no specification objects.
        >>> delegator = Delegator(5555)
        >>> delegator.start()

        Create a delegator with a global specification object as well as two
        job specific spec objects.
        >>> job_specs = {0: 'eggs', 1: 'ham'}
        >>> delegator = Delegator(5555,
        ...     global_spec='green', job_specs=job_specs)
        """
        # Create settings structs for the super constructor
        hb_s = _sl.HeartbeatSettings()
        hb_s.rate, hb_s.poll_rate, hb_s.timeout = 1000, 500, 10000

        del_s = _sl.DelegatorSettings()
        del_s.port, del_s.poll_rate, del_s.heartbeat = port, poll_rate, hb_s

        if job_specs is None:
            # Default to an empty dictionary
            job_specs = {}

        # Pickle the specifications objects
        p_global_spec = cPickle.dumps(global_spec)
        p_job_specs = dict((k, cPickle.dumps(v)) for k, v in job_specs.items())
        self._BASE.__init__(self, p_global_spec, p_job_specs, del_s)

        # Set python friendly properties
        self._port = port
        self._poll_rate = poll_rate
        self._job_specs = job_specs

    @property
    def port(self):
        """Get the port number that this delegator is listening on."""
        return self._port

    @property
    def poll_rate(self):
        """Get the rate at which the delegator is polling in milliseconds."""
        return self._poll_rate

    @property
    def job_specs(self):
        """Get the job specification objects for each job type."""
        return self._job_specs

    def start(self):
        """Start polling and heartbeating.

        This must be called for workers to connect to this delegator.
        """
        self._BASE.start(self)

    def stop(self):
        """Stop polling and heartbeating.

        Workers will automatically disconnect when the heartbeat timeout is
        reached.
        """
        self._BASE.stop(self)


class Requester(_sl.Requester):
    """Class used to submit jobs to workers and get the corresponding results.

    The job submission system is designed to be asynchronous, so submitting a
    job does not block. However, the order in which job results arrive may not
    be the same as the order in which the jobs were submitted.

    Notes
    -----
    There can be multiple `Requester` instances that connect to a delegator.

    Examples
    --------
    Create a requester and submit a single job.
    >>> delegator = Delegator(5555)
    >>> delegator.start()
    >>> requester = Requester(delegator)
    >>> requester.submit(0, "Hello!")
    >>> requester.retrieve()
    (0, "World!")

    Create a requester and submit several batches of different types.
    >>> delegator = Delegator(5555)
    >>> delegator.start()
    >>> requester = Requester(delegator)
    >>> requester.submit(0, [(0, "Two"), (1, 2)], global_data="English")
    >>> requester.submit(1, [(0, "Deux"), (1, 2)], global_data="French")
    >>> requester.retrieve()
    (1, [(0, "Trois"), (1, 3)])
    >>> requester.retrieve()
    (0, [(0, "Three"), (1, 3)])

    Create a requester and submit multiple jobs of the same type.
    >>> delegator = Delegator(5555)
    >>> delegator.start()
    >>> requester = Requester(delegator)
    >>> requester.submit(0, [(0, "Eggs"), (0, "Ham"), (0, "Spam")])
    >>> requester.retrieve()
    (0, [(0, "Bacon"), (0, "Cheese"), (0, "Ham")])
    """

    _BASE = _sl.Requester

    def __init__(self, delegator):
        """Create a new requester.

        Parameters
        ----------
        delegator : `Delegator` instance
            An instance of `Delegator` to connect to.
        """
        self._BASE.__init__(self, delegator)
        self._delegator = delegator
        self._num_jobs = 0

    @property
    def delegator(self):
        """Get the delegator that this requester is connected to."""
        return self._delegator

    def submit(self, batch_id, jobs, global_data=None):
        """Submit a batch of jobs to a worker and returns immediately.

        Parameters
        ----------
        batch_id : int
            A number used to identify this particular batch. Because job results
            may arrive out of order, this number is used to identify the jobs
            that matches a result received from a worker.
        jobs : list
            The job(s) to submit. If this is an picklable object, then the job
            is submitted with job type 0. If this is a list, then each element
            represents a job as a pair. First element of the pair is the type
            of job it is, and the second element is a picklable object which
            contains the data specific to that job.
        global_data : object, optional
            A picklable object to send along with all the jobs in this batch.
        """
        # Add each of the jobs into a batch
        p_global_data = cPickle.dumps(global_data)
        batch = []
        for job_type, job_data in jobs:
            job = _sl.JobData()
            job.type = job_type
            job.job_data = cPickle.dumps(job_data)
            job.global_data = p_global_data
            batch.append(job)

        # Submit the batch
        self._num_jobs += 1
        self._BASE.batch_submit(self, batch_id, batch)

    def retrieve(self):
        """Wait for the result of a batch to arrive.

        Note that the batch could be any that has been submitted but has not
        been retrieved yet (there is no guarantee on which batch is next). The
        batch ID returned by this method can be used to identify which batch
        these results correspond to.

        Returns
        -------
        batch_id, results : (int, list)
            The first element contains the ID that was assigned to this batch
            when it was submitted. The second element is a list containing
            the results for each job that was submitted in the batch. Each
            result is a pair containing the job type and the result data. The
            order of the results in the list is the same as the order in which
            the jobs were stored in the batch.

        Raises
        ------
        RuntimeError
            There are no more job results to retrieve.
        """
        if self._num_jobs <= 0:
            raise RuntimeError("There are no more job results to retrieve.")
        self._num_jobs -= 1

        batch_id, raw = self._BASE.batch_retrieve(self)

        # Convert the raw results (a list of _sl.ResultData) into list of pairs
        results = [(res.type, cPickle.loads(res.get_data())) for res in raw]
        return batch_id, results

    def retrieve_all(self):
        """A generator which returns all the outstanding job results.

        Yields
        -------
        batch_id, results : (int, list)
            Return value of `retrieve()`

        See Also
        --------
        Requester.retrieve()
        """
        try:
            while True:
                yield self.retrieve()
        except RuntimeError:
            pass


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
        self._global_spec = cPickle.loads(self._BASE.global_spec(self))
        self._job_specs = dict()
        for job_type in job_types:
            spec = self._BASE.job_spec(self, job_type)
            if len(spec) == 0:
                # Empty spec means that the delegator did not have a spec for
                # this job type
                self._job_specs[job_type] = None
            else:
                self._job_specs[job_type] = cPickle.loads(spec)

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
        return (cPickle.loads(raw.get_global_data()),
                cPickle.loads(raw.get_job_data()))

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

        # Convert the result into a raw JobResult struct
        raw = _sl.ResultData()
        raw.type = self.job_type
        raw.data = cPickle.dumps(result)
        self._BASE.submit_result(self, raw)


def run_minion(worker, func, job_type=0, use_global=False):
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
    use_global : boolean, optional
        If this is True, then the global data of each job is also passed to
        the work function (calls func(global_data, job_data)). Otherwise,
        only the job data is passed to the work function. Defaults to False.
    """
    if use_global is True:
        m = Minion(worker, job_type)
        for global_data, job_data in m.jobs():
            m.submit(func(global_data, job_data))
    else:
        m = Minion(worker, job_type)
        for _, job_data in m.jobs():
            m.submit(func(job_data))

