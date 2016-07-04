//!
//! String algorithms.
//!
//! \file common/string.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

namespace stateline
{

template <class Container>
std::string joinStr(const Container& container, const std::string& delim)
{
  std::string result;
  for (const auto& v : container)
  {
    result += v + delim;
  }
  result.pop_back(); // Remove last delimiter
  return result;
}

template <class OutputContainer>
void splitStr(OutputContainer& out, const std::string& str, char delim)
{
  std::string segment;
  for (const auto& c : str)
  {
    if (c == delim)
    {
      out.insert(std::end(out), segment);
      segment = "";
    }
    else
    {
      segment += c;
    }
  }

  if (segment != "")
    out.insert(std::end(out), segment);
}

}
