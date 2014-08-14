//!
//! \file comms/minion.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/minion.hpp"

#include <cstdint>

#include "comms/serial.hpp"

namespace stateline
{
  namespace comms
  {
    Minion::Minion(Worker& w, uint jobID)
        : socket_(w.zmqContext(), ZMQ_DEALER),
          jobIDString_(detail::serialise<std::uint32_t>(jobID))
    {
      auto socketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(socketID, socket_);
      socket_.connect(WORKER_SOCKET_ADDR.c_str());
    }

    JobData Minion::nextJob()
    {
      // Make sure we conform to the spec of GDF-SW comms
      if (firstMessage_)
      {
        send(socket_, Message(stateline::comms::JOBREQUEST, { jobIDString_ }));
        firstMessage_ = false;
      }
      VLOG(3) << "Minion waiting on next job";
      stateline::comms::Message r = receive(socket_);
      requesterAddress_ = r.address;
      std::string addrString = "Minion received address: ";
      for (auto a : r.address)
      {
        addrString += a + "::";
      }
      VLOG(3) << addrString;
      JobData j;

      // type, globalData, JobData IN THAT ORDER
      j.type = detail::unserialise<std::uint32_t>(r.data[0]);
      j.globalData = r.data[1];
      j.jobData = r.data[2];
      return j;
    }

    void Minion::submitResult(const ResultData& result)
    {
      // Order of data: Result type, result data, new job type request
      Message m(requesterAddress_, JOBSWAP,
          { detail::serialise<std::uint32_t>(result.type), result.data, jobIDString_ });

      VLOG(3) << "Minion submitting " << m;
      send(socket_, m);
    }

  } // namespace comms
} // namespace stateline
