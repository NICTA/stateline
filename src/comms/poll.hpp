//! Provides an interface for polling endpoints.
//!
//! \file comms/poll.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "common/meta.hpp"

#include <array>
#include <chrono>

namespace stateline
{

namespace comms
{

namespace detail
{

template <class Endpoint>
zmq::pollitem_t getPollItem(Endpoint& endpoint)
{
  return {(void *)endpoint.zmqSocket(), 0, ZMQ_POLLIN, 0};
}

template <class T, int... Is>
std::array<zmq::pollitem_t, sizeof...(Is)> toPollListImpl(
    T&& t, meta::seq<Is...>)
{
  return { { getPollItem(std::get<Is>(t))... } };
}

template <class... Endpoints>
std::array<zmq::pollitem_t, sizeof...(Endpoints)> toPollList(
    const std::tuple<Endpoints&...>& endpoints)
{
  return toPollListImpl(endpoints, meta::gen_seq<sizeof...(Endpoints)>());
}

}

//! Poll a set of endpoints.
//!
template <class... Endpoints>
std::size_t poll(std::tuple<Endpoints&...> endpoints,
    std::chrono::milliseconds wait)
{
  auto pollList = detail::toPollList(endpoints);

  // Block until a message arrives or timeout
  zmq::poll(pollList.data(), pollList.size(), wait);

  // Figure out which socket it's from
  std::size_t msgCount = 0;
  for (std::size_t i = 0; i < pollList.size(); i++)
  {
    bool newMsg = pollList[i].revents & ZMQ_POLLIN;
    if (newMsg)
    {
      meta::apply<void>(endpoints, i, [](auto& e) { return e.accept(); });
      msgCount++;
    }
  }

  return msgCount;
}

} // namespace stateline

}// namespace comms
