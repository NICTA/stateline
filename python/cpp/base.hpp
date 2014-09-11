#pragma once

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Eigen/Dense>

#include <Python.h>
#include <boost/python.hpp>
#include <numpy/arrayobject.h>

#include <map>

namespace py = boost::python;

namespace stateline
{
}

using namespace stateline;

template <class T>
std::vector<T> list2vector(const py::list &list)
{
  std::vector<T> vector;
  for (int i = 0; i < py::len(list); i++)
  {
    vector.push_back(py::extract<T>(list[i]));
  }

  return vector;
}

template <class T>
py::list vector2list(const std::vector<T> &vector)
{
  py::list list;
  for (auto it = vector.begin(); it != vector.end(); ++it)
  {
    list.append(*it);
  }

  return list;
}

template <class Key, class Value>
std::map<Key, Value> dict2map(const py::dict &dict)
{
  std::map<Key, Value> map;
  py::list keys = dict.keys();
  for (int i = 0; i < py::len(keys); i++)
  {
    py::extract<Key> key(keys[i]);
    py::extract<Value> value(dict[(Key)key]);

    map[key] = value;
  }

  return map;
}

template <class Key, class Value>
py::dict map2dict(const std::map<Key, Value> &map)
{
  py::dict dict;
  for (const auto &it : map)
    dict[it.first] = it.second;
  return map;
}

py::object string2bytes(const std::string &str)
{
  return py::object(py::handle<>(PyBytes_FromStringAndSize(str.c_str(), str.length())));
}

py::object eigen2numpy(const Eigen::VectorXd &vec)
{
  npy_intp size = vec.size();

  double *data = const_cast<double *>(&vec[0]);
  PyObject *array = PyArray_SimpleNewFromData(1, &size, NPY_DOUBLE, data);

  return py::numeric::array(py::handle<>(array)).copy();
}

py::object veigen2lnumpy(const std::vector<Eigen::VectorXd> &vvec)
{
  std::vector<py::object> vec;
  for (const auto &x : vvec)
    vec.push_back(eigen2numpy(x));
  return vector2list(vec);
}

Eigen::VectorXd numpy2eigen(py::object x)
{
  PyArrayObject *ptr = (PyArrayObject *)x.ptr();

  if (!PyArray_ISFLOAT(ptr))
    throw std::invalid_argument("PyObject is not an array of floats/doubles!");

  return Eigen::Map<Eigen::VectorXd>((double *)PyArray_DATA(ptr),
      PyArray_SHAPE(ptr)[0]);
}

std::vector<Eigen::VectorXd> lnumpy2veigen(py::object x)
{
  std::vector<Eigen::VectorXd> list;
  for (int i = 0; i < py::len(x); i++)
    list.push_back(numpy2eigen(x[i]));
  return list;
}
