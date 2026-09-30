#ifndef ORGANPANELS_H
#define ORGANPANELS_H

#include "organbuttons.h"

class Blower;
class CentralBroom;
class FrontRail;
class PhysicalButtonManager;

// Кнопки и иконки органов на главном экране: что делает каждая кнопка и какие картинки показывать.
// Общие правила (удержание, блокировка во время движения, иконки) - в OrganButtons.

// виджеты органа на экране
struct OrganWidgets
{
    QPushButton *up;
    QPushButton *down;
    QPushButton *left;
    QPushButton *right;
    QLabel *vertIcon;// вверх/вниз
    QLabel *sideIcon;// сторона
    QPushButton *flow = nullptr;
    QPushButton *press = nullptr;
    QLabel *flowIcon = nullptr;// плавание (и прижим)
};

class DumpButtons : public OrganButtons
{
    Q_OBJECT
public:
    DumpButtons(const OrganWidgets &w, FrontRail *dump, PhysicalButtonManager *pult,
                MachineContext *context, ViewController *view, QObject *parent);
private:
    void setFlow(bool state);
    FrontRail *_dump;
};

class BroomButtons : public OrganButtons
{
    Q_OBJECT
public:
    BroomButtons(const OrganWidgets &w, CentralBroom *broom, PhysicalButtonManager *pult,
                 MachineContext *context, ViewController *view, QObject *parent);
private:
    void setFlow(bool state);
    CentralBroom *_broom;
};

class BlowerButtons : public OrganButtons
{
    Q_OBJECT
public:
    BlowerButtons(const OrganWidgets &w, Blower *blower, PhysicalButtonManager *pult,
                  MachineContext *context, ViewController *view, QObject *parent);
};

#endif // ORGANPANELS_H
