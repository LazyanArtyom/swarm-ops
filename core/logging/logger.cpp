#include "logging/logger.h"

#include "logging/log_store.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <span>
#include <sstream>
#include <thread>
#include <vector>

namespace app::logging {
namespace {

constexpr std::string_view kDefaultPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
constexpr std::size_t kMaxBuiltInSpdlogSinks = 2U;

void PrintLoggerError(std::string_view message);

[[nodiscard]] spdlog::level::level_enum ToSpdLevel(LogLevel level) {
    switch (level) {
        case LogLevel::kTrace:
            return spdlog::level::trace;
        case LogLevel::kDebug:
            return spdlog::level::debug;
        case LogLevel::kInfo:
            return spdlog::level::info;
        case LogLevel::kWarn:
            return spdlog::level::warn;
        case LogLevel::kError:
            return spdlog::level::err;
        case LogLevel::kCritical:
            return spdlog::level::critical;
        case LogLevel::kOff:
            return spdlog::level::off;
    }
    return spdlog::level::info;
}

[[nodiscard]] LogLevel MostVerbose(LogLevel left, LogLevel right) {
    if (left == LogLevel::kOff) {
        return right;
    }
    if (right == LogLevel::kOff) {
        return left;
    }
    return static_cast<std::uint8_t>(left) < static_cast<std::uint8_t>(right) ? left : right;
}

class SpdlogSink final : public ILogSink {
   public:
    explicit SpdlogSink(const LoggerConfig& config) : logger_(CreateLogger(config)) {}

    [[nodiscard]] bool IsEnabled(LogLevel level) const override {
        return logger_ != nullptr && logger_->should_log(ToSpdLevel(level));
    }

    void Log(const LogRecord& record) override {
        if (logger_ == nullptr) {
            return;
        }

        if (record.thread_id.has_value()) {
            logger_->log(ToSpdLevel(record.level), "[{}] [thread:{}] {}", record.category,
                         *record.thread_id, record.message);
            return;
        }
        logger_->log(ToSpdLevel(record.level), "[{}] {}", record.category, record.message);
    }

    void Flush() override {
        if (logger_ != nullptr) {
            logger_->flush();
        }
    }

   private:
    [[nodiscard]] static LogLevel SanitizeFlushLevel(LogLevel level) {
        return level == LogLevel::kOff ? LogLevel::kCritical : level;
    }

    [[nodiscard]] static std::shared_ptr<spdlog::logger> CreateLogger(const LoggerConfig& config) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(kMaxBuiltInSpdlogSinks);
        LogLevel logger_level = LogLevel::kOff;

        if (config.sink_type == LogSinkType::kTerminal ||
            config.sink_type == LogSinkType::kTerminalAndRotatingFile) {
            auto terminal_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            terminal_sink->set_level(ToSpdLevel(config.terminal_level));
            sinks.push_back(terminal_sink);
            logger_level = MostVerbose(logger_level, config.terminal_level);
        }

        if (config.sink_type == LogSinkType::kRotatingFile ||
            config.sink_type == LogSinkType::kTerminalAndRotatingFile) {
            const std::filesystem::path log_path = config.log_file_path;
            if (log_path.has_parent_path()) {
                std::error_code error_code;
                std::filesystem::create_directories(log_path.parent_path(), error_code);
                if (error_code) {
                    throw spdlog::spdlog_ex("failed to create log directory '" +
                                            log_path.parent_path().string() +
                                            "': " + error_code.message());
                }
            }

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                config.log_file_path, static_cast<std::size_t>(config.max_file_size_bytes),
                static_cast<std::size_t>(config.max_files));
            file_sink->set_level(ToSpdLevel(config.file_level));
            sinks.push_back(file_sink);
            logger_level = MostVerbose(logger_level, config.file_level);
        }

        auto logger =
            std::make_shared<spdlog::logger>(config.logger_name, sinks.begin(), sinks.end());
        const std::string pattern =
            config.pattern.empty() ? std::string{kDefaultPattern} : config.pattern;
        logger->set_pattern(pattern);
        logger->set_level(ToSpdLevel(logger_level));
        logger->flush_on(ToSpdLevel(SanitizeFlushLevel(config.flush_level)));
        logger->set_error_handler([](const std::string& message) {
            PrintLoggerError(std::string("spdlog internal error: ") + message);
        });
        return logger;
    }

    std::shared_ptr<spdlog::logger> logger_;
};

struct LoggerState {
    std::mutex mutex;
    LoggerConfig config;
    std::shared_ptr<LogStore> store;
    std::vector<std::shared_ptr<ILogSink>> sinks;
};

LoggerState& State() {
    static LoggerState state;
    return state;
}

void PrintLoggerError(std::string_view message) {
    std::cerr << "App logger: ";
    std::cerr.write(message.data(), static_cast<std::streamsize>(message.size()));
    std::cerr << '\n';
}

