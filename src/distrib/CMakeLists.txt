# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(distrib multigaussian.cpp)

ADD_EXECUTABLE ( test-distrib-gaussian testgaussian.cpp )
TARGET_LINK_LIBRARIES (test-distrib-gaussian ${obsidianAlgoLibraries} ${obsidianBaseLibraries})
REGISTER_UNIT_TESTS( test-distrib-gaussian )
