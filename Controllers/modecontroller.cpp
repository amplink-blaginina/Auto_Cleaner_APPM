#include "modecontroller.h"

#include <Controllers/viewcontroller.h>
#include <organs/organ.h>

#include <QPushButton>
#include <QSettings>
#include <QWidget>

namespace {
const char *modeKey = "Global/machineMode";
}

ModeController::ModeController(const MachineProfile &profile, QSettings *settings, ViewController *view,
                               std::function<bool()> canSwitch, QObject *parent)
    : QObject(parent), _profile(profile), _settings(settings), _view(view), _canSwitch(std::move(canSwitch))
{
    _current = qMax(0, _profile.modeIndex(_settings->value(modeKey).toString()));
}

void ModeController::addOrgan(const OrganSlot &slot){
    _organs.append(slot);
}

void ModeController::setButton(QPushButton *button){
    _button = button;
    _button->setVisible(_profile.modes.size() > 1);
    connect(_button, &QPushButton::clicked, this, &ModeController::next);
}

void ModeController::apply(){
    const MachineProfile::Mode &current = mode();
    for (const OrganSlot &slot : _organs){
        const bool available = current.organs.contains(slot.id);
        slot.organ->setAvailable(available);
        if (!available)
            slot.organ->clearSelection();
        for (QWidget *widget : slot.widgets)
            widget->setVisible(available);
    }
    if (_button)
        _button->setText("Режим:\n" + current.name);
    refresh();
    emit modeChanged();
}

void ModeController::next(){
    if (_profile.modes.size() < 2)
        return;
    if (!_canSwitch()){
        _view->addLog("Режим можно сменить только вне уборки, когда все органы дома");
        return;
    }
    _current = (_current + 1) % _profile.modes.size();
    _settings->setValue(modeKey, mode().id);
    _view->addLog("Режим: " + mode().name);
    apply();
}

void ModeController::refresh(){
    if (_button && !_button->isHidden() && _button->isEnabled() != _canSwitch())
        _button->setEnabled(_canSwitch());
}
