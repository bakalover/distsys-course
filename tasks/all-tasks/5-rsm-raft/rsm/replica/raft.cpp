#include <rsm/replica/raft.hpp>

#include <rsm/replica/proto/raft.hpp>
#include <rsm/replica/store/log.hpp>

#include <commute/rpc/call.hpp>
#include <commute/rpc/service_base.hpp>

#include <await/fibers/core/api.hpp>
#include <await/fibers/sync/channel.hpp>
#include <await/fibers/sync/mutex.hpp>

#include <timber/log.hpp>

#include <whirl/node/runtime/shortcuts.hpp>
#include <whirl/node/cluster/peer.hpp>

#include <mutex>
#include <optional>

using await::futures::Future;
using await::futures::Promise;

using namespace whirl;

namespace rsm {

enum class NodeState {
  Candidate = 1,  // Do not format
  Follower = 2,
  Leader = 3
};

class Raft : public IReplica,
             public commute::rpc::ServiceBase<Raft>,
             public node::cluster::Peer,
             public std::enable_shared_from_this<Raft> {
 public:
  Raft(IStateMachinePtr state_machine, persist::fs::Path store_dir)
      : Peer(node::rt::Config()),
        state_machine_(std::move(state_machine)),
        log_(node::rt::Fs(), store_dir),
        logger_("Raft", node::rt::LoggerBackend()) {
  }

  Future<proto::Response> Execute(Command command) override {
    auto [future, promise] = await::futures::MakeContract<proto::Response>();

    {
      std::lock_guard guard(mutex_);
      auto response = state_machine_->Apply(command);
      std::move(promise).SetValue(proto::Ack{response});
    }

    return std::move(future);
  };

  void Start(commute::rpc::IServer* /*rpc_server*/) {
  }

 protected:
  // RPC handlers

  void RegisterMethods() override {
    COMMUTE_RPC_REGISTER_HANDLER(RequestVote);
    COMMUTE_RPC_REGISTER_HANDLER(AppendEntries);
  }

  // Leader election

  void RequestVote(const raft::proto::RequestVote::Request& /*request*/,
                   raft::proto::RequestVote::Response* /*response*/) {
    // Your code goes here
  }

  // Replication

  void AppendEntries(const raft::proto::AppendEntries::Request& /*request*/,
                     raft::proto::AppendEntries::Response* /*response*/) {
    // Your code goes here
  }

 private:
  // State changes

  // With mutex
  void BecomeFollower(size_t /*term*/) {
    // Your code goes here
  }

  // With mutex
  void BecomeCandidate() {
    // Your code goes here
  }

  // With mutex
  void BecomeLeader() {
    // Your code goes here
  }

 private:
  // Fibers

  void ApplyCommittedCommands() {
    // Your code goes here
  }

  void RunElectionTimer(size_t /*term*/) {
    // Your code goes here
  }

  void RunRequestVote(std::string /*peer*/, size_t /*term*/) {
    // Your code goes here
  }

  void RunAppendEntries(std::string /*peer*/, size_t /*term*/) {
    // Your code goes here
  }

  void Replicate(size_t /*term*/) {
    // Your code goes here
  }

 private:
  Jiffies ElectionTimeout() const {
    uint64_t rtt = node::rt::Config()->GetInt<uint64_t>("net.rtt");
    // Your code goes here
    return Jiffies{2 * rtt};
  }

  void PleaseCompiler() {
    WHEELS_UNUSED(log_);
    WHEELS_UNUSED(term_);
    WHEELS_UNUSED(state_);
    WHEELS_UNUSED(voted_for_);
    WHEELS_UNUSED(next_index_);
    WHEELS_UNUSED(match_index_);
    WHEELS_UNUSED(commit_index_);
  }

 private:
  await::fibers::Mutex mutex_;

  IStateMachinePtr state_machine_;

  Log log_;

  size_t term_{0};
  NodeState state_;

  std::optional<std::string> leader_;
  std::optional<std::string> voted_for_;

  // Peer -> next index id
  std::map<std::string, size_t> next_index_;
  // Peer -> match index id
  std::map<std::string, size_t> match_index_;

  size_t commit_index_{0};

  timber::Logger logger_;
};

//////////////////////////////////////////////////////////////////////

IReplicaPtr MakeRaftReplica(IStateMachinePtr state_machine,
                            commute::rpc::IServer* server) {
  auto store_dir =
      node::rt::Fs()->MakePath(node::rt::Config()->GetString("rsm.store.dir"));

  auto replica =
      std::make_shared<Raft>(std::move(state_machine), std::move(store_dir));
  replica->Start(server);
  return replica;
}

}  // namespace rsm
