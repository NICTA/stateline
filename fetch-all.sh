#!/bin/bash

export STATELINE_SOURCE_DIR=$(readlink -f $(dirname $0))
export PREREQ_DIR=$STATELINE_SOURCE_DIR/build/prereqs

mkdir build
cd build
mkdir prereqs
cd ..
cp prereqs/buildPrereqs.sh build/prereqs
cd build/prereqs
./buildPrereqs.sh
cd ..

cmake $STATELINE_SOURCE_DIR -DSTATELINE_BINARY_DIR=$(pwd) -DSTATELINE_SOURCE_DIR=$STATELINE_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR ..
