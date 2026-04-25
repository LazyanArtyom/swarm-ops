#pragma once

#include <fmt/format.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "utils/result.h"

namespace app::logging {

constexpr std::uint64_t kBytesPerMebibyte = 1024ULL * 1024U;
constexpr std::uint64_t kDefaultMaxFileSizeBytes = 10U * kBytesPerMebibyte;
constexpr std::size_t kDefaultMemoryLogRecords = 10000U;

class LogStore;

enum class LogSinkType : std::uint8_t {
    kNone,
    kTerminal,
    kRotatingFile,
    kTerminalAndRotatingFile,
};

enum class LogLevel : std::uint8_t {
    kTrace,
    kDebug,
    kInfo,
    kWarn,
    kError,
    kCritical,
    kOff,
};

struct LoggerConfig {
    LogSinkType sink_type{LogSinkType::kTerminal};
    LogLevel level{LogLevel::kInfo};
    LogLevel terminal_level{LogLevel::kInfo};
    LogLevel file_level{LogLevel::kInfo};
    LogLevel console_level{LogLevel::kInfo};
    LogLevel memory_level{LogLevel::kTrace};
    LogLevel flush_level{LogLevel::kWarn};

    std::string logger_name{"app"};
    std::string default_category{"app"};
    std::string log_file_path{"app.log"};
    std::uint64_t max_file_size_bytes{kDefaultMaxFileSizeBytes};
    std::uint32_t max_files{3U};
    std::size_t memory_max_records{kDefaultMemoryLogRecords};
    bool include_thread_id{true};

    std::string pattern;
};

struct LogRecord {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level{LogLevel::kInfo};
    std::string category;
    std::string message;
    std::optional<std::string> thread_id;
};

[[nodiscard]] std::string_view ToString(LogLevel level);
[[nodiscard]] std::string_view ToString(LogSinkType sink_type);
[[nodiscard]] std::optional<LogLevel> LogLevelFromString(std::string_view value);
[[nodiscard]] std::optional<LogSinkType> LogSinkTypeFromString(std::string_view value);
[[nodiscard]] bool IsLevelEnabled(LogLevel message_level, LogLevel threshold);
[[nodiscard]] app::VoidResult<std::string> ValidateLoggerConfig(const LoggerConfig& config);

class ILogSink {
   public:
    virtual ~ILogSink() = default;

    [[nodiscard]] virtual bool IsEnabled(LogLevel level) const = 0;
    virtual void Log(const LogRecord& record) = 0;
    virtual void Flush() = 0;
};

class Logger final {
   public:
    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void Init(const LoggerConfig& config);
    static void Shutdown();
    static void Flush();

    static void AddSink(std::shared_ptr<ILogSink> sink);
    static void RemoveSink(const std::shared_ptr<ILogSink>& sink);
    static void SetSinks(std::span<const std::shared_ptr<ILogSink>> sinks);
    static void SetSinks(std::vector<std::shared_ptr<ILogSink>> sinks);

    [[nodiscard]] static LoggerConfig CurrentConfig();
    [[nodiscard]] static std::shared_ptr<LogStore> Store();
    [[nodiscard]] static bool IsEnabled(LogLevel level);

    static void Trace(std::string_view message);
    static void Debug(std::string_view message);
    static void Info(std::string_view message);
    static void Warn(std::string_view message);
    static void Error(std::string_view message);
    static void Critical(std::string_view message);

    static void TraceFor(std::string_view category, std::string_view message);
    static void DebugFor(std::string_view category, std::string_view message);
    static void InfoFor(std::string_view category, std::string_view message);
    static void WarnFor(std::string_view category, std::string_view message);
    static void ErrorFor(std::string_view category, std::string_view message);
    static void CriticalFor(std::string_view category, std::string_view message);

    template <typename... Args>
    static void TraceFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kTrace, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void DebugFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kDebug, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void InfoFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kInfo, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void WarnFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kWarn, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void ErrorFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kError, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void CriticalFmt(fmt::format_string<Args...> format, Args&&... args) {
        LogFmt(LogLevel::kCritical, std::string_view{}, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void TraceFmtFor(std::string_view category, fmt::format_string<Args...> format,
                            Args&&... args) {
        LogFmt(LogLevel::kTrace, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void DebugFmtFor(std::string_view category, fmt::format_string<Args...> format,
                            Args&&... args) {
        LogFmt(LogLevel::kDebug, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void InfoFmtFor(std::string_view category, fmt::format_string<Args...> format,
                           Args&&... args) {
        LogFmt(LogLevel::kInfo, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void WarnFmtFor(std::string_view category, fmt::format_string<Args...> format,
                           Args&&... args) {
        LogFmt(LogLevel::kWarn, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void ErrorFmtFor(std::string_view category, fmt::format_string<Args...> format,
                            Args&&... args) {
        LogFmt(LogLevel::kError, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void CriticalFmtFor(std::string_view category, fmt::format_string<Args...> format,
                               Args&&... args) {
        LogFmt(LogLevel::kCritical, category, format, std::forward<Args>(args)...);
    }

   private:
    static void LogPlain(LogLevel level, std::string_view category, std::string_view message);

    template <typename... Args>
    static void LogFmt(LogLevel level, std::string_view category,
                       fmt::format_string<Args...> format, Args&&... args) {
        if (!IsEnabled(level)) {
            return;
        }
        LogPlain(level, category, fmt::format(format, std::forward<Args>(args)...));
    }
};

}  // namespace app::logging
