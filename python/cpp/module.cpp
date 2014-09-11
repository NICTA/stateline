#include <Python.h>
#include "numpy/arrayobject.h"

#include "base.hpp"
#include "jobdata.hpp"
#include "resultdata.hpp"
#include "state.hpp"
#include "workerinterface.hpp"
#include "settings.hpp"
#include "worker.hpp"
#include "minion.hpp"
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

void numpyTest(const py::object &obj)
{
  /*
  npy_intp size = 3;
  double *data = new double[20];

  std::cout << "HELLO " << std::endl;
  PyObject *array = PyArray_SimpleNewFromData(1, &size, NPY_DOUBLE, data);
  std::cout << "HELLO " << std::endl;

  std::string object_classname = boost::python::extract<std::string>(obj.attr("__class__").attr("__name__"));
  std::cout<<"this is an Object: "<<object_classname<<std::endl;

  PyArrayObject *p = (PyArrayObject *)obj.ptr();
  std::cout << "NDIMS: " << PyArray_NDIM(p) << std::endl;

  std::cout << "SHAPE: ";
  npy_intp *shape = PyArray_SHAPE(p);
  for (int i = 0; i < PyArray_NDIM(p); ++i)
    std::cout << shape[i];
  std::cout << std::endl;

  double *data = (double *)PyArray_DATA(p);
  std::cout << data[0] << " " << data[1] << " " << data[2] << std::endl;

  Eigen::VectorXd x = numpy2eigen(obj);
  std::cout << x << std::endl;*/
}

BOOST_PYTHON_MODULE(_stateline)
{
  wrap_import_array();
  py::numeric::array::set_module_and_type("numpy", "ndarray");

  exportJobData();
  exportResultData();
  exportState();
  exportWorkerInterface();
  exportSettings();
  exportWorker();
  exportMinion();
  py::def("numpy_test", numpyTest);
  py::def("init_logging", initLogging);
}
