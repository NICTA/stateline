#!/bin/bash

mkdir -p output
g++ demo-worker.cpp -I../../src/ -L../../build/debug -lstateline -lpthread -std=c++11 -o demo-worker
g++ demo-server.cpp -I../../src/ -I../../build/prereqs/include/eigen3 -I../../build/prereqs/include/boost-1_55/ -I../../build/prereqs/include -L../../build/debug -lstateline -lpthread -std=c++11 -o demo-server
