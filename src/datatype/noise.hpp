//!
//! Contains the GDF datatype for specifiying sensor noise.
//!
//! \file datatype/noise.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

namespace obsidian
{
  struct NoiseSpec
  {
    double inverseGammaAlpha;
    double inverseGammaBeta;
  };

} // namespace obsidian

