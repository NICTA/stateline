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

#include "common/logging.hpp"
#include "comms/endpoint.hpp"
#include "comms/router.hpp"
#include "comms/protocol.hpp"

#include <queue>

namespace stateline { namespace comms {

Agent::State::State(Socket& worker, Socket& network)
  : worker{worker}
  , network{network}
  , workerWaiting{true}
{
}

struct Agent::WorkerEndpoint : Endpoint<WorkerEndpoint>
{
  Agent::State& agent;
  std::chrono::seconds timeout;

  WorkerEndpoint(Agent::State& agent, std::chrono::seconds timeout)
    : Endpoint<WorkerEndpoint>{agent.worker}
    , agent{agent}
    , timeout{timeout}
  {
  }

  void onHello(const Message& m)
  {
    // Worker HELLOs do not have heartbeat timeout information.
    // It is our job to append that to the message.
    Message copy = m;
    Packer p{copy.data};
    p.reserve(m.data.size() + 4);
    p.value(static_cast<std::uint32_t>(timeout.count()));

    forwardMessage(agent.network, copy);
  }

  void onResult(const Message& m)
  {
    forwardMessage(agent.network, m);

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

struct Agent::NetworkEndpoint : Endpoint<NetworkEndpoint>
{
  Agent::State& agent;

  NetworkEndpoint(Agent::State& agent)
    : Endpoint<NetworkEndpoint>{agent.network}
    , agent{agent}
  {
  }

  void onWelcome(const Message& m)
  {
    const auto welcome = protocol::unserialise<protocol::Welcome>(m.data);
    agent.network.startHeartbeats(m.address, std::chrono::seconds{welcome.hbTimeoutSecs});
  }

  void onJob(const Message& m)
  {
    if (agent.workerWaiting)
    {
      forwardMessage(agent.worker, m);
      agent.workerWaiting = false;
    }
    else
    {
      agent.queue.push(m);
    }
  }

  void onBye(const Message& m)
  {
    // TODO stop!
  }
};

Agent::Agent(zmq::context_t& ctx, const AgentSettings& settings)
  : settings_{settings}
  , worker_{ctx, zmq::socket_type::rep, "toWorker"}
  , network_{ctx, zmq::socket_type::dealer, "toNetwork"}
  , state_{worker_, network_}
{
  // Initialise the local sockets
  LOG(INFO) << "Agent binding to " << settings.bindAddress;
  worker_.bind(settings.bindAddress);

  LOG(INFO) << "Agent connecting to delegator at " << settings.networkAddress;
  network_.setIdentity();
  network_.connect(settings.networkAddress);
}

void Agent::poll()
{
  bool running = false; // Poll only once
  start(running);
}

void Agent::start(bool& running)
{
  WorkerEndpoint worker{state_, settings_.heartbeatTimeout};
  NetworkEndpoint network{state_};

  Router<WorkerEndpoint, NetworkEndpoint> router{"agent", std::tie(worker, network)};

  const auto onIdle = [&network]() { network.idle(); };

  do
  {
    router.poll(onIdle);
  } while (running);
}

} }
