//!
//! Stateline worker interface.
//!
//! \file worker.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include <memory>
#include <string>
#include <vector>

namespace stateline
{

// TODO: add back the Worker class. Currently it's not very useful...

using LogLFn = std::function<double(const std::string&, const std::vector<double>&)>;

void runWorkers(const LogLFn& logLFn, const std::string& addr,
    const std::vector<std::string>& jobTypes,
    uint nThreads = 1);

}
