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

    void Requester::submit(uint id, const std::vector<std::string>& jobTypes, const Eigen::VectorXd& data)
    {
        std::string jtstring = boost::algorithm::join(jobTypes, ":");

        std::vector<std::string> dataVectorStr;
        for (uint i = 0; i < data.size(); i++) {
          dataVectorStr.push_back(std::to_string(data(i)));
        }

        socket_.send({{ std::to_string(id)}, REQUEST, { jtstring, boost::algorithm::join(dataVectorStr, ":") }});
    }

    std::pair<uint, std::vector<double>> Requester::retrieve()
    {
      Message r = socket_.receive();
      uint id = std::stoul(r.address[0]);

      std::vector<double> results;
      for (const auto& x : r.data)
      {
        results.push_back(std::stod(x));
      }

      return std::make_pair(id, results);
    }

  } // namespace comms
} // namespace stateline
