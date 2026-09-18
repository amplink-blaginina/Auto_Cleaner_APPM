#include "organs/organcontroller.h"

OrganController::OrganController(organsEnums::Organ id, QObject* parent): QObject(parent), m_id(id)
{
}

OrganController::~OrganController() = default;

organsEnums::Organ OrganController::id() const
{
    return m_id;
}
