stateline
=========
Stateline is a framework for distributed Markov Chain Monte Carlo (MCMC) sampling written in C++ with high-level Python bindings. It focuses on [parallel tempering](http://en.wikipedia.org/wiki/Parallel_tempering) methods which are highly parallelisable.

System Support
--------------
Currently, stateline runs on Linux-based operating systems only.

Compiler Support
----------------
Stateline has been compiled and tested under g++ 4.8.2.

Prerequisites
-------------
Stateline requires the following libraries as prerequisites:

* Boost 1.55
* Eigen 3.2.0
* google-log (glog) 0.3.3
* google-test (gtest) 1.7.0
* zeromq 4.0.3
* cppzeromq 2358037407 (commit hash)
* leveldb 1.15.0

Building
--------
There is a `build.sh` script included with the code in that can be used to easily build the project using CMake, by specifying environment variables corresponding to the locations of the prerequisite libraries. If you have installed them all into `/usr/local`, you can probably ignore them and just run CMake. Once you have run CMake in your build directory, a simple make command will suffice to build the project.

Documentation
-------------

Licence
-------

Bug Reports
-----------
If you find a bug, please open an [issue](http://github.com/NICTA/stateline/issues).

Contributing 
------------
Contributions and comments are welcome. Please read our [style guide](docs/CodeGuidelines.md) before submitting a pull request.
