#include <kv/client.hpp>
#include <kv/main.hpp>
#include <rsm/proxy/main.hpp>

// Node
#include <whirl/node/runtime/shortcuts.hpp>

// Serialization
#include <muesli/serializable.hpp>
// Support std::string serialization
#include <cereal/types/string.hpp>

// Logging
#include <timber/log.hpp>

// Concurrency
#include <await/fibers/core/api.hpp>
#include <await/fibers/sync/future.hpp>

// Simulation
#include <matrix/facade/world.hpp>
#include <matrix/world/global/vars.hpp>
#include <matrix/world/global/time.hpp>
#include <matrix/client/rpc.hpp>
#include <matrix/test/random.hpp>
#include <matrix/test/main.hpp>
#include <matrix/test/event_log.hpp>
#include <matrix/test/runner.hpp>

#include <matrix/fault/access.hpp>
#include <matrix/fault/net/split.hpp>
#include <matrix/fault/util.hpp>

#include <matrix/semantics/printers/print.hpp>

#include <commute/rpc/id.hpp>
#include <commute/rpc/wire.hpp>

#include <muesli/serialize.hpp>

#include <algorithm>

#include <tests/time_models/async_1.hpp>

#include "../common/atomic_counter.hpp"

using namespace whirl;

//////////////////////////////////////////////////////////////////////

void Client() {
  await::fibers::self::SetName("main");

  node::rt::SleepFor(123_jfs);

  // + Random delay
  node::rt::SleepFor({node::rt::RandomNumber(50, 100)});

  timber::Logger logger_{"Client", node::rt::LoggerBackend()};

  auto channel = matrix::client::MakeRpcChannel("proxy", 42);

  kv::Client kv_client{channel};

  tests::AtomicCounter counter(kv_client, "counter");

  size_t increments_to_do = matrix::GetGlobal<size_t>("increments_per_client");

  for (size_t i = 0; i < increments_to_do; ++i) {
    size_t prev_value = counter.FetchAdd(1);

    matrix::GlobalCounter("increments").Increment();
    matrix::GlobalCounter("total").Add(prev_value);

    node::rt::SleepFor(node::rt::RandomNumber(1, 100));
  }
}

//////////////////////////////////////////////////////////////////////

static const matrix::TimePoint kNoMoreFaults = 50000;

//////////////////////////////////////////////////////////////////////

void NetAdversary() {
  timber::Logger logger_{"Net-Adversary", node::rt::LoggerBackend()};

  // List system nodes
  auto pool = node::rt::Discovery()->ListPool("rsm");

  auto& net = matrix::fault::Network();

  while (matrix::GlobalNow() < kNoMoreFaults) {
    node::rt::SleepFor(node::rt::RandomNumber(10, 750));

    size_t lhs_size = node::rt::RandomNumber(1, pool.size() - 1);
    LOG_INFO("Random split: {}/{}", lhs_size, pool.size() - lhs_size);
    matrix::fault::RandomSplit(pool, lhs_size);

    matrix::fault::RandomPause(100_jfs, 500_jfs);

    net.Heal();
  }
}

//////////////////////////////////////////////////////////////////////

