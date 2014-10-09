import _stateline as _sl


def initialise(log_level, use_std_err, directory):
    _sl.init_logging("stateline", log_level, use_std_err, directory)


def nolog():
    initialise(0, False, '.')
