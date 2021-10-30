#pragma once

#include <rsm/client/command.hpp>

#include <muesli/serializable.hpp>

namespace rsm {

struct LogEntry {
  // Acceptor state?
  uint64_t ___;

  // Make empty log entry
  static LogEntry Empty() {
    return {};
  }

  MUESLI_SERIALIZABLE(___)
};

}  // namespace rsm
