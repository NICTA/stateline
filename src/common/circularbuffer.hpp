//!
//! Simple circular buffer.
//!
//! \file comms/circularbuffer.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <deque>

namespace stateline
{

namespace comms
{

template <class T>
class CircularBuffer
{
public:
  using size_type = typename std::deque<T>::size_type;
  using iterator = typename std::deque<T>::iterator;
  using const_iterator = typename std::deque<T>::const_iterator;

  explicit CircularBuffer(size_type size)
    : buffer_(size), size_(size)
  {
  }

  void push_back(T val)
  {
    buffer_.push_back(val);
    if (buffer_.size() > size_)
      buffer_.pop_front();
  }

  iterator begin() { return buffer_.begin(); }
  iterator end() { return buffer_.end(); }
  const_iterator begin() const { return buffer_.begin(); }
  const_iterator end() const { return buffer_.end(); }
  size_type size() const { return size_; }

private:
  std::deque<T> buffer_;
  size_type size_;
};

}

}
