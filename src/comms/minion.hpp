//!
//! Object which actually performs the work requested then distributed by the
//! worker. There can be many minions in the same machine, but only one per
//! thread sending to a shared worker object.
//!
//! \file comms/minion.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <string>
#include <thread>
// Prerequisites
#include <glog/logging.h>
#include <zmq.hpp>
// Project
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/worker.hpp"

namespace stateline
{
  namespace comms
  {
    //! Minion object that requests a job, computes the result, 
    //! then submits that result to the connected worker.
    //!
    class Minion
    {
    public:

      //! Create a new a minion.
      //!
      //! \param w The parent worker object it communicates with.
      //! \param jobID The ID of the job that the minion will do.
      //!
      Minion(Worker& w, uint jobID);

      //! Gets a job from the worker.
      //!
      //! \return The job to do.
      //!
      JobData nextJob();

      //! Submits a result to the worker. Call this function
      //! after requesting a job with job().
      //!
      //! \param result The computed result.
      //!
      void submitResult(const ResultData& result);

    private:
      bool firstMessage_ = true;
      Address requesterAddress_;
      std::string jobTypeString_;
      zmq::socket_t socket_;
      std::string jobIDString_;
    };
  } // namespace comms
} // namespace stateline

