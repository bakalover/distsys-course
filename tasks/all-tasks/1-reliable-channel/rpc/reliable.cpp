#include <rpc/reliable.hpp>
#include <rpc/errors.hpp>

// Futures
#include <await/futures/core/future.hpp>

// Fibers
#include <await/fibers/core/api.hpp>
#include <await/fibers/sync/future.hpp>

// Logging
#include <timber/log.hpp>

#include <wheels/support/compiler.hpp>

#include <chrono>

using await::fibers::Await;
using await::futures::Future;
using wheels::Result;

namespace rpc {

//////////////////////////////////////////////////////////////////

class ReliableChannel : public IChannel {
 public:
  ReliableChannel(IChannelPtr fair_loss, Backoff::Params backoff_params,
                  IRuntime* runtime)
      : fair_loss_(std::move(fair_loss)),
        backoff_params_(backoff_params),
        runtime_(runtime),
        logger_("Reliable", runtime->Log()) {
  }

  Future<Message> Call(Method method, Message request,
                       CallOptions options) override {
    WHEELS_UNUSED(backoff_params_);
    WHEELS_UNUSED(runtime_);

    LOG_INFO("Call({}, {}) started", method, request);

    return fair_loss_->Call(method, request,
                            options);  // <-- Your code goes here
  }

 private:
  IChannelPtr fair_loss_;
  const Backoff::Params backoff_params_;
  IRuntime* runtime_;
  timber::Logger logger_;
};

//////////////////////////////////////////////////////////////////////

IChannelPtr MakeReliableChannel(IChannelPtr fair_loss,
                                Backoff::Params backoff_params,
                                IRuntime* runtime) {
  return std::make_shared<ReliableChannel>(std::move(fair_loss), backoff_params,
                                           runtime);
}

}  // namespace rpc
