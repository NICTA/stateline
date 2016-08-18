//! Common interface to logging independent of the log system we use.
//!
//! \file src/comms/logging.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!
#pragma once

#include <sstream>

#include <easylogging/easylogging++.h>

namespace stateline {

#define SL_LOG(level) LOG(level)
#define SL_CLOG(level, logger) CLOG(level, logger)

namespace detail {

inline void pprintVal(std::ostringstream& s, const std::string& val)
{
  // Print string wrapped in quotes
  s << '"' << val << '"';
}

template <class T>
void pprintVal(std::ostringstream& s, const T& val)
{
  // Use the stringstream default format
  s << val;
}

template <class T1, class T2>
void pprintVal(std::ostringstream& s, const std::pair<T1, T2>& val)
{
  s << "(";
  pprintVal(s, val.first);
  s << ", ";
  pprintVal(s, val.second);
}

inline void pprintVal(std::ostringstream& s, const std::chrono::milliseconds& val)
{
  pprintVal(s, val.count());
  s << "ms";
}

inline void pprintVal(std::ostringstream& s, const std::chrono::seconds& val)
{
  pprintVal(s, val.count());
  s << "s";
}

template <bool First>
inline void pprintImpl(std::ostringstream&)
{
  // Base case
}

template <bool First, class T, class... Args>
void pprintImpl(std::ostringstream& s, const std::string& key, const T& val, Args&&... args)
{
  if (!First)
    s << ", ";
  s << key << "=";

  pprintVal(s, val);
  pprintImpl<false>(s, std::forward<Args>(args)...);
}

}

template <class... Args>
std::string pprint(Args&&... args)
{
#ifdef NDEBUG
  return "";
#else
  std::ostringstream ss;
  ss << "[";
  detail::pprintImpl<true>(ss, std::forward<Args>(args)...);
  ss << "]";
  return ss.str();
#endif
}

}
