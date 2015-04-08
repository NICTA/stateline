//!
//! Contains common serialisation code used by comms.
//!
//! \file comms/serial.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA

#pragma once

#include <cstdint>
#include <vector>

namespace stateline
{
  namespace comms
  {
    namespace detail
    {
      template <typename T, typename U>
      std::string serialise(const U &value)
      {
        T v = static_cast<T>(value);
        return std::string((char *)&v, sizeof(T));
      }

      template <typename T, typename U>
      std::string serialise(const std::vector<U> &value)
      {
        // We need to be careful here. We have to first convert each element
        // of the vector into the designated type before serialising.
        std::string buffer;
        buffer.reserve(value.size() * sizeof(U));

        for (const U &u : value) {
          buffer.append(serialise<T>(u));
        }

        return buffer;
      }

      template <typename T>
      T unserialise(const std::string &s)
      {
        return *((T *)&s[0]);
      }

      template <typename T, typename U>
      void unserialise(const std::string &s, std::vector<U> &values)
      {
        std::size_t numElements = s.length() / sizeof(T);
        values.reserve(numElements);

        // Add each element to the vector
        T *raw = (T *)&s[0];
        for (std::size_t i = 0; i < numElements; i++)
        {
          values.push_back(static_cast<U>(raw[i]));
        }
      }
    }
  }
}
