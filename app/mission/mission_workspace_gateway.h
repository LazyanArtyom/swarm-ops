#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "app/client_gateway/client_gateway_types.h"
#include "app/mission/mission_workspace.h"

namespace app::mission {

class IMissionWorkspaceGateway : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~IMissionWorkspaceGateway() override = default;

    [[nodiscard]] virtual MissionWorkspace ActiveWorkspace() const = 0;
    [[nodiscard]] virtual QList<client_gateway::WorkspaceListItem> ListWorkspaces() const = 0;
    [[nodiscard]] virtual QString CreateWorkspace(const WorkspaceBackground& background) = 0;
    [[nodiscard]] virtual bool OpenWorkspace(const QString& workspace_id, QString user_display_name,
                                             QString* error_message = nullptr) = 0;
    [[nodiscard]] virtual bool SaveWorkspace(MissionWorkspace workspace,
                                             QString* error_message = nullptr) = 0;
    [[nodiscard]] virtual bool SetWorkspaceShared(const QString& workspace_id, bool shared,
                                                  QString* error_message = nullptr) = 0;
    [[nodiscard]] virtual client_gateway::WorkspaceShareInvite CreateShareInvite(
        const QString& workspace_id, QString* error_message = nullptr) = 0;
    virtual void ReplaceWorkspace(MissionWorkspace workspace) = 0;

   signals:
    void SigWorkspaceChanged(const app::mission::MissionWorkspace& workspace);
    void SigWorkspaceListChanged(QList<app::client_gateway::WorkspaceListItem> workspaces);
    void SigWorkspacePresenceChanged(
        const QString& workspace_id,
        QList<app::client_gateway::WorkspacePresenceUser> users);
};

}  // namespace app::mission
