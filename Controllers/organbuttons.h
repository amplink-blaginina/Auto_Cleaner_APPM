#ifndef ORGANBUTTONS_H
#define ORGANBUTTONS_H

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

class QLabel;
class QPushButton;
class MachineContext;
class Organ;
class ViewController;

// Кнопки одного органа. Экранная кнопка и кнопка пульта работают одинаково. Общие правила:
// - пока орган в движении, кнопки не работают («… в движении, ожидайте»);
// - во время уборки кнопка управляет органом: сразу при нажатии или только после удержания;
// - вне уборки кнопка меняет выбор оператора (сторону, плавание и т.п.);
// - иконка нажатия ставится сразу, после отпускания - иконка по состоянию органа.
// Конкретный орган описывает свои кнопки и иконки (organpanels.h).
class OrganButtons : public QObject
{
    Q_OBJECT
public:
    struct Button {
        QPushButton *screen = nullptr;
        std::function<bool()> physical;      // кнопка на пульте нажата
        bool onlyWhenEnabled = false;        // работает только во время уборки (экранная кнопка доступна)
        bool blockWhileMoving = true;        // не работает, пока орган в движении
        QLabel *icon = nullptr;              // иконка на время нажатия
        QString pressedIcon;
        bool pressedIconWhileCleaning = true;// false - во время уборки иконка всё время по состоянию органа
        std::function<void()> onPress;       // во время уборки: при нажатии
        std::function<void()> onRelease;     // во время уборки: при отпускании (если нажатие принято)
        std::function<QString()> holdHint;   // во время уборки: подсказка при нажатии; onHeld - после удержания
        std::function<void()> onHeld;
        float holdSec = 3;
        std::function<void()> onSelect;      // вне уборки: выбор оператора
    };

    OrganButtons(const QString &busyMessage, Organ *organ, MachineContext *context, ViewController *view, QObject *parent);

    void addButton(const Button &button);
    void addIcon(QLabel *label, std::function<QString()> style);// иконка по состоянию органа

    void update();                   // опрос кнопок, каждый такт
    void click(QPushButton *screen); // щелчок по экранной кнопке (короткое нажатие могло проскочить между опросами)
    void setWorking(bool working);   // уборка началась/закончилась: кнопки «только во время уборки» доступны
    void refreshIcons();             // иконки по состоянию органа (кроме нажатых кнопок)
    bool isPressed() const;          // нажата любая кнопка органа

protected:
    ViewController *view;
    MachineContext *context;

private:
    struct ButtonState {
        Button button;
        bool pressed = false;
        bool accepted = false;// нажатие принято - орган не был в движении
        bool held = false;
        QElapsedTimer since;
    };
    bool showsPressedIcon(const ButtonState &state) const;
    void press(ButtonState &state);
    void release(ButtonState &state);

    QString _busyMessage;
    Organ *_organ;
    QVector<ButtonState> _buttons;
    QVector<QPair<QLabel *, std::function<QString()>>> _icons;
    bool _initialized = false;
};

#endif // ORGANBUTTONS_H
