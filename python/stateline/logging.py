import _stateline as _sl


def initialise(log_level, use_std_err, directory="."):
    """Initialise the logging system.
    
    Parameters
    ----------
    log_level : int
        Controls the amount of logging. Set this to be 0 for release logging.
        A negative value indicates verbose logging. A positive value indicates
        quiet logging.
    use_std_err : bool
        If set to True, standard error is used for logging. Otherwise, logs
        are written to a file in the directory specified by the `directory`
        parameter.
    directory : str, optional
        The directory used to store log files. This is ignored if use_std_err
        is True. Defaults to the current directory.
    """

    _sl.init_logging("stateline", log_level, use_std_err, directory)


def no_logging():
    """Initialise the logging system to be silent."""

    initialise(4, True, '.')
