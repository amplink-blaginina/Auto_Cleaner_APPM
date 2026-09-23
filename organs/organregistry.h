#ifndef ORGANREGISTRY_H
#define ORGANREGISTRY_H

#include <QHash>
#include <QList>

#include "organcontroller.h"

class OrganRegistry
{
public:
    void add(OrganController *organ);

    OrganController *find(
        organsEnums::Organ id
    ) const;

    QList<OrganController *> all() const;

    QList<OrganController *> installed() const;

private:
    QHash<organsEnums::Organ, OrganController *> m_organs;
};

#endif // ORGANREGISTRY_H