[[nodiscard]] std::string ThreadIdString() {
    std::ostringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}

[[nodiscard]] std::vector<std::shared_ptr<ILogSink>> CreateDefaultSinks(
    const LoggerConfig& config, const std::shared_ptr<LogStore>& store) {
    std::vector<std::shared_ptr<ILogSink>> sinks;

    if (config.sink_type != LogSinkType::kNone) {
        try {
            sinks.push_back(std::make_shared<SpdlogSink>(config));
        } catch (const spdlog::spdlog_ex& error) {
            PrintLoggerError(std::string("failed to initialize configured logger: ") +
                             error.what() + " - falling back to terminal");
            LoggerConfig fallback_config = config;
            fallback_config.sink_type = LogSinkType::kTerminal;
            fallback_config.terminal_level = config.level;
            fallback_config.log_file_path.clear();
            sinks.push_back(std::make_shared<SpdlogSink>(fallback_config));
        }
    }

    if (config.memory_level != LogLevel::kOff && store != nullptr) {
        sinks.push_back(store);
    }

    return sinks;
}

[[nodiscard]] std::string EffectiveCategory(const LoggerConfig& config, std::string_view category) {
    if (!category.empty()) {
        return std::string{category};
    }
    return config.default_category.empty() ? std::string{"app"} : config.default_category;
}

[[nodiscard]] std::vector<std::shared_ptr<ILogSink>> CurrentSinks() {
    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.store == nullptr && state.config.memory_level != LogLevel::kOff) {
        state.store =
            std::make_shared<LogStore>(state.config.memory_max_records, state.config.memory_level);
    }
    if (state.sinks.empty() &&
        (state.config.sink_type != LogSinkType::kNone || state.store != nullptr)) {
        state.sinks = CreateDefaultSinks(state.config, state.store);
    }
    return state.sinks;
}

void FlushSinks(std::span<const std::shared_ptr<ILogSink>> sinks) {
    for (const auto& sink : sinks) {
        if (sink != nullptr) {
            sink->Flush();
        }
    }
}

[[nodiscard]] bool AnySinkEnabled(std::span<const std::shared_ptr<ILogSink>> sinks,
                                  LogLevel level) {
    return std::ranges::any_of(
        sinks, [level](const auto& sink) { return sink != nullptr && sink->IsEnabled(level); });
}

}  // namespace

std::string_view ToString(LogLevel level) {
    switch (level) {
        case LogLevel::kTrace:
            return "trace";
        case LogLevel::kDebug:
            return "debug";
        case LogLevel::kInfo:
            return "info";
        case LogLevel::kWarn:
            return "warn";
        case LogLevel::kError:
            return "error";
        case LogLevel::kCritical:
            return "critical";
        case LogLevel::kOff:
            return "off";
    }
    return "info";
}

std::string_view ToString(LogSinkType sink_type) {
    switch (sink_type) {
        case LogSinkType::kNone:
            return "none";
        case LogSinkType::kTerminal:
            return "terminal";
        case LogSinkType::kRotatingFile:
            return "file";
        case LogSinkType::kTerminalAndRotatingFile:
            return "terminal+file";
    }
    return "terminal";
}

std::optional<LogLevel> LogLevelFromString(std::string_view value) {
    if (value == "trace") {
        return LogLevel::kTrace;
    }
    if (value == "debug") {
        return LogLevel::kDebug;
    }
    if (value == "info") {
        return LogLevel::kInfo;
    }
    if (value == "warn" || value == "warning") {
        return LogLevel::kWarn;
    }
    if (value == "error") {
        return LogLevel::kError;
    }
    if (value == "critical") {
        return LogLevel::kCritical;
    }
    if (value == "off") {
        return LogLevel::kOff;
    }
    return std::nullopt;
}

std::optional<LogSinkType> LogSinkTypeFromString(std::string_view value) {
    if (value == "none") {
        return LogSinkType::kNone;
    }
    if (value == "terminal") {
        return LogSinkType::kTerminal;
    }
    if (value == "file") {
        return LogSinkType::kRotatingFile;
    }
    if (value == "terminal+file" || value == "terminal_and_file") {
        return LogSinkType::kTerminalAndRotatingFile;
    }
    return std::nullopt;
}

bool IsLevelEnabled(LogLevel message_level, LogLevel threshold) {
    if (message_level == LogLevel::kOff || threshold == LogLevel::kOff) {
        return false;
    }
    return static_cast<std::uint8_t>(message_level) >= static_cast<std::uint8_t>(threshold);
}

