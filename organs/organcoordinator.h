#ifndef ORGANCOORDINATOR_H
#define ORGANCOORDINATOR_H

#include <QObject>
#include <QString>
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
    bool isAnyTransitioning() const;
    bool areAllStopped() const;
    bool areAllInHomeState() const;
    bool areAllIdle() const;
    QString organName(
        organsEnums::Organ organ
        ) const;

    QString stateName(
        organsEnums::Organ organ,
        int state
        ) const;
    QString directionName(
        organsEnums::Direction direction
        ) const;
public slots:
    void applyWorkMode(
        const OrganWorkMode &mode
        );

    void requestAllHome();
    void stopAllOutputs();
    void forceAllSafeStates();

signals:
    void organStateChanged(
        organsEnums::Organ organ,
        int state
        );
    void organModeChanged(
        organsEnums::Organ organ,
        const QString &mode,
        bool enabled
        );
    void organMovementChanged(
        organsEnums::Organ organ,
        organsEnums::Direction direction,
        bool active
        );
private:
    OrganRegistry *m_registry = nullptr;
};

#endif // ORGANCOORDINATOR_H
