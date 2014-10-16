import _stateline as _sl


def initialise(log_level, use_std_err, directory):
    """Initialise the logging system."""

    _sl.init_logging("stateline", log_level, use_std_err, directory)


def no_logging():
    """Initialise the logging system to be silent."""

    initialise(4, True, '.')
