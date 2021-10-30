#include <rsm/replica/multipaxos.hpp>

#include <rsm/replica/store/log.hpp>

#include <commute/rpc/call.hpp>

#include <await/fibers/core/api.hpp>
#include <await/fibers/sync/channel.hpp>
#include <await/fibers/sync/mutex.hpp>

#include <timber/log.hpp>

#include <whirl/node/runtime/shortcuts.hpp>
#include <whirl/node/cluster/peer.hpp>

using await::fibers::Channel;
using await::futures::Future;
using await::futures::Promise;

using namespace whirl;

namespace rsm {

//////////////////////////////////////////////////////////////////////

class MultiPaxos : public IReplica {
 public:
  MultiPaxos(IStateMachinePtr state_machine, persist::fs::Path store_dir,
             commute::rpc::IServer* server)
      : state_machine_(std::move(state_machine)),
        log_(store_dir),
        logger_("Replica", node::rt::LoggerBackend()) {
    Start(server);
  }

  Future<Response> Execute(Command command) override {
    auto [future, promise] = await::futures::MakeContract<Response>();

    LOG_INFO("Executing command {}", command);

    {
      auto guard = mutex_.Guard();

      auto response = state_machine_->Apply(command);
      std::move(promise).SetValue(Ack{response});
    }

    return std::move(future);
  };

  void Start(commute::rpc::IServer* /*server*/) {
    // Reset state machine state
    state_machine_->Reset();

    // Open log on disk
    log_.Open();

    // Launch pipeline fibers
    // ...

    // Register RPC services
    // ...
  }

 private:
  // Replicated state
  IStateMachinePtr state_machine_;

  // Persistent log
  Log log_;

  await::fibers::Mutex mutex_;

  // Logging
  timber::Logger logger_;
};

//////////////////////////////////////////////////////////////////////

IReplicaPtr MakeMultiPaxosReplica(IStateMachinePtr state_machine,
                                  commute::rpc::IServer* server) {
  auto store_dir =
      node::rt::Fs()->MakePath(node::rt::Config()->GetString("rsm.store.dir"));

  return std::make_shared<MultiPaxos>(std::move(state_machine),
                                      std::move(store_dir), server);
}

}  // namespace rsm
