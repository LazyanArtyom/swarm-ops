#pragma once

#include <QObject>
#include <QString>

namespace app {

class DocumentSession final : public QObject {
    Q_OBJECT

   public:
    explicit DocumentSession(QObject* parent = nullptr);
    ~DocumentSession() override = default;

    [[nodiscard]] QString CurrentFilePath() const;
    [[nodiscard]] bool IsDirty() const;
    [[nodiscard]] bool HasFilePath() const;

    void SetCurrentFilePath(QString file_path);
    void SetDirty(bool dirty);
    void Reset();

   signals:
    void SigFilePathChanged(const QString& file_path);
    void SigDirtyChanged(bool dirty);

   private:
    QString current_file_path_;
    bool dirty_{false};
};

}  // namespace app
