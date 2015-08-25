#!/bin/sh -ex

# Script to build stateline prerequisites
# MUST HAVE TAR and UNZIP installed.

PREREQ_DIR=$(pwd)
N_PROC=$(getconf _NPROCESSORS_ONLN)

mkdir -p src include lib bin
cd src

# Boost 1.55
wget -c http://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.gz
[ -d boost_1_55_0 ] || tar -xvf boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh
./b2 -j $N_PROC --layout=versioned variant=debug,release threading=multi link=static runtime-link=static toolset=gcc address-model=64 install --prefix=$PREREQ_DIR
cd -

exit 1

# Eigen 3.2.0
wget -c http://bitbucket.org/eigen/eigen/get/3.2.0.tar.gz -O eigen_3.2.0.tar.gz
[ -d eigen-eigen-ffa86ffb5570 ] || tar -xvf eigen_3.2.0.tar.gz
cd eigen-eigen-ffa86ffb5570
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PREREQ_DIR
make install
cd ../..

# google-log (glog) 0.3.3
wget -c http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz
[ -d glog-0.3.3 ] || tar -xvf glog-0.3.3.tar.gz
cd glog-0.3.3
./configure --prefix=$PREREQ_DIR
make install
cd ..

# google-test (gtest) 1.7.0
wget -c http://googletest.googlecode.com/files/gtest-1.7.0.zip
[ -d gtest-1.7.0 ] || unzip -o gtest-1.7.0.zip

# zeromq 4.0.3
wget -c http://download.zeromq.org/zeromq-4.0.3.tar.gz
[ -d zeromq-4.0.3 ] || tar -xvf zeromq-4.0.3.tar.gz
cd zeromq-4.0.3
./configure --prefix=$PREREQ_DIR
make -j$(nproc)
make install
cd ..

# cppzeromq 2358037407 (commit hash)
wget -c https://github.com/zeromq/cppzmq/archive/master.zip -O cppzmq-master.zip
[ -d cppzmq-master ] || unzip -o cppzmq-master.zip
cp cppzmq-master/zmq.hpp $PREREQ_DIR/include

# json
wget -c https://github.com/nlohmann/json/archive/master.zip -O json-master.zip
[ -d json-master ] || unzip -o json-master.zip
cp json-master/src/json.hpp $PREREQ_DIR/include
