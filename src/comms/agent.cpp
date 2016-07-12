//! Contains the implementation of the agent.
//!
//! \file comms/agent.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/agent.hpp"

#include "comms/endpoint.hpp"
#include "comms/protobuf.hpp"
#include "comms/router.hpp"

#include <queue>

#include <easylogging/easylogging++.h>

namespace stateline { namespace comms {

namespace {

struct AgentState
{
  Socket& worker;
  Socket& network;
  std::queue<Message> queue;
  bool workerWaiting;

  AgentState(Socket& worker, Socket& network)
    : worker{worker}
    , network{network}
    , workerWaiting{false}
  {
  }
};

struct WorkerEndpoint : Endpoint<WorkerEndpoint>
{
  AgentState& agent;
  std::chrono::seconds timeout;

  WorkerEndpoint(AgentState& agent, std::chrono::seconds timeout)
    : Endpoint<WorkerEndpoint>{agent.worker}
    , agent{agent}
    , timeout{timeout}
  {
  }

  void onHello(const Message& m)
  {
    messages::Hello hello;
    hello.set_hb_timeout_secs(timeout.count());
    agent.network.send({"", HELLO, protobufToString(hello)});
  }

  void onResult(const Message& m)
  {
    agent.network.send(m); // Forward result to delegator

    if (agent.queue.empty())
    {
      agent.workerWaiting = true;
    }
    else
    {
      agent.worker.send(std::move(agent.queue.front()));
      agent.queue.pop();
    }
  }
};

struct NetworkEndpoint : Endpoint<NetworkEndpoint>
{
  AgentState& agent;

  NetworkEndpoint(AgentState& agent)
    : Endpoint<NetworkEndpoint>{agent.network}
    , agent{agent}
  {
  }

  void onWelcome(const Message& m)
  {
    const auto welcome = stringToProtobuf<messages::Welcome>(m.data);
    agent.network.startHeartbeats(m.address, std::chrono::seconds{welcome.hb_timeout_secs()});
  }

  void onJob(const Message& m)
  {
    if (agent.workerWaiting)
      agent.worker.send(m); // Forward job to worker
    else
      agent.queue.push(m);
  }

  void onBye(const Message& m)
  {
    // TODO stop!
  }
};

}

Agent::Agent(zmq::context_t& ctx, const AgentSettings& settings)
  : settings_{settings}
  , worker_{ctx, zmq::socket_type::rep, "toWorker"}
  , network_{ctx, zmq::socket_type::dealer, "toNetwork"}
{
  // Initialise the local sockets
  worker_.bind(settings.bindAddress);
  network_.setIdentity();
  LOG(INFO) << "Worker connecting to " << settings.networkAddress;
  network_.connect(settings.networkAddress);
}

void Agent::poll()
{
  bool running = false; // Poll only once
  start(running);
}

void Agent::start(bool& running)
{
  AgentState agent{worker_, network_};
  WorkerEndpoint worker{agent, settings_.heartbeatTimeout};
  NetworkEndpoint network{agent};

  Router<WorkerEndpoint, NetworkEndpoint> router{"agent", std::tie(worker, network)};

  const auto onIdle = [&network]()
  {
    // TODO: Ugly.
    network.socket().heartbeats().idle();
  };

  do
  {
    router.poll(onIdle);
  } while (running);
}

} }
