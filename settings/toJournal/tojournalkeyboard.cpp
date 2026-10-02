/*
 * Журнал ТО — экранная клавиатура (см. tojournalkeyboard.h)
 * Версия: 01, 2026-09-29
 */
#include "tojournalkeyboard.h"

#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace ToJ
{

QPointer<Keyboard> Keyboard::current;

static const int kPanelHeight = 336;

static const char* kRu[3] = {"йцукенгшщзхъ", "фывапролджэ", "ячсмитьбюё"};
static const char* kEn[3] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
static const char* kDigits[3] = {"1234567890", "-/:;()№\"+", ".,?!'*_#%"};

void Keyboard::open(QLineEdit* edit)
{
    if (!edit)
        return;
    closeCurrent();
    QWidget* win = edit->window();
    Keyboard* k = new Keyboard(edit, win);
    current = k;
    k->setGeometry(win->rect());
    k->show();
    k->raise();
    edit->setFocus(Qt::MouseFocusReason);
    edit->end(false);
}

void Keyboard::closeCurrent()
{
    if (current)
        current->finish();
}

Keyboard::Keyboard(QLineEdit* edit_, QWidget* parent) :
    QWidget(parent),
    edit(edit_)
{
    setAttribute(Qt::WA_StyledBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("tojKeyboard");
    setStyleSheet(
        "QWidget#tojKeyboard { background: rgba(0,0,0,110); }"
        "QWidget#tojKbPanel { background:#15191f; border-top:2px solid #2a2f37; }"
        "QWidget#tojKbKeys { background:transparent; }"
        "QLabel#tojKbPreview { background:#1d2127; color:#ffffff; font-family:\"Manrope\"; font-size:18px;"
        "  border-radius:22px; padding:0 22px; }"
        "QPushButton { background:#1d2127; color:#e8e9ea; font-family:\"Manrope\"; font-size:20px;"
        "  border:none; border-radius:8px; }"
        "QPushButton:pressed { background:#3a414c; }"
        "QPushButton[func=\"true\"] { background:#262b33; font-size:15px; font-weight:bold; }"
        "QPushButton[func=\"true\"]:pressed { background:#3a414c; }"
        "QPushButton[active=\"true\"] { color:#2fcb59; }"
        "QPushButton#tojKbDone { background:#2fcb59; color:#0d1117; font-size:15px; font-weight:bold; }"
        "QPushButton#tojKbDone:pressed { background:#27a94a; }");

    panel = new QWidget(this);
    panel->setObjectName("tojKbPanel");
    panel->setAttribute(Qt::WA_StyledBackground);

    QVBoxLayout* v = new QVBoxLayout(panel);
    v->setContentsMargins(12, 10, 12, 10);
    v->setSpacing(8);

    preview = new QLabel(panel);
    preview->setObjectName("tojKbPreview");
    preview->setFixedHeight(46);
    preview->setTextFormat(Qt::PlainText);
    v->addWidget(preview);

    keysBox = new QWidget(panel);
    keysBox->setObjectName("tojKbKeys");
    v->addWidget(keysBox, 1);

    if (edit)
    {
        connect(edit, &QLineEdit::textChanged, this, &Keyboard::updatePreview);
        connect(edit, &QObject::destroyed, this, [this]() { close(); });
        preview->setToolTip(QString());
    }
    // латиница, если поле уже начато латиницей
    if (edit && !edit->text().isEmpty())
    {
        QChar c = edit->text().at(0);
        if (c.isLetter() && c.unicode() < 0x80)
            layout = lastLetters = LayoutEn;
    }
    shift = edit && edit->text().isEmpty();
    build();
    updatePreview();
}

void Keyboard::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    panel->setGeometry(0, height() - kPanelHeight, width(), kPanelHeight);
}

void Keyboard::mousePressEvent(QMouseEvent* e)
{
    // нажатие на затемнённую часть — закрыть клавиатуру (промежутки между клавишами панели не закрывают)
    if (!panel->geometry().contains(e->pos()))
        finish();
}

void Keyboard::finish()
{
    if (edit)
    {
        edit->deselect();
        edit->clearFocus();
    }
    hide();
    close();
}

void Keyboard::updatePreview()
{
    QString t = edit ? edit->text() : QString();
    if (t.isEmpty() && edit)
        preview->setText(edit->placeholderText());
    else
        preview->setText(t + QStringLiteral("|"));
    preview->setStyleSheet(t.isEmpty() ? "color:#6b6f75;" : "");
}

void Keyboard::sendText(const QString& text)
{
    if (!edit)
        return;
    QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, text);
    QApplication::sendEvent(edit, &press);
    if (shift && layout != LayoutDigits)
    {
        shift = false;
        build();
    }
}

