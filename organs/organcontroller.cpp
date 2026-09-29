#include "organs/organcontroller.h"

OrganController::OrganController(organsEnums::Organ id, QObject* parent): QObject(parent), m_id(id)
{
}

OrganController::~OrganController() = default;


void OrganController::publishStateChanged(
    int state
    )
{
    emit stateChanged(
        id(),
        state
        );
}

void OrganController::publishModeChanged(
    const QString &mode,
    bool enabled
    )
{
    emit modeChanged(
        id(),
        mode,
        enabled
        );
}

void OrganController::publishMovementChanged(
    organsEnums::Direction direction,
    bool active
    )
{
    emit movementChanged(
        id(),
        direction,
        active
        );
}

organsEnums::Organ OrganController::id() const
{
    return m_id;
}
void OrganController::toggleConfigSide(organsEnums::Direction direction)
{
    Q_UNUSED(direction);
}

void OrganController::toggleMode(const QString& modeName)
{
    Q_UNUSED(modeName);
}
