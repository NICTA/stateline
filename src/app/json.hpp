//! Helpers to work with JSON.
//!
//! \file app/json.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#pragma once

#include <json.hpp>
#include <string>
#include <easylogging/easylogging++.h>

namespace stateline {

template <class T>
void readFields(const nlohmann::json& j, T& val)
{
  val = j.get<T>();
}

template <class T, class Field, class... Fields>
void readFields(const nlohmann::json& j, Field&& field, Fields&&... fields, T& val)
{
  if (j.count(field))
  {
    readFields(j[field], std::forward<Fields>(fields)..., val);
  }
  else
  {
    LOG(ERROR) << field << " not found in config file and no default value exists. Exiting.";
    exit(EXIT_FAILURE);
  }
}

template <class T, class S>
void readFieldsWithDefault(const nlohmann::json& j, T& val, const S&)
{
  val = j.get<T>();
}

template <class T, class S, class Field, class... Fields>
void readFieldsWithDefault(const nlohmann::json& j, Field&& field, Fields&&... fields, T& val, const S& def)
{
  if (j.count(field))
    readFieldsWithDefault(j[field], std::forward<Fields>(fields)..., val, def);
  else
    val = def;
}

}
