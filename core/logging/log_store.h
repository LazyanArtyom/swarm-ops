#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "logging/logger.h"

namespace app::logging {

class LogStore final : public ILogSink {
   public:
    using Observer = std::function<void(const LogRecord&)>;
    using ObserverToken = std::uint64_t;

    explicit LogStore(std::size_t capacity = kDefaultMemoryLogRecords,
                      LogLevel level = LogLevel::kTrace);
    ~LogStore() override = default;

    [[nodiscard]] bool IsEnabled(LogLevel level) const override;
    void Log(const LogRecord& record) override;
    void Flush() override;

    void Clear();
    void SetCapacity(std::size_t capacity);
    void SetLevel(LogLevel level);

    [[nodiscard]] std::size_t Capacity() const;
    [[nodiscard]] std::size_t Size() const;
    [[nodiscard]] LogLevel Level() const;
    [[nodiscard]] std::vector<LogRecord> Records() const;
    [[nodiscard]] std::vector<LogRecord> Records(LogLevel minimum_level) const;

    [[nodiscard]] ObserverToken AddObserver(Observer observer);
    [[nodiscard]] ObserverToken AddObserver(Observer observer, bool replay_existing);
    void RemoveObserver(ObserverToken token);

   private:
    void TrimToCapacity();

    mutable std::mutex mutex_;
    std::deque<LogRecord> records_;
    std::vector<std::pair<ObserverToken, Observer>> observers_;
    std::size_t capacity_{kDefaultMemoryLogRecords};
    LogLevel level_{LogLevel::kTrace};
    ObserverToken next_observer_token_{1U};
};

}  // namespace app::logging
