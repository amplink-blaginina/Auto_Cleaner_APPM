/*
 * Журнал ТО — экранная клавиатура для полей ввода модуля
 * Версия: 01, 2026-09-29
 * Зависимости: Qt 5.15 widgets; шрифт Manrope (загружается экраном журнала)
 *
 * В Auto_Cleaner нет текстовой экранной клавиатуры (только цифровая Password_Form), поэтому модуль несёт свою.
 * Открывается поверх окна поля (нижняя часть экрана), над клавишами — строка с редактируемым текстом,
 * поэтому поле может оказаться под клавиатурой. Раскладки: РУС / ENG / 123. Нажатия уходят в поле как
 * QKeyEvent — работают maxLength, textEdited и textChanged, как с физической клавиатурой.
 * Закрытие: «ГОТОВО», нажатие на затемнённую часть экрана, закрытие/удаление поля.
 */
#ifndef TOJOURNALKEYBOARD_H
#define TOJOURNALKEYBOARD_H

#include <QWidget>
#include <QPointer>
#include <QVector>

class QLineEdit;
class QLabel;
class QPushButton;
class QGridLayout;

namespace ToJ
{

class Keyboard : public QWidget
{
    Q_OBJECT
public:
    static void open(QLineEdit* edit);
    static void closeCurrent();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    enum Layout { LayoutRu, LayoutEn, LayoutDigits };

    explicit Keyboard(QLineEdit* edit, QWidget* parent);

    void build();
    void sendText(const QString& text);
    void sendKey(int key);
    void updatePreview();
    void finish();

    QPointer<QLineEdit> edit;
    QWidget*            panel = nullptr;
    QLabel*             preview = nullptr;
    QWidget*            keysBox = nullptr;
    QVector<QPushButton*> letterKeys;
    QPushButton*        shiftKey = nullptr;
    Layout              layout = LayoutRu;
    Layout              lastLetters = LayoutRu;
    bool                shift = false;

    static QPointer<Keyboard> current;
};

} // namespace ToJ

#endif // TOJOURNALKEYBOARD_H
