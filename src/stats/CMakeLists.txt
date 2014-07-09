# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(stats multivariate.cpp uniform.cpp normal.cpp diagnormal.cpp)

ADD_TEST(test-stats-uniform testuniform.cpp stats)
ADD_TEST(test-stats-normal testnormal.cpp stats)
