//!
//! Contains global types which is used for MCMC.
//!
//! \file datatype/datatypes.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

namespace obsidian
{
  #if __APPLE__ && __MACH__
    // Alias unsigned int as uint for convenience
    typedef unsigned int uint;
  #endif
} // namespace obsidian
