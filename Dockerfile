FROM ubuntu:15.10
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

RUN apt-get update && apt-get install -y \
  build-essential \
  git \
  libbz2-dev \
  bzip2 \
  cmake \
  wget \
  unzip \
  nmap \
  libeigen3-dev \
  libzmq3-dev \
  libboost-all-dev \
  libgtest-dev

ENV PREREQ_DIR=/usr/local BUILD_DIR=/tmp/stateline LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

RUN mkdir -p /stateline /usr/local/src/stateline /usr/local/src/stateline/tools /tmp/build

COPY ./tools /usr/local/src/stateline/tools
# RUN  /usr/local/src/stateline/tools/fetch-deps

COPY . /usr/local/src/stateline/
RUN mkdir /usr/local/build
WORKDIR /usr/local/build
RUN  cmake /usr/local/src/stateline -DCMAKE_BUILD_TYPE=Release -DLOCAL_INSTALL=OFF -DPREREQ_DIR=/usr/local \
  &&  make \
  &&  make install \
  &&  apt-get purge -y git bzip2 wget unzip \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /stateline
EXPOSE 5555 5556
