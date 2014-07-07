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
    void sendJob(zmq::socket_t& socket, const JobData& job)
    {
      Message m({ }, stateline::comms::JOB, { detail::serialise<std::uint32_t>(job.type), job.globalData, job.jobData });
      send(socket, m);
    }

    //! Send a job over a ZMQ socket.
    //!
    //! \param socket The socket to send the job over.
    //! \param job The job to send.
    //!
    void sendJob(zmq::socket_t& socket, const std::vector<uint>& ids, const JobData& job)
    {
      std::vector<std::string> idStrings;
      for (auto i : ids)
      {
        idStrings.push_back(std::to_string(i));
      }

      Message m(idStrings, stateline::comms::JOB, { detail::serialise<std::uint32_t>(job.type), job.globalData, job.jobData });
      send(socket, m);
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
        m.data[1]
      };

      return r;
    }

    //! Read a job result from a socket.
    //!
    //! \param socket The socket to read from.
    //! \return The job result that was read from the socket.
    //!
    std::pair<std::vector<uint>, ResultData> receiveResultAndIDs(zmq::socket_t& socket)
    {
      std::vector<uint> indices;
      Message m = receive(socket);
      for (auto& a : m.address)
      {
        indices.push_back((uint) std::stoi(a));
      }

      ResultData r
      {
        detail::unserialise<std::uint32_t>(m.data[0]);
        m.data[1];
      };

      return std::make_pair(indices, r);
    }

    Requester::Requester(Delegator& d)
        : socket_(d.zmqContext(), ZMQ_DEALER)
    {
      auto socketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(socketID, socket_);
      socket_.connect(DELEGATOR_SOCKET_ADDR.c_str());
    }

    ResultData Requester::operator()(const JobData& j)
    {
      sendJob(socket_, j);
      return receiveResult(socket_);
    }

    void Requester::submit(uint id, const JobData& j)
    {
      sendJob(socket_, { id }, j);
    }

    std::pair<uint, ResultData> Requester::retrieve()
    {
      auto r = receiveResultAndIDs(socket_);
      return { r.first[0], r.second};
    }

    std::vector<ResultData> Requester::batch(const std::vector<JobData>& jobs)
    {
      uint nJobs = jobs.size();
      for (uint i = 0; i < nJobs; i++)
      {
        submit(i, jobs[i]);
      }
      std::vector<ResultData> results(nJobs);
      for (uint i = 0; i < nJobs; i++)
      {
        auto p = retrieve();
        results[p.first] = p.second;
      }
      return results;
    }

    void Requester::batchSubmit(uint id, const std::vector<JobData>& jobs)
    {
      uint nJobs = jobs.size();
      batches_.insert(std::make_pair(id, std::vector<ResultData>(nJobs)));
      batchSizes_.insert(std::make_pair(id, nJobs));
      batchNComplete_.insert(std::make_pair(id, 0));
      for (uint i = 0; i < nJobs; i++)
      {
        sendJob(socket_, { id, i }, jobs[i]);
      }
    }

    std::pair<uint, std::vector<ResultData>> Requester::batchRetrieve()
    {
      bool batchFinished = false;
      uint finishedBatch = 0;
      while (!batchFinished)
      {
        stateline::comms::Message r = stateline::comms::receive(socket_);
        uint batchNum = std::stoul(r.address[0]);
        uint idx = std::stoul(r.address[1]);
        ResultData res
        {
          detail::unserialise(r.data[0]);
          r.data[1];
        };

        batches_[batchNum][idx] = res;
        batchNComplete_[batchNum]++;
        if (batchNComplete_[batchNum] == batchSizes_[batchNum])
        {
          batchFinished = true;
          finishedBatch = batchNum;
        }
      }
      std::vector<ResultData> result = batches_[finishedBatch];
      batches_.erase(finishedBatch);
      batchSizes_.erase(finishedBatch);
      batchNComplete_.erase(finishedBatch);
      return std::make_pair(finishedBatch, result);
    }

  } // namespace comms
} // namespace obsidian
