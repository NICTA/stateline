# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014

ADD_LIBRARY(commonwrapper OBJECT logging.cpp signal.cpp)
ADD_LIBRARY(serverwrapper OBJECT serverwrapper.cpp api.cpp)
