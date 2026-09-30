#include "backmagnet.h"

#include <settingsreader.h>
#include <Controllers/viewcontroller.h>

#include <QDebug>
#include <QMetaEnum>

BackMagnet::BackMagnet(const MachineIo &machine, MachineContext *context, ViewController *logger_, QObject *parent)
    : Organ("Магнит", machine, context, logger_, parent)
{
    readSettings();

    OrganSequence::Step down;
    down.out = {{"опускание", "Опускаем магнит", [this]{ goDown(); }, nullptr,// вниз концевика нет - по времени
                 [this]{ return timeouts.value(BackMagnetDownOut); }}};
    down.in = {{"подъём", "Поднимаем магнит", [this]{ goUp(); },
                [this]{ return io->get(StateDKPBackMagnetUp).toBool(); },
                [this]{ return timeouts.value(BackMagnetDownIn); }}};
    down.done = [this]{ logger->addLog("Магнит опущен"); };
    sequence.addStep(down);

    sequence.setHalt([this]{ goOff(); });
    sequence.setHome([this]{ logger->addLog("Магнит поднят"); });
    sequence.setOnChange([this](int s){ qDebug() << "Back magnet state " << toString(BackMagnetStates(s)); });
    goHome();
}

void BackMagnet::readSettings(){
    timeouts.clear();
    SettingsReader* reader = _context->settingsReader();
    // назначаем таймауты на длительные операции
    timeouts.insert(BackMagnetDownOut, reader->readSettingsValue("BackMagnet/timeouts.BackMagnetDownOut").toInt());
    timeouts.insert(BackMagnetDownIn, reader->readSettingsValue("BackMagnet/timeouts.BackMagnetDownIn").toInt());
}

QString BackMagnet::toString(BackMagnetStates s){
    const char *key = QMetaEnum::fromType<BackMagnetStates>().valueToKey(s);
    return key ? QString::fromLatin1(key) : QStringLiteral("UnknownState");
}

void BackMagnet::toggleSelected(){
    _selected = !_selected;
    emit selectionChanged();
}

void BackMagnet::clearSelection(){
    if (!_selected)
        return;
    _selected = false;
    emit selectionChanged();
}

void BackMagnet::goOff()
{
    io->set(StateValveE2, false);
    io->set(StateValveE6, false);
    hydraulics->request(this, false);// магнит стоит - гидравлика ему не нужна
}

void BackMagnet::goUp()
{
    hydraulics->request(this, true);
    io->set(StateValveE2, true);
    io->set(StateValveE6, false);
}

void BackMagnet::goDown()
{
    hydraulics->request(this, true);
    io->set(StateValveE2, false);
    io->set(StateValveE6, true);
}
