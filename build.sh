#!/bin/bash

# Build script for Obsidian
# =========================
# Instructions:
# 1. Build the prerequisites (see prereq folder README)
# 2. Call this script from your build folder

# FILL THESE IN
export OBSIDIAN_SOURCE_DIR=$(readlink -f $(dirname $0))
export PREREQ_DIR=$OBSIDIAN_SOURCE_DIR/prereqs

cmake $OBSIDIAN_SOURCE_DIR -DOBSIDIAN_BINARY_DIR=$(pwd) -DOBSIDIAN_SOURCE_DIR=$OBSIDIAN_SOURCE_DIR -DPREREQ_DIR=$PREREQ_DIR $@

