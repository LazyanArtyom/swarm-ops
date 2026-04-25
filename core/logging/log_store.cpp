#include "logging/log_store.h"

#include <algorithm>

namespace app::logging {
namespace {

constexpr std::size_t kMinimumCapacity = 1U;

}  // namespace

LogStore::LogStore(std::size_t capacity, LogLevel level)
    : capacity_(std::max(capacity, kMinimumCapacity)), level_(level) {}

bool LogStore::IsEnabled(LogLevel level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return IsLevelEnabled(level, level_);
}

void LogStore::Log(const LogRecord& record) {
    std::vector<Observer> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!IsLevelEnabled(record.level, level_)) {
            return;
        }

        records_.push_back(record);
        TrimToCapacity();

        observers.reserve(observers_.size());
        for (const auto& observer_entry : observers_) {
            if (observer_entry.second) {
                observers.push_back(observer_entry.second);
            }
        }
    }

    for (const auto& observer : observers) {
        observer(record);
    }
}

void LogStore::Flush() {}

void LogStore::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

void LogStore::SetCapacity(std::size_t capacity) {
    std::lock_guard<std::mutex> lock(mutex_);
    capacity_ = std::max(capacity, kMinimumCapacity);
    TrimToCapacity();
}

void LogStore::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

std::size_t LogStore::Capacity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return capacity_;
}

std::size_t LogStore::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_.size();
}

LogLevel LogStore::Level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

std::vector<LogRecord> LogStore::Records() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {records_.begin(), records_.end()};
}

std::vector<LogRecord> LogStore::Records(LogLevel minimum_level) const {
    std::vector<LogRecord> filtered_records;
    std::lock_guard<std::mutex> lock(mutex_);
    filtered_records.reserve(records_.size());

    for (const auto& record : records_) {
        if (IsLevelEnabled(record.level, minimum_level)) {
            filtered_records.push_back(record);
        }
    }

    return filtered_records;
}

LogStore::ObserverToken LogStore::AddObserver(Observer observer) {
    return AddObserver(std::move(observer), false);
}

LogStore::ObserverToken LogStore::AddObserver(Observer observer, bool replay_existing) {
    if (!observer) {
        return {};
    }

    std::vector<LogRecord> replay_records;
    const Observer callback = std::move(observer);
    ObserverToken token{};

    {
        std::lock_guard<std::mutex> lock(mutex_);
        token = next_observer_token_;
        ++next_observer_token_;
        if (replay_existing) {
            replay_records.assign(records_.begin(), records_.end());
        }
        observers_.emplace_back(token, callback);
    }

    for (const auto& record : replay_records) {
        callback(record);
    }

    return token;
}

void LogStore::RemoveObserver(ObserverToken token) {
    if (token == 0U) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto removed = std::ranges::remove_if(
        observers_, [token](const auto& item) { return item.first == token; });
    observers_.erase(removed.begin(), removed.end());
}

void LogStore::TrimToCapacity() {
    while (records_.size() > capacity_) {
        records_.pop_front();
    }
}

}  // namespace app::logging
