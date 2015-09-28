#pragma once
//!
//! JSON Settings reader
//!
//! 
//!
//! \file app/jsonsettings.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include <json.hpp>
#include <string>
#include <easylogging/easylogging++.h>

namespace stateline
{

  template <class T>
  T readSettings(const nlohmann::json& j, const std::string& field)
  {
    try
    {
      T val = j[field].get<T>();
      return val;
    }
    catch(...)
    {
      LOG(ERROR)  << field << " not found in config file and no default value exists. Exiting.";
      exit(EXIT_FAILURE);
    }
  }
  
  template <class T>
  T readSettings(const nlohmann::json& j, const std::string& field1, const std::string& field2)
  {
    try
    {
      T val = j[field1][field2].get<T>();
      return val;
    }
    catch(...)
    {
      LOG(ERROR) << field1 << ":" << field2 << " not found in config file and no default value exists. Exiting.";
      exit(EXIT_FAILURE);
    }
  }
  
  template <class T>
  T readWithDefault(const nlohmann::json& j, const std::string& field, const T& def)
  {
    if (j.count(field)) 
      return j[field].get<T>();
    else
      return def;
  }
  
  template <class T>
  T readWithDefault(const nlohmann::json& j, const std::string& field1 , const std::string& field2, const T& def)
  {
    if (j.count(field1) && j[field1].count(field2)) 
      return j[field1][field2].get<T>();
    else
      return def;
  }
  
  template <class T>
  T readWithDefault(const nlohmann::json& j, const std::string& field1 , const std::string& field2,  const std::string& field3, const T& def)
  {
    if (j.count(field1) && j[field1].count(field2) && j[field1][field2].count(field3)) 
      return j[field1][field2][field3].get<T>();
    else
      return def;
  }

} // namespace stateline
