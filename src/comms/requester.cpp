//!
//! \file comms/requester.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/requester.hpp"

#include <iterator>
#include <string>

namespace stateline
{
  namespace comms
  {
    //! Read a job result from a socket.
    //!
    //! \param socket The socket to read from.
    //! \return The job result that was read from the socket.
    //!
    std::string receiveResult(zmq::socket_t& socket)
    {
      Message m = receive(socket);
      return m.data[0];
    }

    Requester::Requester(zmq::context_t& context)
        : socket_(context, ZMQ_DEALER)
    {
      auto socketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(socketID, socket_);
      socket_.connect(DELEGATOR_SOCKET_ADDR.c_str());
    }

    void Requester::submit(uint id, const std::vector<JobType>& jobTypes, const std::string& data)
    {
      uint nJobs = jobTypes.size();
      batches_[id] = std::vector<std::string>(nJobs);
      batchLeft_[id] = nJobs;

      for (uint i = 0; i < nJobs; i++)
      {
        socket.send({{ std::to_string(id), std::to_string(i) }, WORK, { jobTypes[i], data });
      }
    }

    std::pair<uint, std::vector<std::string>> Requester::retrieve()
    {
      while (true)
      {
        Message r = receive(socket_);

        uint batchNum = std::stoul(r.address[0]);
        uint idx = std::stoul(r.address[1]);

        batches_[batchNum][idx] = std::move(r.data[1]);

        if (--batchLeft_[batchNum] == 0) {
          // This batch has finished
          return std::make_pair(batchNum, std::move(batches_[batchNum]));
        }
      }
    }

  } // namespace comms
} // namespace stateline
