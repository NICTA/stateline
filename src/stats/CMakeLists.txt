# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(stats multivariate.cpp uniform.cpp normal.cpp diagnormal.cpp truncnormal.cpp)

ADD_TEST(test-stats-uniform tests/uniform.cpp stats)
ADD_TEST(test-stats-normal tests/normal.cpp stats)
ADD_TEST(test-stats-truncnormal tests/truncnormal.cpp stats)
