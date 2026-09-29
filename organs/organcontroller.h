#ifndef ORGANCONTROLLER_H
#define ORGANCONTROLLER_H

#include <QObject>
#include <QString>
#include "organs/organworkmode.h"
#include "organs/organsenums.h"
#include "organs/organbuttondef.h"
#include <QList>
class OrganController : public QObject
{
    Q_OBJECT

public:
    explicit OrganController(
        organsEnums::Organ id,
        QObject* parent = nullptr
        );

    ~OrganController() override;

    organsEnums::Organ id() const;
    virtual bool isInstalled() const = 0;

    virtual bool isSelected() const = 0;
    virtual void setSelected(bool selected) = 0;

    virtual bool isTransitioning() const = 0;
    virtual bool isInHomeState() const = 0;
    virtual bool isInWorkingState() const = 0;

    virtual void requestHomeState() = 0;
    virtual void forceSafeState() = 0;
    virtual void updateTargetFromWorkMode( const OrganWorkMode &mode ) = 0;
    virtual void stopAllOutputs() = 0;
    virtual bool supportsDirection(organsEnums::Direction direction) const = 0;
    virtual void setManualDirection(organsEnums::Direction direction) = 0;
    virtual QList<OrganButtonDef> buttonDefinitions() const { return {}; }
    virtual void holdTick(organsEnums::Direction direction) { Q_UNUSED(direction); }
    virtual void toggleConfigSide(organsEnums::Direction direction);
    virtual void toggleMode(const QString& modeName);

protected:
    void publishStateChanged(
        int state
        );
    void publishModeChanged(
        const QString &mode,
        bool enabled
        );
    void publishMovementChanged(
        organsEnums::Direction direction,
        bool active
        );
signals:
    void stateChanged(
        organsEnums::Organ organ,
        int state
        );
    void modeChanged(
        organsEnums::Organ organ,
        const QString &mode,
        bool enabled
        );
    void movementChanged(
        organsEnums::Organ organ,
        organsEnums::Direction direction,
        bool active
        );
private:
    organsEnums::Organ m_id;

};

#endif // ORGANCONTROLLER_H
