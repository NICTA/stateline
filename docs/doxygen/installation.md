{#installation}
============

# Installation

The product should have been distributed in source form.

This will give you general overview on how to build the software and its prerequisites.

## Platform

The software has been developed and tested on various linux distributions and mac os.

## Toolchain

The software has been developed and tested using the latest GNU Compiler Collection (GCC) toolchain.

It is recommended that you install GCC version 4.9 or later before proceeding.

cmake and make are also essential

## Prerequisites

These sections list the various dependency libraries. There is a simple shell script (buildPrereqs.sh) in prereqs folder that automatically downloads and builds the necessary prerequisites. Change directory to the prereqs folder and run this script.

### Boost 1.55 or later

For easy integration, compile using static linking and static runtime.

### google-log (glog) 0.3.3 or later

### google-test (gtest) 1.7.0 or later

### zeromq 4.0.3 or later

### cppzeromq

### google-protobuf 2.5.0 or later

### leveldb 1.15.0 or later

### zlib

## Build from source

Once the prerequisites are built, make a new folder where you want to build the tools. Change current folder to that folder and run build.sh or buildRelease.sh in the source folder from that folder. This will invoke cmake
to produce necessary build scripts. Once this has completed successfully, build using the make command.
