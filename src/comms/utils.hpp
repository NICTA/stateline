//! Utilities for comms.
//!
//! \file comms/utils.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

namespace stateline { namespace comms {

template <class T>
class ExpMovingAverage
{
public:
  ExpMovingAverage(float alpha)
    : alpha_{alpha}
    , avg_{0}
  {
    assert(alpha >= 0.0 && alpha <= 1.0);
  }

  T average() const { return avg_; }

  void add(const T& val)
  {
    avg_ = (alpha_ * val) + (1.0 - alpha_) * avg_;
  }

private:
  float alpha_;
  T avg_;
};

} }
