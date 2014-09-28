#include "base.hpp"
#include "jobdata.hpp"
#include "resultdata.hpp"
#include "state.hpp"
#include "workerinterface.hpp"
#include "settings.hpp"
#include "worker.hpp"
#include "minion.hpp"
#include "chainarray.hpp"
#include "sampler.hpp"
#include "proposal.hpp"
#include "adaptive.hpp"
#include "logging.hpp"
#include "diagnostics.hpp"
#include "app/logging.hpp"

#if PY_MAJOR_VERSION == 2
static void wrap_import_array() {
  import_array();
}
#else
static void * wrap_import_array() {
  import_array();
}
#endif

BOOST_PYTHON_MODULE(_stateline)
{
  wrap_import_array();
  py::numeric::array::set_module_and_type("numpy", "ndarray");

  exportJobData();
  exportResultData();
  exportSwapType();
  exportState();
  exportWorkerInterface();
  exportSettings();
  exportWorker();
  exportMinion();
  exportChainSettings();
  exportChainArray();
  exportSampler();
  exportGaussianProposal();
  exportSlidingWindowSigmaSettings();
  exportSlidingWindowBetaSettings();
  exportSlidingWindowSigmaAdapter();
  exportSlidingWindowBetaAdapter();
  exportTableLogger();
  exportEPSRDiagnostic();

  py::def("init_logging", initLogging);
}