void Keyboard::sendKey(int key)
{
    if (!edit)
        return;
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(edit, &press);
}

void Keyboard::build()
{
    // пересобрать клавиши под текущую раскладку
    // старые клавиши — через deleteLater: build() вызывается из обработчика нажатой клавиши
    for (QWidget* w : keysBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
    {
        w->hide();
        w->deleteLater();
    }
    delete keysBox->layout();
    letterKeys.clear();

    QGridLayout* g = new QGridLayout(keysBox);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(6);
    g->setVerticalSpacing(6);
    const int cols = 26;             // сетка в полклавиши — ряды разной длины центрируются
    for (int c = 0; c < cols; ++c)
        g->setColumnStretch(c, 1);

    auto makeKey = [this](const QString& text, bool func) -> QPushButton*
    {
        QPushButton* b = new QPushButton(text, keysBox);
        b->setFocusPolicy(Qt::NoFocus);
        b->setMinimumHeight(52);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (func)
            b->setProperty("func", true);
        return b;
    };

    const char** rows = layout == LayoutRu ? kRu : layout == LayoutEn ? kEn : kDigits;
    for (int r = 0; r < 3; ++r)
    {
        QString chars = QString::fromUtf8(rows[r]);
        int n = chars.size();
        int reserve = (r == 2) ? 3 : 0;                    // место под «⌫» (и «⇧» слева) в 3-м ряду
        int start = qMax(0, (cols - 2 * n - 2 * reserve) / 2) + reserve;
        if (r == 2)
        {
            if (layout != LayoutDigits)
            {
                shiftKey = makeKey(QStringLiteral("⇧"), true);
                shiftKey->setProperty("active", shift);
                connect(shiftKey, &QPushButton::clicked, this, [this]() { shift = !shift; build(); });
                g->addWidget(shiftKey, r, 0, 1, 3);
            }
            QPushButton* bs = makeKey(QStringLiteral("⌫"), true);
            bs->setAutoRepeat(true);
            bs->setAutoRepeatDelay(400);
            bs->setAutoRepeatInterval(80);
            connect(bs, &QPushButton::clicked, this, [this]() { sendKey(Qt::Key_Backspace); });
            g->addWidget(bs, r, cols - 3, 1, 3);
        }
        for (int i = 0; i < n; ++i)
        {
            QString ch = chars.mid(i, 1);
            if (shift && layout != LayoutDigits)
                ch = ch.toUpper();
            QPushButton* b = makeKey(ch, false);
            connect(b, &QPushButton::clicked, this, [this, ch]() { sendText(ch); });
            g->addWidget(b, r, start + 2 * i, 1, 2);
            letterKeys.append(b);
        }
    }

    // нижний ряд: 123/АБВ, РУС/ENG, очистить, пробел, «-», «.», ГОТОВО
    QPushButton* mode = makeKey(layout == LayoutDigits ? (lastLetters == LayoutRu ? QStringLiteral("АБВ") : QStringLiteral("ABC"))
                                                       : QStringLiteral("123"), true);
    connect(mode, &QPushButton::clicked, this, [this]()
    {
        layout = layout == LayoutDigits ? lastLetters : LayoutDigits;
        build();
    });
    g->addWidget(mode, 3, 0, 1, 3);

    QPushButton* lang = makeKey(lastLetters == LayoutRu ? QStringLiteral("ENG") : QStringLiteral("РУС"), true);
    connect(lang, &QPushButton::clicked, this, [this]()
    {
        lastLetters = lastLetters == LayoutRu ? LayoutEn : LayoutRu;
        layout = lastLetters;
        build();
    });
    g->addWidget(lang, 3, 3, 1, 3);

    QPushButton* clear = makeKey(QStringLiteral("СТЕРЕТЬ"), true);
    connect(clear, &QPushButton::clicked, this, [this]()
    {
        if (!edit || edit->text().isEmpty())
            return;
        edit->selectAll();
        sendKey(Qt::Key_Backspace);
        shift = true;
        build();
    });
    g->addWidget(clear, 3, 6, 1, 3);

    QPushButton* space = makeKey(QString(), false);
    connect(space, &QPushButton::clicked, this, [this]() { sendText(QStringLiteral(" ")); });
    g->addWidget(space, 3, 9, 1, 10);

    QPushButton* dash = makeKey(QStringLiteral("-"), false);
    connect(dash, &QPushButton::clicked, this, [this]() { sendText(QStringLiteral("-")); });
    g->addWidget(dash, 3, 19, 1, 2);

    QPushButton* done = makeKey(QStringLiteral("ГОТОВО"), true);
    done->setObjectName("tojKbDone");
    connect(done, &QPushButton::clicked, this, &Keyboard::finish);
    g->addWidget(done, 3, 21, 1, 5);
}

} // namespace ToJ
