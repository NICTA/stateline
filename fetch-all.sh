#!/bin/bash

export STATELINE_SOURCE_DIR=$(readlink -f $(dirname $0))
export PREREQ_DIR=$STATELINE_SOURCE_DIR/build/prereqs

mkdir build
cd build
cp -r ../prereqs .
cd prereqs
./buildPrereqs.sh
cd ..

cmake $STATELINE_SOURCE_DIR -DSTATELINE_BINARY_DIR=$(pwd) -DSTATELINE_SOURCE_DIR=$STATELINE_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR ..
