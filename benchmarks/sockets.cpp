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

  std::vector<std::string> data = { std::string(state.range_x(), ' ') };

  while (state.KeepRunning())
  {
    req.send({ comms::WORK, data });
    rep.receive();
    rep.send({ comms::WORK, data });
    req.receive();
  }

  state.SetBytesProcessed(int64_t(state.iterations()) * data[0].size());
  state.SetItemsProcessed(int64_t(state.iterations()) * 2);
}

// 1B to 1MB
BENCHMARK(BM_SocketReqRepSendRecv)->Range(1, 1e6);

BENCHMARK_MAIN()
