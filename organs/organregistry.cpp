#include "organregistry.h"

void OrganRegistry::add(OrganController *organ)
{
    if (organ == nullptr) {
        return;
    }

    m_organs.insert(
        organ->id(),
        organ
    );
}

OrganController *OrganRegistry::find(
    organsEnums::Organ id
) const
{
    return m_organs.value(
        id,
        nullptr
    );
}

QList<OrganController *> OrganRegistry::all() const
{
    return m_organs.values();
}

QList<OrganController *> OrganRegistry::installed() const
{
    QList<OrganController *> result;

    for (OrganController *organ : m_organs) {
        if (organ != nullptr
            && organ->isInstalled()) {
            result.append(organ);
        }
    }

    return result;
}
