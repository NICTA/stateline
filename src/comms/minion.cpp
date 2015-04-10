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

    std::pair<std::string, std::string> Minion::nextJob()
    {
      VLOG(3) << "Minion waiting on next job";
      stateline::comms::Message r = socket_.receive();  
      currentJob_ = r.data[1];
      return std::make_pair(r.data[0], r.data[2]);
    }

    void Minion::submitResult(const std::string& result)
    {
      socket_.send({RESULT,{currentJob_,result}});
    }

  } // namespace comms
} // namespace stateline
