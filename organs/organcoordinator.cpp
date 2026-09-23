#include "organcoordinator.h"

OrganCoordinator::OrganCoordinator(
    OrganRegistry *registry,
    QObject *parent
    )
    : QObject(parent)
    , m_registry(registry)
{
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