app::VoidResult<std::string> ValidateLoggerConfig(const LoggerConfig& config) {
    if (config.logger_name.empty()) {
        return std::unexpected("logger_name must not be empty");
    }
    if (config.max_file_size_bytes == 0U) {
        return std::unexpected("max_file_size_bytes must be > 0");
    }
    if (config.max_files == 0U) {
        return std::unexpected("max_files must be > 0");
    }
    if (config.memory_level != LogLevel::kOff && config.memory_max_records == 0U) {
        return std::unexpected("memory_max_records must be > 0 when memory logging is enabled");
    }
    if ((config.sink_type == LogSinkType::kRotatingFile ||
         config.sink_type == LogSinkType::kTerminalAndRotatingFile) &&
        config.log_file_path.empty()) {
        return std::unexpected("log_file_path must not be empty when file logging is used");
    }
    return {};
}

void Logger::Init(const LoggerConfig& config) {
    LoggerConfig active_config = config;
    if (const app::VoidResult<std::string> validation = ValidateLoggerConfig(config); !validation) {
        PrintLoggerError(std::string("invalid logger config: ") + validation.error() +
                         " - falling back to terminal");
        active_config = LoggerConfig{};
    }

    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.config = active_config;
    state.store =
        active_config.memory_level == LogLevel::kOff
            ? nullptr
            : std::make_shared<LogStore>(active_config.memory_max_records,
                                         active_config.memory_level);
    state.sinks = CreateDefaultSinks(active_config, state.store);
}

void Logger::Shutdown() {
    std::vector<std::shared_ptr<ILogSink>> sinks;
    {
        LoggerState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        sinks = std::move(state.sinks);
        state.store.reset();
        state.config = LoggerConfig{};
    }

    FlushSinks(sinks);
}

void Logger::Flush() {
    FlushSinks(CurrentSinks());
}

void Logger::AddSink(std::shared_ptr<ILogSink> sink) {
    if (sink == nullptr) {
        return;
    }

    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.sinks.push_back(std::move(sink));
}

void Logger::RemoveSink(const std::shared_ptr<ILogSink>& sink) {
    if (sink == nullptr) {
        return;
    }

    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    const auto removed = std::ranges::remove(state.sinks, sink);
    state.sinks.erase(removed.begin(), removed.end());
}

void Logger::SetSinks(std::span<const std::shared_ptr<ILogSink>> sinks) {
    SetSinks(std::vector<std::shared_ptr<ILogSink>>(sinks.begin(), sinks.end()));
}

void Logger::SetSinks(std::vector<std::shared_ptr<ILogSink>> sinks) {
    std::erase(sinks, nullptr);
    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.sinks = std::move(sinks);
}

LoggerConfig Logger::CurrentConfig() {
    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.config;
}

std::shared_ptr<LogStore> Logger::Store() {
    LoggerState& state = State();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.store;
}

bool Logger::IsEnabled(LogLevel level) {
    std::vector<std::shared_ptr<ILogSink>> sinks;
    {
        LoggerState& state = State();
        std::lock_guard<std::mutex> lock(state.mutex);
        sinks = state.sinks;
    }

    if (sinks.empty()) {
        sinks = CurrentSinks();
    }

    return AnySinkEnabled(sinks, level);
}

void Logger::Trace(std::string_view message) {
    LogPlain(LogLevel::kTrace, std::string_view{}, message);
}
void Logger::Debug(std::string_view message) {
    LogPlain(LogLevel::kDebug, std::string_view{}, message);
}
void Logger::Info(std::string_view message) {
    LogPlain(LogLevel::kInfo, std::string_view{}, message);
}
void Logger::Warn(std::string_view message) {
    LogPlain(LogLevel::kWarn, std::string_view{}, message);
}
void Logger::Error(std::string_view message) {
    LogPlain(LogLevel::kError, std::string_view{}, message);
}
void Logger::Critical(std::string_view message) {
    LogPlain(LogLevel::kCritical, std::string_view{}, message);
}

void Logger::TraceFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kTrace, category, message);
}
void Logger::DebugFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kDebug, category, message);
}
void Logger::InfoFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kInfo, category, message);
}
void Logger::WarnFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kWarn, category, message);
}
void Logger::ErrorFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kError, category, message);
}
void Logger::CriticalFor(std::string_view category, std::string_view message) {
    LogPlain(LogLevel::kCritical, category, message);
}

void Logger::LogPlain(LogLevel level, std::string_view category, std::string_view message) {
    if (!IsEnabled(level)) {
        return;
    }

    LoggerConfig config = CurrentConfig();
    const LogRecord record{
        .timestamp = std::chrono::system_clock::now(),
        .level = level,
        .category = EffectiveCategory(config, category),
        .message = std::string{message},
        .thread_id =
            config.include_thread_id ? std::optional<std::string>{ThreadIdString()} : std::nullopt,
    };

    for (const auto& sink : CurrentSinks()) {
        if (sink != nullptr && sink->IsEnabled(level)) {
            sink->Log(record);
        }
    }
}

}  // namespace app::logging
