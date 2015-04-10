//!
//! \file comms/requester.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/requester.hpp"
#include "comms/delegator.hpp"

#include <iterator>
#include <string>
#include <boost/algorithm/string.hpp>

namespace stateline
{
  namespace comms
  {
    Requester::Requester(zmq::context_t& context)
        : socket_(context, ZMQ_DEALER ,"toDelegator")
    {
      socket_.setIdentifier();
      socket_.connect(DELEGATOR_SOCKET_ADDR.c_str());
    }

    void Requester::submit(uint id, const std::vector<std::string>& jobTypes, const std::string& data)
    {
        std::string jtstring = boost::algorithm::join(jobTypes, ":");
        socket_.send({{ std::to_string(id)}, REQUEST, { jtstring, data }});
    }

    std::pair<uint, std::vector<std::string>> Requester::retrieve()
    {
      Message r = socket_.receive();
      uint id = std::stoul(r.address[0]);
      return std::make_pair(id, r.data);
    }

  } // namespace comms
} // namespace stateline
