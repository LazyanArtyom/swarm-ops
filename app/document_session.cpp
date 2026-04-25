#include "app/document_session.h"

#include <utility>

namespace app {

DocumentSession::DocumentSession(QObject* parent) : QObject(parent) {}

QString DocumentSession::CurrentFilePath() const {
    return current_file_path_;
}

bool DocumentSession::IsDirty() const {
    return dirty_;
}

bool DocumentSession::HasFilePath() const {
    return !current_file_path_.isEmpty();
}

void DocumentSession::SetCurrentFilePath(QString file_path) {
    if (current_file_path_ == file_path) {
        return;
    }

    current_file_path_ = std::move(file_path);
    emit SigFilePathChanged(current_file_path_);
}

void DocumentSession::SetDirty(bool dirty) {
    if (dirty_ == dirty) {
        return;
    }

    dirty_ = dirty;
    emit SigDirtyChanged(dirty_);
}

void DocumentSession::Reset() {
    SetCurrentFilePath(QString());
    SetDirty(false);
}

}  // namespace app
