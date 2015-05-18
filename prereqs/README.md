Compiling Prerequisites
=======================

TLDR:
-----
  ./buildPrereqs.sh


The simplest way to deal with the required libraries for Stateline is to build and
install them into a prerequisite directory, then explicitly point CMake to
that directory in order to find the libraries.

The project currently builds with the following libaries:

* Boost 1.55
* Eigen 3.2.0
* google-log (glog) 0.3.3
* google-test (gtest) 1.7.0
* zeromq 4.0.3
* cppzeromq 2358037407 (commit hash)


There is a build script buildPrereqs.sh that should automatically download and
build all the requirements. Just run it and see!


The individual instructions for building each of these libraries for the
project are are also given below. We assume that you defined an environment
variable describing the location of a prerequisite directory, i.e.

export PREREQ_DIR=<directory of prerequisite library installations>


from boost source directory
---------------------------
    ./bootstrap.sh
    ./b2 -j 8 --layout=versioned variant=debug,release threading=multi link=shared runtime-link=shared  toolset=gcc address-model=64 install --prefix=$PREREQ_DIR


from eigen src directory
-------------------------
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=$PREREQ_DIR
    make install

from glog src directory
-----------------------
    ./configure --prefix=$PREREQ_DIR
    make install

from gtest src directory
------------------------
    ./configure --prefix=$PREREQ_DIR --enable-shared --enable-static

from zeromq src directory
-------------------------
    ./configure --prefix=$PREREQ_DIR
    make -j8
    make install

from cppzmq src directory
-------------------------
    cp zmq.hpp $PREREQ_DIR/include
