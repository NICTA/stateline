# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014

ADD_LIBRARY(commoncomms OBJECT message.cpp socket.cpp heartbeat.cpp)
ADD_LIBRARY(servercomms OBJECT requester.cpp delegator.cpp)
ADD_LIBRARY(clientcomms OBJECT agent.cpp)
