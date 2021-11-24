#pragma once

#include <rsm/replica/store/log_entry.hpp>

#include <muesli/serializable.hpp>

#include <cstdlib>

namespace rsm {

namespace raft::proto {

//////////////////////////////////////////////////////////////////////

// Leader election

struct RequestVote {
  struct Request {
    uint64_t term;
    std::string candidate;
    uint64_t last_log_index;
    uint64_t last_log_term;

    MUESLI_SERIALIZABLE(term, candidate, last_log_index, last_log_term)
  };

  struct Response {
    uint64_t term;
    bool vote_granted;

    MUESLI_SERIALIZABLE(term, vote_granted)
  };
};

//////////////////////////////////////////////////////////////////////

// Replication

struct AppendEntries {
  struct Request {
    uint64_t term;
    std::string leader;
    uint64_t prev_log_index;
    uint64_t prev_log_term;
    LogEntries entries;
    uint64_t leader_commit_index;

    MUESLI_SERIALIZABLE(term, leader, prev_log_index, prev_log_term, entries,
                        leader_commit_index)
  };

  struct Response {
    uint64_t term;
    bool success;
    uint64_t next_index_hint;

    MUESLI_SERIALIZABLE(term, success, next_index_hint)
  };
};

}  // namespace raft::proto

}  // namespace rsm