void NodeAdversary() {
  timber::Logger logger_{"Node-Adversary", node::rt::LoggerBackend()};

  // List system nodes
  auto pool = node::rt::Discovery()->ListPool("rsm");

  while (matrix::GlobalNow() < kNoMoreFaults) {
    // Some random delay
    matrix::fault::RandomPause(200_jfs, 1000_jfs);

    auto& victim = matrix::fault::RandomServer(pool);
    auto& victim_2 = matrix::fault::RandomServer(pool);

    switch (node::rt::RandomNumber(5)) {
      case 0:
      case 1:
      case 2:
        // Reboot
        victim.FastReboot();
        if (victim.Name() != victim_2.Name()) {
          victim_2.FastReboot();
        }
        break;
      case 3:
        // Freeze
        victim.Pause();
        matrix::fault::RandomPause(100_jfs, 1000_jfs);
        victim.Resume();
        break;
      case 4:
        // Clocks
        victim.AdjustWallClock();
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////

void LeaderAdversary() {
  timber::Logger logger_{"Net-Listener", node::rt::LoggerBackend()};

  auto& listener = matrix::fault::NetworkListener();

  size_t frame_index = 0;

  uint16_t rpc_port = node::rt::Config()->GetInt<uint16_t>("rpc.port");

  while (matrix::GlobalNow() < kNoMoreFaults) {
    std::optional<std::string> leader;

    while (listener.FrameCount() > frame_index) {
      const auto& frame = listener.GetFrame(frame_index++);

      LOG_INFO("Message sent from {}:{} to {}:{}", frame.header.source_host,
               frame.packet.header.source_port, frame.header.dest_host,
               frame.packet.header.dest_port);

      if (frame.packet.header.type != matrix::net::Packet::Type::Data) {
        continue;
      }
      if (frame.packet.header.dest_port != rpc_port) {
        continue;
      }
      // RPC request!
      auto rpc_request = muesli::Deserialize<commute::rpc::proto::Request>(
          frame.packet.message);
      if (rpc_request.method.name == "AppendEntries") {
        LOG_INFO("AppendEntries sent by {}", frame.header.source_host);
        leader.emplace(frame.header.source_host);
      }
    }

    if (leader.has_value()) {
      LOG_INFO("Reboot leader {}", *leader);
      matrix::fault::Server(*leader).FastReboot();
    }

    node::rt::SleepFor(10_jfs);
  }
}

//////////////////////////////////////////////////////////////////////

void NodeReaper() {
  timber::Logger logger_{"Node-Reaper", node::rt::LoggerBackend()};

  // List system nodes
  auto pool = node::rt::Discovery()->ListPool("rsm");

  // Bound on number of crashes
  size_t bound = (pool.size() - 1) / 2;

  // [0, bound]
  size_t crashes = node::rt::RandomNumber(0, bound);

  LOG_INFO("Crash budget: {}", crashes);

  for (size_t i = 0; i < crashes; ++i) {
    matrix::fault::RandomPause(100_jfs, 3000_jfs);

    auto& victim = matrix::fault::RandomServer(pool);

    if (victim.IsAlive()) {
      victim.Crash();
    }
  }
}

//////////////////////////////////////////////////////////////////////

// Seed -> simulation digest
// Deterministic
size_t RunSimulation(size_t seed) {
  auto& runner = matrix::TestRunner::Access();

  static const Jiffies kTimeLimit = 200000_jfs;

  runner.Verbose() << "Simulation seed: " << seed << std::endl;

  matrix::Random random{seed};

  // Randomize simulation parameters
  const size_t replicas = random.Get(3, 5);
  const size_t clients = random.Get(2, 3);
  const size_t increments_per_client = random.Get(2, 3);

  size_t increments = increments_per_client * clients;

  runner.Verbose() << "Parameters: "
                   << "replicas = " << replicas << ", "
                   << "clients = " << clients << ", "
                   << "increments_per_client = " << increments_per_client
                   << std::endl;

  // Reset RPC ids
  commute::rpc::ResetIds();

  matrix::facade::World world{seed};

  runner.Configure(world);

  world.SetTimeModel(tests::MakeAsyncTimeModel());

  // Cluster
  world.MakePool("rsm", kv::ReplicaMain).Size(replicas);
  world.MakePool("proxy", rsm::ProxyMain).Size(2);

  // Clients
  world.AddClients(Client, /*count=*/clients);

  // Adversaries

  if (random.Maybe(5)) {
    // Network partitions
    runner.Verbose() << "Partitions" << std::endl;
    world.AddAdversary(NetAdversary);
  }

  if (random.Maybe(2)) {
    // Reboots, pauses
    runner.Verbose() << "Reboots" << std::endl;
    world.AddAdversary(NodeAdversary);
  }

  if (random.Maybe(2)) {
    // Crashes
    runner.Verbose() << "Crashes" << std::endl;
    world.AddAdversary(NodeReaper);
  }

  if (random.Maybe(2)) {
    runner.Verbose() << "Leader adversary" << std::endl;
    world.AddAdversary(LeaderAdversary);
  }

  // Globals
  world.SetGlobal("increments_per_client", increments_per_client);

  world.InitCounter("increments");
  world.InitCounter("total");

  // For proxies
  world.SetGlobal<std::string>("config.rsm.pool.name", "rsm");

  // For rsm
  world.SetGlobal<std::string>("config.rsm.store.dir", "/rsm/store");

  // For paxos
  world.SetGlobal<int64_t>("config.paxos.backoff.init", 100);
  world.SetGlobal<int64_t>("config.paxos.backoff.max", 3000);
  world.SetGlobal<int64_t>("config.paxos.backoff.factor", 2);

  // Run simulation

  world.Start();
  while (world.GetCounter("increments") < increments &&
         world.TimeElapsed() < kTimeLimit) {
    if (!world.Step()) {
      break;  // Deadlock
    }
  }

  // Stop and compute simulation digest
  size_t digest = world.Stop();

  // Print report
  runner.Verbose() << "Seed " << seed << " -> "
                   << "digest: " << digest << ", time: " << world.TimeElapsed()
                   << ", steps: " << world.StepCount() << std::endl;

  const auto event_log = world.EventLog();

  const bool completed = world.GetCounter("increments") == increments;

  // Time limit exceeded
  if (!completed) {
    // Log
    runner.Report() << "Log:" << std::endl;
    matrix::WriteTextLog(event_log, runner.Report());
    runner.Report() << std::endl;

    runner.Report() << "Simulation for seed = " << seed << " failed: ";

    if (world.TimeElapsed() < kTimeLimit) {
      runner.Report() << "deadlock in simulation" << std::endl;
    } else {
      runner.Report() << "time limit exceeded" << std::endl;
    }
    runner.Fail();
  }

  // Check safety properties
  const size_t total = world.GetCounter("total");
  const size_t total_expected = increments * (0 + increments - 1) / 2;

  runner.Verbose() << "Total = " << total << ", expected = " << total_expected
                   << std::endl;

  const bool correct = total == total_expected;

  if (!correct) {
    // Log
    runner.Report() << "Log:" << std::endl;
    matrix::WriteTextLog(event_log, runner.Report());
    runner.Report() << std::endl;

    // History
    runner.Report() << "Test invariant VIOLATED for seed = " << seed
                    << std::endl;

    runner.Fail();
  }

  return digest;
}

int main(int argc, const char** argv) {
  return matrix::Main(argc, argv, RunSimulation);
}
