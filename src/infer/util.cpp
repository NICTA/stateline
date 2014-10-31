//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA

#include "infer/util.hpp"
#include "app/serial.hpp"
#include "comms/serial.hpp"

namespace stateline
{
  namespace mcmc
  {
    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x)
    {
      return {{ 0, "", serialise(x) }};
    }

    double singleJobEnergy(const std::vector<comms::ResultData> &results)
    {
      return comms::detail::unserialise<double>(results[0].data);
    }

  } // namespace mcmc
} // namespace stateline
