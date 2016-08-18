//!
//! Signal handler.
//!
//! \file signal.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// DO NOT USE
// C++ std::signal causes undefined behaviour in multithreaded programs
// http://en.cppreference.com/w/cpp/utility/program/signal

/*
#include <atomic>

namespace stateline {

// TODO use atomic
void initialiseSignalHandler();

bool isRunning();

}*/
