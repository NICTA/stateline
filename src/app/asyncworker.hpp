//!
//! Async policy for job multiplexing on worker side.
//!
//! \file app/asyncworker.hpp
//! \author Lachlan McCalman
//! \author Nahid Akbar
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "datatype/datatypes.hpp"
#include "comms/worker.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "comms/minion.hpp"
#include "fwdmodel/fwd.hpp"
#include "likelihood/likelihood.hpp"
#include "serial/serial.hpp"

namespace obsidian
{
  //! Thread method receives jobs and evaluates likelihoods and sends results back until until interrupted by signal.
  //!
  template<ForwardModel f>
  bool workerThread(const typename Types<f>::Spec& spec, const typename Types<f>::Cache& cache, const typename Types<f>::Results& real,
                    stateline::comms::Worker& worker)
  {
    stateline::comms::Minion minion(worker, (uint) f); // comms uses uints for jobs ID
    auto tLastLogged = hrc::now();
    bool haveLogged = false;
    double averageTime = 0;
    uint loopCount = 0;
    while (!global::interruptedBySignal)
    {
      // Get the next job
      auto job = minion.nextJob();
      WorldParams worldParams;
      comms::unserialise(job.globalData, worldParams);
      typename Types<f>::Params params;
      comms::unserialise(job.jobData, params);
      // Make the results
      typename Types<f>::Results result;
      auto tStart = hrc::now();
      typename Types<f>::Results synthetic = fwd::forwardModel<f>(spec, cache, worldParams);
      auto tEnd = hrc::now();
      if (params.returnSensorData)
        result = synthetic;
      result.likelihood = lh::likelihood<f>(synthetic, real, spec);
      CHECK(!std::isnan(result.likelihood)) << f << " numerical error";
      // Submit the results
      // Use a uint for the job ID
      minion.submitResult( { (uint) f, comms::serialise(result) });

      // calculate average time
      uint deltaMuS = std::chrono::duration_cast < std::chrono::microseconds > (tEnd - tStart).count();
      averageTime = (averageTime * (loopCount / double(loopCount + 1))) + deltaMuS / double(loopCount + 1);
      loopCount++;
      // check for logging
      uint deltaLog = std::chrono::duration_cast < std::chrono::seconds > (hrc::now() - tLastLogged).count();
      if (deltaLog % 2 == 0)
      {
        if (!haveLogged)
        {
          VLOG(1) << f << " average time:" << averageTime / 1000.0 << "ms";
          tLastLogged = hrc::now();
          loopCount = 0;
          averageTime = 0;
        }
        haveLogged = true;
      } else
      {
        haveLogged = false;
      }
    }
    return true;
  }

} // namespace obsidian
