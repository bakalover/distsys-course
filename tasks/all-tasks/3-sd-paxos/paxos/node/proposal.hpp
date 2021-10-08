#pragma once

#include <muesli/serializable.hpp>

// Enable string serialization
#include <cereal/types/string.hpp>

#include <string>
#include <ostream>

namespace paxos {

////////////////////////////////////////////////////////////////////////////////

using Value = std::string;

////////////////////////////////////////////////////////////////////////////////

// Proposal number

struct ProposalNumber {
  uint64_t k = 0;

  static ProposalNumber Zero() {
    return {0};
  }

  bool operator<(const ProposalNumber& that) const {
    return k < that.k;
  }

  MUESLI_SERIALIZABLE(k)
};

inline std::ostream& operator<<(std::ostream& out, const ProposalNumber& n) {
  out << "{" << n.k << "}";
  return out;
}

////////////////////////////////////////////////////////////////////////////////

// Proposal = Proposal number + Value

struct Proposal {
  ProposalNumber n;
  Value value;

  MUESLI_SERIALIZABLE(n, value)
};

inline std::ostream& operator<<(std::ostream& out, const Proposal& proposal) {
  out << "{" << proposal.n << ", " << proposal.value << "}";
  return out;
}

}  // namespace paxos
