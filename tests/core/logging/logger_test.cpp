#include "logging/logger.h"
#include "logging/log_store.h"
#include "logging/qt_message_handler.h"

#include <QtTest/QtTest>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kQtMessageStoreRecordLimit = 8U;

class RecordingSink final : public app::logging::ILogSink {
   public:
    explicit RecordingSink(app::logging::LogLevel level = app::logging::LogLevel::kTrace)
        : level_(level) {}

    [[nodiscard]] bool IsEnabled(app::logging::LogLevel level) const override {
        return app::logging::IsLevelEnabled(level, level_);
    }

    void Log(const app::logging::LogRecord& record) override {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.push_back(record);
    }

    void Flush() override {
        ++flush_count_;
    }

    [[nodiscard]] std::size_t RecordCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_.size();
    }

    [[nodiscard]] app::logging::LogRecord RecordAt(std::size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_.at(index);
    }

    [[nodiscard]] int FlushCount() const {
        return flush_count_;
    }

   private:
    mutable std::mutex mutex_;
    int flush_count_{0};
    app::logging::LogLevel level_{app::logging::LogLevel::kTrace};
    std::vector<app::logging::LogRecord> records_;
};

app::logging::LoggerConfig TestConfig() {
    app::logging::LoggerConfig config;
    config.sink_type = app::logging::LogSinkType::kNone;
    config.memory_level = app::logging::LogLevel::kOff;
    config.default_category = "test";
    config.include_thread_id = false;
    return config;
}

}  // namespace

class LoggerTest : public QObject {
    Q_OBJECT

   private slots:
    static void cleanup() {
        app::logging::QtMessageHandlerBridge::Uninstall();
        app::logging::Logger::Shutdown();
    }

    static void ValidatesFileSinkConfig() {
        app::logging::LoggerConfig config = TestConfig();
        config.sink_type = app::logging::LogSinkType::kRotatingFile;
        config.log_file_path.clear();

        const app::VoidResult<std::string> validation = app::logging::ValidateLoggerConfig(config);

        QVERIFY(!validation);
        QCOMPARE(validation.error(),
                 std::string{"log_file_path must not be empty when file logging is used"});
    }

    static void SendsStructuredRecordsToCustomSink() {
        app::logging::Logger::Init(TestConfig());
        auto sink = std::make_shared<RecordingSink>();
        app::logging::Logger::AddSink(sink);

        app::logging::Logger::Info("plain message");
        app::logging::Logger::WarnFmtFor("network", "formatted {}", "message");
        app::logging::Logger::Flush();

        QCOMPARE(sink->RecordCount(), std::size_t{2});
        QCOMPARE(sink->RecordAt(0).level, app::logging::LogLevel::kInfo);
        QCOMPARE(sink->RecordAt(0).category, std::string{"test"});
        QCOMPARE(sink->RecordAt(0).message, std::string{"plain message"});
        QCOMPARE(sink->RecordAt(1).level, app::logging::LogLevel::kWarn);
        QCOMPARE(sink->RecordAt(1).category, std::string{"network"});
        QCOMPARE(sink->RecordAt(1).message, std::string{"formatted message"});
        QVERIFY(sink->FlushCount() >= 1);
    }

    static void RespectsCustomSinkLevel() {
        app::logging::LoggerConfig config = TestConfig();
        app::logging::Logger::Init(config);

        auto sink = std::make_shared<RecordingSink>(app::logging::LogLevel::kWarn);
        app::logging::Logger::AddSink(sink);

        app::logging::Logger::Info("hidden");
        app::logging::Logger::Error("visible");

        QCOMPARE(sink->RecordCount(), std::size_t{1});
        QCOMPARE(sink->RecordAt(0).message, std::string{"visible"});
    }

    static void StoresRecentRecordsInBoundedMemorySink() {
        app::logging::LoggerConfig config = TestConfig();
        config.memory_level = app::logging::LogLevel::kTrace;
        config.memory_max_records = 3U;
        app::logging::Logger::Init(config);

        app::logging::Logger::Debug("first");
        app::logging::Logger::Info("second");
        app::logging::Logger::Warn("third");
        app::logging::Logger::Error("fourth");

        const auto store = app::logging::Logger::Store();
        QVERIFY(store != nullptr);
        QCOMPARE(store->Capacity(), std::size_t{3});

        const std::vector<app::logging::LogRecord> records = store->Records();
        QCOMPARE(records.size(), std::size_t{3});
        QCOMPARE(records.at(0).message, std::string{"second"});
        QCOMPARE(records.at(1).message, std::string{"third"});
        QCOMPARE(records.at(2).message, std::string{"fourth"});

        const std::vector<app::logging::LogRecord> warnings =
            store->Records(app::logging::LogLevel::kWarn);
        QCOMPARE(warnings.size(), std::size_t{2});
    }

    static void ReplaysExistingRecordsWhenObserverSubscribes() {
        app::logging::LogStore store(kQtMessageStoreRecordLimit);
        app::logging::LogRecord record;
        record.level = app::logging::LogLevel::kInfo;
        record.message = "stored";
        store.Log(record);

        std::vector<app::logging::LogRecord> observed;
        const app::logging::LogStore::ObserverToken token =
            store.AddObserver([&observed](const app::logging::LogRecord& replayed_record) {
                observed.push_back(replayed_record);
            }, true);

        QCOMPARE(observed.size(), std::size_t{1});
        QCOMPARE(observed.front().message, std::string{"stored"});

        store.RemoveObserver(token);
    }

    static void RoutesQtMessagesIntoLogger() {
        app::logging::LoggerConfig config = TestConfig();
        config.memory_level = app::logging::LogLevel::kDebug;
        config.memory_max_records = kQtMessageStoreRecordLimit;
        app::logging::Logger::Init(config);
        app::logging::QtMessageHandlerBridge::Install();

        qWarning("qt warning from test");

        app::logging::QtMessageHandlerBridge::Uninstall();

        const auto store = app::logging::Logger::Store();
        QVERIFY(store != nullptr);
        const std::vector<app::logging::LogRecord> records = store->Records();
        QVERIFY(!records.empty());
        QCOMPARE(records.back().level, app::logging::LogLevel::kWarn);
        QCOMPARE(records.back().message, std::string{"qt warning from test"});
    }
};

QTEST_APPLESS_MAIN(LoggerTest)

#include "logger_test.moc"
