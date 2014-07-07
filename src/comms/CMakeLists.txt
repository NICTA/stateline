# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(delegator delegator.cpp)

ADD_LIBRARY(minion minion.cpp)

ADD_LIBRARY(requester requester.cpp)

ADD_LIBRARY(worker worker.cpp)

ADD_LIBRARY(clientheartbeat clientheartbeat.cpp)

ADD_LIBRARY(serverheartbeat serverheartbeat.cpp)

ADD_LIBRARY(messages messages.cpp)

ADD_LIBRARY(router router.cpp)

ADD_LIBRARY(transport 
  transport.cpp)

ADD_EXECUTABLE ( test-comms testcomms.cpp )

TARGET_LINK_LIBRARIES ( test-comms ${obsidianClientLibraries}
                                   ${obsidianServerLibraries}
                                   ${obsidianCommsLibraries}
                                   ${obsidianBaseLibraries})

REGISTER_UNIT_TESTS( test-comms )
