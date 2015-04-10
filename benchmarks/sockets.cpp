#include <benchmark/benchmark_api.h>

#include "comms/socket.hpp"

using namespace stateline;

static void BM_SocketReqRepSendRecv(benchmark::State& state)
{
  zmq::context_t ctx;

  comms::Socket rep(ctx, ZMQ_REP, "rep");
  rep.bind("tcp://*:5556");

  comms::Socket req(ctx, ZMQ_REQ, "req");
  req.connect("tcp://localhost:5556");

  std::vector<std::string> data = { "data" };

  while (state.KeepRunning())
  {
    req.send({ comms::WORK, data });
    rep.receive();
    rep.send({ comms::WORK, data });
    req.receive();
  }
}

BENCHMARK(BM_SocketReqRepSendRecv);

BENCHMARK_MAIN()
