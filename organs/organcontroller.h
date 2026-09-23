#ifndef ORGANCONTROLLER_H
#define ORGANCONTROLLER_H

#include <QObject>
#include "organs/organworkmode.h"
#include "organs/organsenums.h"

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
    virtual void updateTargetFromWorkMode(
        const OrganWorkMode &mode
        )
    {
        Q_UNUSED(mode);
    }

    virtual void stopAllOutputs() = 0;

    virtual bool supportsDirection(organsEnums::Direction direction) const = 0;

    virtual void setManualDirection(organsEnums::Direction direction) = 0;

private:
    organsEnums::Organ m_id;
};

#endif // ORGANCONTROLLER_H
