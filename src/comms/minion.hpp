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
#include "comms/worker.hpp"
#include "comms/socket.hpp"

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
      //! \param jobTypes The job types that the minion will do
      //!
      Minion(zmq::context_t& context, const std::vector<std::string>& jobTypes);

      //! Gets a job from the worker.
      //!
      //! \return The job to do.
      //!
      std::pair<std::string, std::string> nextJob();

      //! Submits a result to the worker. Call this function
      //! after requesting a job with job().
      //!
      //! \param result The computed result.
      //!
      void submitResult(const std::string& result);

    private:
      Socket socket_;
      std::string currentJob_;
    };
  } // namespace comms
} // namespace stateline

