FROM alistaireid/statbuntu
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  build-essential \
  git \
  libbz2-dev \
  bzip2 \
  cmake \
  wget \
  unzip \
  nmap

ENV PREREQ_DIR=/usr/local BUILD_DIR=/tmp/stateline LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

RUN mkdir -p /stateline /usr/local/src/stateline /usr/local/src/stateline/tools /tmp/build
COPY . /usr/local/src/stateline/tools
RUN  ./usr/local/src/stateline/tools/fetch-deps 

