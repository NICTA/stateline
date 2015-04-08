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

#include "comms/serial.hpp"

namespace stateline
{
  namespace comms
  {
    //! Send a job over a ZMQ socket.
    //!
    //! \param socket The socket to send the job over.
    //! \param job The job to send.
    //!
    void sendJob(zmq::socket_t& socket, const std::vector<uint>& id, uint jobType, const std::string& data)
    {
      std::vector<std::string> idString;
      for (auto i : id)
      {
        idString.push_back(std::to_string(i));
      }

      Message m(idString, stateline::comms::WORK, { detail::serialise<std::uint32_t>(jobType), data });
      send(socket, m);
    }

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

    void Requester::submit(uint id, JobType jobType, const std::string& data)
    {
      batchSubmit(id, { jobType }, data);
    }

    std::pair<uint, std::string> Requester::retrieve()
    {
      auto r = batchRetrieve();
      return { r.first, std::move(r.second.front()) };
    }

    void Requester::batchSubmit(JobID id, const std::vector<JobType>& jobTypes, const std::string &data)
    {
      uint nJobs = jobTypes.size();
      batches_[id] = std::vector<std::string>(nJobs);
      batchLeft_[id] = nJobs;

      for (uint i = 0; i < nJobs; i++)
      {
        sendJob(socket_, { id, i }, jobTypes[i], data);
      }
    }

    std::pair<JobID, std::vector<std::string>> Requester::batchRetrieve()
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
