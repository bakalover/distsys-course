#pragma once

#include <rsm/replica/store/log_entry.hpp>

#include <persist/fs/path.hpp>
#include <persist/rsm/log/log.hpp>

#include <memory>
#include <optional>

namespace rsm {

// Persistent log
// Indexed from 1
// NOT thread safe, external synchronization required

class Log {
 public:
  explicit Log(const persist::fs::Path& dir);

  // One-shot
  void Open();

  bool IsEmpty(size_t index) const;
  std::optional<LogEntry> Read(size_t index) const;

  void Update(size_t index, LogEntry entry);

  void TruncatePrefix(size_t index);

 private:
  std::shared_ptr<persist::rsm::ILog> MakeLogImpl(const persist::fs::Path& dir);

 private:
  std::shared_ptr<persist::rsm::ILog> impl_;
};

}  // namespace rsm
