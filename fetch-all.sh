#!/bin/sh -ex

SOURCE_DIR=$(pwd)
BUILD_DIR=$SOURCE_DIR/build
PREREQ_DIR=$BUILD_DIR/prereqs

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cp -r $SOURCE_DIR/prereqs $PREREQ_DIR
cd $PREREQ_DIR
source buildPrereqs.sh

exit 1

# Configure debug folder
mkdir debug
cd debug
cmake $STATELINE_SOURCE_DIR -DSTATELINE_BINARY_DIR=$(pwd) -DSTATELINE_SOURCE_DIR=$STATELINE_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR ../..
cd ..

# Configure release folder
mkdir release
cd release
cmake $STATELINE_SOURCE_DIR -DSTATELINE_BINARY_DIR=$(pwd) -DSTATELINE_SOURCE_DIR=$STATELINE_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR ../.. -DCMAKE_BUILD_TYPE=Release
cd ..
