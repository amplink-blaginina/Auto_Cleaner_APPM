#ifndef ORGANCOORDINATOR_H
#define ORGANCOORDINATOR_H

#include <QObject>

#include "organregistry.h"
#include "organworkmode.h"

class OrganCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit OrganCoordinator(
        OrganRegistry *registry,
        QObject *parent = nullptr
        );

public slots:
    void applyWorkMode(
        const OrganWorkMode &mode
        );

    void requestAllHome();
    void stopAllOutputs();

    bool isAnyTransitioning() const;
    bool areAllInHomeState() const;

private:
    OrganRegistry *m_registry = nullptr;
};

#endif // ORGANCOORDINATOR_H
