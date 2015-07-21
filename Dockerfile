FROM ubuntu:15.04
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>
RUN apt-get update && apt-get install -y \
  build-essential \
	bzip2 \
	cmake \
	wget \
  unzip

COPY prereqs/buildPrereqs.sh /usr/local/
WORKDIR /usr/local
RUN ./buildPrereqs.sh

# get stateline inside and compiled
RUN mkdir /tmp/stateline /tmp/build /tmp/stateline/CMakeModules /tmp/stateline/src
COPY CMakeLists.txt /tmp/stateline/
COPY src /tmp/stateline/src/
COPY CMakeModules /tmp/stateline/CMakeModules/
WORKDIR /tmp/build
RUN cmake /tmp/stateline -DCMAKE_BUILD_TYPE=Release -DLOCAL_INSTALL=OFF -DPREREQ_DIR=/usr/local && make && make install



