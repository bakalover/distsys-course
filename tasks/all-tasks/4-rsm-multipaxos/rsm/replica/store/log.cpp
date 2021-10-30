#include <rsm/replica/store/log.hpp>

#include <persist/rsm/log/file.hpp>

#include <muesli/serialize.hpp>

#include <whirl/node/runtime/shortcuts.hpp>

namespace rsm {

Log::Log(const persist::fs::Path& dir) : impl_(MakeLogImpl(dir)) {
}

void Log::Open() {
  impl_->Open();
}

std::optional<LogEntry> Log::Read(size_t index) const {
  auto entry = impl_->Read(index);
  if (entry.has_value()) {
    return muesli::Deserialize<LogEntry>(*entry);
  }
  return std::nullopt;
}

void Log::Update(size_t index, LogEntry entry) {
  impl_->Update(index, muesli::Serialize(entry));
}

bool Log::IsEmpty(size_t index) const {
  return !impl_->Read(index).has_value();
}

void Log::TruncatePrefix(size_t end_index) {
  impl_->TruncatePrefix(end_index);
}

std::shared_ptr<persist::rsm::ILog> Log::MakeLogImpl(
    const persist::fs::Path& dir) {
  return std::make_shared<persist::rsm::FileLog>(whirl::node::rt::Fs(),
                                                 dir / "log");
}

}  // namespace rsm
