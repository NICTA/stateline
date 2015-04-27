//!
//! Object which actually requests work and returns a result. Many of these can
//! live in the same executable, but only 1 per thread. They forward requests to
//! a shared (threadsafe) delegator object using zeromq inproc messaging
//!
//! \file comms/requester.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <Eigen/Eigen>

#include "comms/datatypes.hpp"
#include "comms/messages.hpp"
#include "comms/socket.hpp"

namespace stateline
{
  namespace comms
  {
    //! Requester object that takes jobs and returns results. Communicates with
    //! a delegator in a (possibly) different thread.
    //!
    class Requester
    {
    public:

      //! Create a new Requester.
      //!
      //! \param d A reference to the delegator object to communicate with
      //!
      Requester(zmq::context_t& context);

      //! Submits a batch of jobs for computation and immediately returns. An id is
      //! included to allow the batch to be identified later, because when batches
      //! are retrieved they may not arrive in the order they were submitted.
      //!
      //! \param id The id of the batch
      //! \param jobs The vector of jobs to compute
      //! \return The results of the job computations
      //!
      void submit(uint id, const std::vector<std::string>& jobTypes, const Eigen::VectorXd& data);

      //! Retrieves a batch of jobs that have previously been submitted for computation.
      //! A pair is returned, with the id of the batch (from the submit call),
      //! and the results. Note that batch may not be retrieved in the order
      //! they were submitted.
      //!
      //! \returns A pair of the job id and the result
      //!
      std::pair<uint, std::vector<double>> retrieve();

    private:
      // Communicates with another inproc socket in the delegator
      Socket socket_;
    };
  } // namespace comms
} // namespace stateline

