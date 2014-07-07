#!/bin/bash

#BUILD ALL GDF PREREQUISITES
#MUST HAVE TAR and UNZIP installed.

# checkout from repo
mkdir src
mkdir include
mkdir lib
mkdir bin
export PREREQ_DIR=$(pwd)
cd src


# Boost 1.55
wget -c http://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.gz
[ -d boost_1_55_0 ] || tar -xvf boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh
./b2 -j $(nproc) --layout=versioned variant=debug,release threading=multi link=static runtime-link=static  toolset=gcc address-model=64 install --prefix=$PREREQ_DIR
cd ..

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

# Protocol-buffers (protobuf) 2.5.0
wget -c http://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz
[ -d protobuf-2.5.0 ] || tar -xvf protobuf-2.5.0.tar.gz
cd protobuf-2.5.0
./configure --prefix=$PREREQ_DIR
make -j$(nproc)
make install
cd ..

# leveldb
wget -c https://leveldb.googlecode.com/files/leveldb-1.15.0.tar.gz
[ -d leveldb-1.15.0 ] || tar -xvf leveldb-1.15.0.tar.gz
cd leveldb-1.15.0
make -j$(nproc)
cp libleveldb.a libleveldb.so libleveldb.so.1 libleveldb.so.1.15 $PREREQ_DIR/lib
cp -r include/leveldb $PREREQ_DIR/include/
cd ..
