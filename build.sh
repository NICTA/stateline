#!/bin/bash

# Build script for Stateline
# =========================
# Instructions:
# 1. Build the prerequisites (see prereq folder README)
# 2. Call this script from your build folder

# FILL THESE IN
export STATELINE_SOURCE_DIR=$(readlink -f $(dirname $0))
export PREREQ_DIR=$STATELINE_SOURCE_DIR/prereqs

cmake $STATELINE_SOURCE_DIR -DSTATELINE_BINARY_DIR=$(pwd) -DSTATELINE_SOURCE_DIR=$STATELINE_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR $@

