#pragma once

#include <QObject>
#include <QString>

#include "app/mission/mission_workspace.h"

namespace app::mission {

class IMissionWorkspaceGateway : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~IMissionWorkspaceGateway() override = default;

    [[nodiscard]] virtual MissionWorkspace ActiveWorkspace() const = 0;
    [[nodiscard]] virtual QString CreateWorkspace(const WorkspaceBackground& background) = 0;
    virtual void ReplaceWorkspace(MissionWorkspace workspace) = 0;

   signals:
    void SigWorkspaceChanged(const app::mission::MissionWorkspace& workspace);
};

}  // namespace app::mission
