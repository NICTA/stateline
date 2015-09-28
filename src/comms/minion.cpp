//!
//! \file comms/minion.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/minion.hpp"

#include <boost/algorithm/string.hpp>
#include <easylogging/easylogging++.h>

namespace stateline
{
  namespace comms
  {
    Minion::Minion(zmq::context_t& context, const std::string socketAddr)
        : socket_(context, ZMQ_DEALER, "toWorker")
    {
      socket_.connect(socketAddr.c_str());
      socket_.send({HELLO,{""}});
    }

    Minion::Minion(zmq::context_t& context, const std::pair<uint, uint>& jobTypesRange,
                   const std::string socketAddr)
        : socket_(context, ZMQ_DEALER, "toWorker")
    {
      socket_.connect(socketAddr.c_str());
      std::string jobstring = std::to_string(jobTypesRange.first) + ":" +
                              std::to_string(jobTypesRange.second);
      socket_.send({HELLO,{jobstring}});
    }

    std::pair<uint, std::vector<double>> Minion::nextJob()
    {
      VLOG(3) << "Minion waiting on next job";
      stateline::comms::Message r = socket_.receive();
      currentJob_ = r.data[1];

      std::vector<std::string> sampleVectorStr;
      boost::algorithm::split(sampleVectorStr, r.data[2], boost::is_any_of(":"));

      std::vector<double> sample(sampleVectorStr.size());
      for (uint i = 0; i < sample.size(); i++)
        sample[i] = std::stod(sampleVectorStr[i]);

      return std::make_pair(std::stoi(r.data[0]), sample);
    }

    void Minion::submitResult(double result)
    {
      socket_.send({RESULT,{currentJob_, std::to_string(result)}});
    }

  } // namespace comms
} // namespace stateline
