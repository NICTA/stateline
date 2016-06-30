//! \file comms/requester.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/requester.hpp"

#include "comms/protobuf.hpp"

#include <string>

namespace stateline { namespace comms {

Requester::Requester(zmq::context_t& ctx, const std::string& addr)
  : socket_{ctx, zmq::socket_type::dealer, "toDelegator"}
{
  socket_.setIdentity();
  socket_.connect(addr);
}

void Requester::submit(BatchID id, const std::vector<JobType>& jobTypes,
    const std::vector<double>& data)
{
  messages::BatchJob batchJob;
  batchJob.set_id(id);

  for (const auto& jobType : jobTypes)
    batchJob.add_job_type(jobType);

  for (auto x : data)
    batchJob.add_data(x);

  socket_.send({"", BATCH_JOB, protobufToString(batchJob)});
}

std::pair<uint, std::vector<double>> Requester::retrieve()
{
  const auto msg = socket_.recv();
  const auto batchResult = stringToProtobuf<messages::BatchResult>(msg.data);

  std::vector<double> data;
  for (int i = 0; i < batchResult.data_size(); i++)
    data.push_back(batchResult.data(i));

  return {batchResult.id(), std::move(data)};
}

} }
