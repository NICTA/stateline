//! \file comms/requester.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/requester.hpp"

#include "comms/protocol.hpp"

#include <string>

namespace stateline { namespace comms {

Requester::Requester(zmq::context_t& ctx, const std::string& addr)
  : socket_{ctx, zmq::socket_type::dealer, "toDelegator"}
{
  socket_.setIdentity();
  socket_.connect(addr);
}

void Requester::submit(BatchID id, const std::vector<double>& data)
{
  protocol::BatchJob batchJob;
  batchJob.id = id;
  batchJob.data = data;

  socket_.send({"", BATCH_JOB, serialise(batchJob)});
}

std::pair<uint, std::vector<double>> Requester::retrieve()
{
  const auto msg = socket_.recv();
  const auto batchResult = protocol::unserialise<protocol::BatchResult>(msg.data);

  return {batchResult.id, std::move(batchResult.data)};
}

} }
