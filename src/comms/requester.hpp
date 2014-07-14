//!
//! Object which actually requests work and returns a result. Many of these can
//! live in the same executable, but only 1 per thread. They forward requests to
//! a shared (threadsafe) delegator object using zeromq inproc messaging
//!
//! \file comms/requester.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <glog/logging.h>
#include <zmq.hpp>

#include "comms/datatypes.hpp"
#include "comms/messages.hpp"
#include "comms/transport.hpp"
#include "comms/delegator.hpp"

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
      Requester(Delegator& d);

      //! Submits a job for computation and immediately returns. An id is
      //! included to allow the job to be identified later, because when jobs
      //! are retrieved they may not arrive in the order they were submitted.
      //!
      //! \warning Do not interleave different types of job call pairs and blocking
      //!          calls. This will break horribly. For example, don't call submit
      //!          in between batchsubmit and batch retrieve.
      //!
      //! \param id The job ID.
      //! \param j The job to compute.
      //!
      void submit(JobID id, const JobData& j);

      void submit(JobID id, JobData&& j);

      //! Retrieves a job that has previously been submitted for computation.
      //! A pair is returned, with the id of the job (from the submit call),
      //! and the result. Note that jobs may not be retrieved in the order
      //! they were submitted
      //!
      //! \warning Do not interleave different types of job call pairs and blocking
      //!          calls. This will break horribly. For example, don't call submit
      //!          in between batchsubmit and batch retrieve.
      //!
      //! \returns A pair of the job id and the result
      //!
      std::pair<uint, ResultData> retrieve();

      //! Submits a batch of jobs for computation and immediately returns. An id is
      //! included to allow the batch to be identified later, because when batches
      //! are retrieved they may not arrive in the order they were submitted.
      //!
      //! \warning Do not interleave different types of job call pairs and blocking
      //!          calls. This will break horribly. For example, don't call submit
      //!          in between batchsubmit and batch retrieve.
      //!
      //! \param id The id of the batch
      //! \param jobs The vector of jobs to compute
      //! \return The results of the job computations
      //!
      void batchSubmit(JobID id, const std::vector<JobData>& jobs);

      void batchSubmit(JobID id, std::vector<JobData>&& jobs);

      //! Retrieves a batch of jobs that have previously been submitted for computation.
      //! A pair is returned, with the id of the batch (from the submit call),
      //! and the results. Note that batch may not be retrieved in the order
      //! they were submitted.
      //!
      //! \warning Do not interleave different types of job call pairs and blocking
      //!          calls. This will break horribly. For example, don't call submit
      //!          in between batchsubmit and batch retrieve.
      //!
      //! \returns A pair of the job id and the result
      //!
      std::pair<uint, std::vector<ResultData>> batchRetrieve();

    private:
      // Communicates with another inproc socket in the delegator
      zmq::socket_t socket_;

      // Used to keep track of the batch submissions
      std::map<uint, std::vector<ResultData>> batches_;
      std::map<uint, uint> batchLeft_;
    };
  } // namespace comms
} // namespace obsidian

