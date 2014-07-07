//!
//! Async policy for job multiplexing on server side
//!
//! \file app/asyncdelegator.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "app/asyncdelegator.hpp"
#include "serial/serial.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "serial/serial.hpp"
#include "datatype/sensors.hpp"

namespace obsidian
{
  //! Sends new job.
  //! Consists of world params and sensor params.
  //!
  template<ForwardModel f>
  struct AsyncSend
  {
    AsyncSend(const std::string &worldParams, std::vector<stateline::comms::JobData> &j)
    {
      typename Types<f>::Params param;
      param.returnSensorData = false; // false atm. maybe some day for some use case, we might want to set this to true
      j.push_back(comms::serialiseJob<f>(param, worldParams));
    }
  };

  //! Retrives results of a job
  //!
  template<ForwardModel f>
  struct AsyncRetrieve
  {
    AsyncRetrieve(const std::vector<stateline::comms::ResultData> &results, std::vector<double> &lh, uint &sensorId)
    {
      typename Types<f>::Results res;
      comms::unserialiseResult<f>(results[sensorId++], res);
      lh.push_back(res.likelihood);
    }
  };

  GeoAsyncPolicy::GeoAsyncPolicy(stateline::comms::Delegator &delegator, const GlobalPrior& prior,
                                 const std::set<ForwardModel> &sensorsEnabled)
      : req_(delegator), prior_(prior), sensorsEnabled_(sensorsEnabled)
  {
  }

  void GeoAsyncPolicy::submit(uint id, const Eigen::VectorXd &theta)
  {
    GlobalParams params = prior_.reconstruct(theta);
    std::string globalData = comms::serialise(params.world);

    priorValues_[id] = prior_.evaluate(theta);

    if (!is_neg_infinity(priorValues_[id])) // Within acceptable bounds
    {
      std::vector<stateline::comms::JobData> jobs;
      applyToSensorsEnabled<AsyncSend>(sensorsEnabled_, globalData, std::ref(jobs));
      req_.batchSubmit(id, jobs);
    } else // outside bounds; no point sending work to shards; we already know the outcome: likelihood = -infinity
    {
      zeroSet_.push(id);
    }
  }

  std::pair<uint, double> GeoAsyncPolicy::retrieve()
  {
    if (zeroSet_.size() > 0) // out of bounds
    {
      auto val = std::make_pair(zeroSet_.front(), -std::numeric_limits<double>::infinity());
      zeroSet_.pop();
      return val;
    }

    // collate likelihood
    auto msg = req_.batchRetrieve();
    uint id = msg.first;
    std::vector<stateline::comms::ResultData> results = msg.second;

    std::vector<double> logLikelihoods;
    uint sensorId = 0;
    applyToSensorsEnabled<AsyncRetrieve>(sensorsEnabled_, std::cref(results), std::ref(logLikelihoods), std::ref(sensorId));
    double logLikelihood = std::accumulate(logLikelihoods.begin(), logLikelihoods.end(), 0.0) + priorValues_[id];
    double negLogLikelihood = -1.0 * logLikelihood;
    return std::make_pair(id, negLogLikelihood);
  }
} // namespace obsidian
