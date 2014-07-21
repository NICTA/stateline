"""This module contains the delegator class which performs job allocation and
manage ment for workers."""

import _stateline as _sl
import cPickle

class Delegator(_sl.Delegator):
    """Class used to allocate jobs to workers."""

    _BASE = _sl.Delegator

    def __init__(self, port, global_spec=None, job_specs=None, poll_rate=100):
        """Create a new delegator.

        Params
        ------
        port : int
            The port number for the delegator to listen on.

        global_spec : object, optional
            A picklable object to send to a worker when it first connects.
            This object is sent to all workers (hence 'global'). By default,
            `None` is sent as the global specification object.

        job_specs : list or dict, optional
            A dictionary containing objects to send to minions with specific
            job types. Each key in the dictionary specifies a job type (keys can
            be integers or strings), and each value is an object that is sent
            to minions that handle the corresponding job type. By default,
            the delegator sends `None` as the job spec for all job types.

        poll_rate : int, optional
            The number of milliseconds before polls for worker messages.
            Defaults to 100ms.
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
        p_job_specs = ((k, cPickle.dumps(v)) for k, v in job_specs.items())
        self._BASE.__init__(self, p_global_spec, p_job_specs, del_s)

        # Set python friendly properties
        self._port = port
        self._poll_rate = poll_rate

    @property
    def port(self):
        """Get the port number that this delegator is listening on."""
        return self._port

    @property
    def poll_rate(self):
        """Get the rate at which the delegator is polling for messages."""
        return self._poll_rate

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
