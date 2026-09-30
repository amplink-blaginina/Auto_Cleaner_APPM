#ifndef MODECONTROLLER_H
#define MODECONTROLLER_H

#include <QList>
#include <QObject>
#include <functional>

#include <machine/machineprofile.h>

class Organ;
class QPushButton;
class QSettings;
class QWidget;
class ViewController;

// Режим работы машины (например, лето/зима) по профилю машины. Органы, которых нет в текущем режиме,
// не выходят из дома, их выбор снимается, кнопки и иконки скрыты. Сменить режим можно только вне уборки,
// когда все органы дома. Выбранный режим запоминается в настройках.
class ModeController : public QObject
{
    Q_OBJECT
public:
    struct OrganSlot {
        QString id;              // как в профиле: "dump", "centralBroom"...
        Organ *organ;
        QList<QWidget *> widgets;// кнопки и иконки органа на экране
    };

    ModeController(const MachineProfile &profile, QSettings *settings, ViewController *view,
                   std::function<bool()> canSwitch, QObject *parent);

    void addOrgan(const OrganSlot &slot);
    void setButton(QPushButton *button);// кнопка смены режима; видна, только если режимов больше одного
    void apply();  // применить текущий режим к органам и экрану
    void next();   // перейти к следующему режиму (кнопка)
    void refresh();// доступность кнопки смены режима

    const MachineProfile &profile() const { return _profile; }
    const MachineProfile::Mode &mode() const { return _profile.modes[_current]; }

signals:
    void modeChanged();

private:
    MachineProfile _profile;
    QSettings *_settings;
    ViewController *_view;
    std::function<bool()> _canSwitch;
    QList<OrganSlot> _organs;
    QPushButton *_button = nullptr;
    int _current = 0;
};

#endif // MODECONTROLLER_H
