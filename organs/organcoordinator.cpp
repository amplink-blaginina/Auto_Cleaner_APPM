#include "organcoordinator.h"
#include "centralbroom.h"
#include "frontrail.h"
#include "blower.h"
#include "backmagnet.h"

#include <QMetaEnum>

OrganCoordinator::OrganCoordinator(
    OrganRegistry *registry,
    QObject *parent
    )
    : QObject(parent)
    , m_registry(registry)
{
    if (m_registry == nullptr) {
        return;
    }

    for (OrganController *organ : m_registry->all()) {
        if (organ == nullptr) {
            continue;
        }

        connect(
            organ,
            &OrganController::stateChanged,
            this,
            &OrganCoordinator::organStateChanged
            );
        connect(
            organ,
            &OrganController::modeChanged,
            this,
            &OrganCoordinator::organModeChanged
            );
        connect(
            organ,
            &OrganController::movementChanged,
            this,
            &OrganCoordinator::organMovementChanged
            );
    }
}

void OrganCoordinator::applyWorkMode(
    const OrganWorkMode &mode
    )
{
    if (m_registry == nullptr) {
        return;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ == nullptr) {
            continue;
        }

        organ->updateTargetFromWorkMode(mode);
    }
}

void OrganCoordinator::requestAllHome()
{
    if (m_registry == nullptr) {
        return;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ == nullptr) {
            continue;
        }

        organ->requestHomeState();
    }
}

void OrganCoordinator::stopAllOutputs()
{
    if (m_registry == nullptr) {
        return;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ == nullptr) {
            continue;
        }

        organ->stopAllOutputs();
    }
}

bool OrganCoordinator::isAnyTransitioning() const
{
    if (m_registry == nullptr) {
        return false;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ != nullptr
            && organ->isTransitioning()) {
            return true;
        }
    }

    return false;
}
bool OrganCoordinator::areAllStopped() const
{
    if (m_registry == nullptr) {
        return false;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ != nullptr
            && (organ->isInWorkingState()
                || organ->isTransitioning())) {
            return false;
        }
    }

    return true;
}

bool OrganCoordinator::areAllInHomeState() const
{
    if (m_registry == nullptr) {
        return true;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ != nullptr
            && !organ->isInHomeState()) {
            return false;
        }
    }

    return true;
}

void OrganCoordinator::forceAllSafeStates()
{
    if (m_registry == nullptr) {
        return;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ == nullptr) {
            continue;
        }

        organ->forceSafeState();
    }
}

bool OrganCoordinator::areAllIdle() const
{
    if (m_registry == nullptr) {
        return false;
    }

    for (OrganController *organ : m_registry->installed()) {
        if (organ == nullptr) {
            continue;
        }

        if (organ->isInWorkingState()
            || organ->isTransitioning()) {
            return false;
        }
    }

    return true;
}

QString OrganCoordinator::organName(
    organsEnums::Organ organ
    ) const
{
    switch (organ) {
    case organsEnums::BroomBlock:
        return QStringLiteral("BroomBlock");

    case organsEnums::Broom:
        return QStringLiteral("Broom");

    case organsEnums::Blower:
        return QStringLiteral("Blower");

    case organsEnums::Dump:
        return QStringLiteral("Dump");

    case organsEnums::BackMagnet:
        return QStringLiteral("BackMagnet");
    }

    return QStringLiteral("UnknownOrgan");
}

QString OrganCoordinator::directionName(
    organsEnums::Direction direction
    ) const
{
    switch (direction) {
    case organsEnums::Up:
        return QStringLiteral("Up");

    case organsEnums::Down:
        return QStringLiteral("Down");

    case organsEnums::Left:
        return QStringLiteral("Left");

    case organsEnums::Right:
        return QStringLiteral("Right");

    case organsEnums::None:
    default:
        return QStringLiteral("None");
    }
}

QString OrganCoordinator::stateName(
    organsEnums::Organ organ,
    int state
    ) const
{
    const char *key = nullptr;

    switch (organ) {
    case organsEnums::Broom:
        key = QMetaEnum::fromType<CentralBroom::BroomStates>()
                  .valueToKey(state);
        break;

    case organsEnums::Dump:
        key = QMetaEnum::fromType<FrontRail::FrontRailStates>()
                  .valueToKey(state);
        break;

    case organsEnums::Blower:
        key = QMetaEnum::fromType<Blower::BlowerStates>()
                  .valueToKey(state);
        break;

    case organsEnums::BackMagnet:
        key = QMetaEnum::fromType<BackMagnet::BackMagnetStates>()
                  .valueToKey(state);
        break;

    case organsEnums::BroomBlock:
    default:
        break;
    }

    if (key == nullptr) {
        return QStringLiteral("UnknownState(%1)")
        .arg(state);
    }

    return QString::fromLatin1(key);
}
