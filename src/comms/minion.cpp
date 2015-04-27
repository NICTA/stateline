//!
//! \file comms/minion.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/minion.hpp"
#include <boost/algorithm/string.hpp>

namespace stateline
{
  namespace comms
  {
    Minion::Minion(zmq::context_t& context, const std::vector<std::string>& jobTypes)
        : socket_(context, ZMQ_DEALER, "toWorker")
    {
      socket_.connect(WORKER_SOCKET_ADDR.c_str());
      std::string jobstring = boost::algorithm::join(jobTypes, ":");
      socket_.send({HELLO,{jobstring}});
    }

    std::pair<std::string, Eigen::VectorXd> Minion::nextJob()
    {
      VLOG(3) << "Minion waiting on next job";
      stateline::comms::Message r = socket_.receive();
      currentJob_ = r.data[1];

      // TODO Serialisation code
      std::vector<std::string> sampleVectorStr;
      boost::algorithm::split(sampleVectorStr, r.data[2], boost::is_any_of(":"));

      Eigen::VectorXd sample(sampleVectorStr.size());
      for (uint i = 0; i < sample.size(); i++)
        sample(i) = std::stod(sampleVectorStr[i]);

      return std::make_pair(r.data[0], sample);
    }

    void Minion::submitResult(double result)
    {
      socket_.send({RESULT,{currentJob_, std::to_string(result)}});
    }

  } // namespace comms
} // namespace stateline
