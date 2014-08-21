#pragma once

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
    npy_intp shape[] = { vec.size() };
    PyArrayObject *array = (PyArrayObject *)PyArray_SimpleNew(
        1, shape, NPY_DOUBLE
    );

    double *dest = (double *)PyArray_DATA(array);
    memcpy(dest, vec.data(), sizeof(double) * vec.size());

    return py::object(py::handle<>(array));
}


