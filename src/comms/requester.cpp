//!
//! \file comms/requester.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
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
    void sendJob(zmq::socket_t& socket, const std::vector<JobID>& ids, const JobData& job)
    {
      // Delegate call to the move version.
      return sendJob(socket, ids, job);
    }

    void sendJob(zmq::socket_t& socket, const std::vector<JobID>& ids, JobData&& job)
    {
      std::vector<std::string> idStrings;
      for (auto i : ids)
      {
        idStrings.push_back(std::to_string(i));
      }

      Message m(idStrings, stateline::comms::JOB,
        { detail::serialise<std::uint32_t>(job.type), std::move(job.globalData), std::move(job.jobData) });
      send(socket, std::move(m));
    }

    //! Read a job result from a socket.
    //!
    //! \param socket The socket to read from.
    //! \return The job result that was read from the socket.
    //!
    ResultData receiveResult(zmq::socket_t& socket)
    {
      Message m = receive(socket);

      ResultData r {
        detail::unserialise<std::uint32_t>(m.data[0]),
        std::move(m.data[1])
      };

      return r;
    }

    Requester::Requester(Delegator& d)
        : socket_(d.zmqContext(), ZMQ_DEALER)
    {
      auto socketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(socketID, socket_);
      socket_.connect(DELEGATOR_SOCKET_ADDR.c_str());
    }

    void Requester::submit(uint id, const JobData& j)
    {
      batchSubmit(id, { j });
    }

    void Requester::submit(uint id, JobData&& j)
    {
      batchSubmit(id, { std::move(j) });
    }

    std::pair<uint, ResultData> Requester::retrieve()
    {
      auto r = batchRetrieve();
      return { r.first, std::move(r.second.front()) };
    }

    void Requester::batchSubmit(uint id, const std::vector<JobData>& jobs)
    {
      // Delegate call to the move version.
      return batchSubmit(id, jobs);
    }

    void Requester::batchSubmit(uint id, std::vector<JobData>&& jobs)
    {
      uint nJobs = jobs.size();
      batches_[id] = std::vector<ResultData>(nJobs);
      batchLeft_[id] = nJobs;

      for (uint i = 0; i < nJobs; i++)
      {
        sendJob(socket_, { id, i }, std::move(jobs[i]));
      }
    }

    std::pair<uint, std::vector<ResultData>> Requester::batchRetrieve()
    {
      while (true)
      {
        Message r = receive(socket_);

        uint batchNum = std::stoul(r.address[0]);
        uint idx = std::stoul(r.address[1]);

        batches_[batchNum][idx] =
        {
          detail::unserialise<std::uint32_t>(r.data[0]),
          std::move(r.data[1])
        };

        if (--batchLeft_[batchNum] == 0) {
          // This batch has finished
          return std::make_pair(batchNum, std::move(batches_[batchNum]));
        }
      }
    }

  } // namespace comms
} // namespace obsidian
