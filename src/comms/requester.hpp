//! Interface for requesters.
//!
//! \file comms/requester.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "datatypes.hpp"
#include "socket.hpp"

namespace stateline { namespace comms {

//! Requester object that takes jobs and returns results. Communicates with
//! a delegator in a (possibly) different thread.
//!
class Requester
{
public:

  //! Construct a new Requester.
  //!
  //! \param ctx ZMQ context used to communicate with the delegator.
  //! \param addr Address of the delegator.
  //!
  Requester(zmq::context_t& ctx, const std::string& addr);

  //! Submits a batch of jobs for computation and immediately returns. An id is
  //! included to allow the batch to be identified later, because when batches
  //! are retrieved they may not arrive in the order they were submitted.
  //!
  //! \param id The id of the batch
  //! \param jobTypes The vector of jobs to compute
  //! \param data The data shared between all the jobs
  //!
  void submit(BatchID id, const std::vector<JobType>& jobTypes,
      const std::vector<double>& data);

  //! Retrieves the results of a batch submitted previously.
  //! This call blocks until the result of a batch is available. The order of the
  //! results within the batch corresponds to the order of the submitted job types.
  //!
  //! \returns A pair of the job id and the result.
  //!
  std::pair<BatchID, std::vector<double>> retrieve();

private:
  // Communicates with another inproc socket in the delegator
  Socket socket_;
};

} }

