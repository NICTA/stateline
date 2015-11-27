FROM alistaireid/pystateline:stateline-env
MAINTAINER Lachlan McCalman <lachlan.mccalman@nicta.com.au>

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
