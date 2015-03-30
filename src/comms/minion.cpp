//!
//! \file comms/minion.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/minion.hpp"

#include <cstdint>

#include "comms/serial.hpp"

namespace stateline
{
  namespace comms
  {
    Minion::Minion(Worker& w, JobType jobType)
        : socket_(w.zmqContext(), ZMQ_DEALER),
          jobTypeString_(detail::serialise<std::uint32_t>(jobType))
    {
      auto socketID = stateline::comms::randomSocketID();
      stateline::comms::setSocketID(socketID, socket_);
      socket_.connect(WORKER_SOCKET_ADDR.c_str());
    }

    std::string Minion::nextJob()
    {
      // Make sure we conform to the Stateline protocol
      if (firstMessage_)
      {
        // TODO: what do we do here?
        send(socket_, Message(stateline::comms::WORK, { jobTypeString_ }));
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

      // r.data[0] is job type
      // r.data[1] is the data
      return r.data[1];
    }

    void Minion::submitResult(const std::string& result)
    {
      Message m(requesterAddress_, stateline::comms::WORK,
          { jobTypeString_, result, jobTypeString_});

      VLOG(3) << "Minion submitting " << m;
      send(socket_, m);
    }

  } // namespace comms
} // namespace stateline
