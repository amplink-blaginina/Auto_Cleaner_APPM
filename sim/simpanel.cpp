#include "simpanel.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QScrollArea *scrollable(QWidget *content){
    auto *area = new QScrollArea;
    area->setWidget(content);
    area->setWidgetResizable(true);
    return area;
}
}

SimPanel::SimPanel(SimIoBus *io, const QList<SimSignalInfo> &signalList, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , _io(io)
{
    setWindowTitle("Симуляция машины");
    resize(900, 700);

    QList<SimSignalInfo> inputs, outputs;
    for (const SimSignalInfo &info : signalList)
        (info.input ? inputs : outputs).append(info);

    auto *columns = new QHBoxLayout;
    auto *left = new QVBoxLayout;
    left->addWidget(createAxes());
    left->addWidget(createOutputs(outputs), 1);
    columns->addLayout(left, 1);
    columns->addWidget(createInputs(inputs), 1);
    setLayout(columns);

    connect(&_timer, &QTimer::timeout, this, &SimPanel::refresh);
    _timer.start(100);
    refresh();
}

QWidget *SimPanel::createAxes(){
    auto *box = new QGroupBox("Органы (модель машины)");
    auto *form = new QFormLayout(box);
    for (const SimAxis &axis : _io->axes()){
        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setMinimumWidth(220);
        bar->setFormat(axis.minName + " %p% " + axis.maxName);
        _axisBars.append(bar);
        auto *side = new QLabel;// поворот: текущая сторона
        side->setMinimumWidth(side->fontMetrics().horizontalAdvance("ПРАВАЯ ▶") + 16);
        side->setAlignment(Qt::AlignCenter);
        side->setVisible(axis.sides);
        _axisSides.append(side);
        auto *row = new QHBoxLayout;
        row->addWidget(bar, 1);
        row->addWidget(side);
        form->addRow(axis.name, row);
    }
    for (const SimRotor &rotor : _io->rotors()){
        auto *bar = new QProgressBar;
        bar->setRange(0, 100);
        bar->setMinimumWidth(220);
        _rotorBars.append(bar);
        form->addRow(rotor.name, bar);
    }
    return box;
}

QWidget *SimPanel::createOutputs(const QList<SimSignalInfo> &outputs){
    auto *box = new QGroupBox("Выходы (включает программа)");
    auto *content = new QWidget;
    auto *form = new QFormLayout(content);
    for (const SimSignalInfo &info : outputs){
        auto *value = new QLabel;
        value->setMinimumWidth(50);
        _outputs.append({info.signal, value});
        form->addRow(info.name, value);
    }
    auto *layout = new QVBoxLayout(box);
    layout->addWidget(scrollable(content));
    return box;
}

QWidget *SimPanel::createInputs(const QList<SimSignalInfo> &inputs){
    auto *box = new QGroupBox("Входы (датчики)");
    auto *layout = new QVBoxLayout(box);

    auto *model = new QCheckBox("Концевики положения задаёт модель (снять - задавать вручную, как без датчиков)");
    model->setChecked(_io->isModelEnabled());
    connect(model, &QCheckBox::toggled, this, [this](bool on){ _io->setModelEnabled(on); });
    layout->addWidget(model);

    auto *content = new QWidget;
    auto *form = new QFormLayout(content);
    for (const SimSignalInfo &info : inputs){
        const DeviceStates signal = info.signal;
        if (info.analog){
            auto *spin = new QSpinBox;
            spin->setRange(0, 65535);
            spin->setValue(_io->value(signal).toInt());
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this,
                    [this, signal](int v){ _io->setInput(signal, v); });
            _inputs.append({signal, nullptr, spin});
            form->addRow(info.name, spin);
        }
        else{
            auto *check = new QCheckBox;
            check->setChecked(_io->value(signal).toBool());
            connect(check, &QCheckBox::clicked, this,
                    [this, signal](bool on){ _io->setInput(signal, on); });
            _inputs.append({signal, check, nullptr});
            form->addRow(info.name, check);
        }
    }
    layout->addWidget(scrollable(content), 1);
    return box;
}

void SimPanel::refresh(){
    const QList<SimAxis> &axes = _io->axes();
    for (int i = 0; i < axes.size() && i < _axisBars.size(); ++i){
        const SimAxis &axis = axes[i];
        QProgressBar *bar = _axisBars[i];
        const int percent = qRound(axis.pos * 100);
        bar->setValue(percent);
        if (!axis.sides)
            continue;
        // поворот: сторона меняется при переходе через середину
        const QString color = percent < 50 ? "#3a7bd5" : percent > 50 ? "#e08a1e" : "";
        const QString style = color.isEmpty() ? "" : "QProgressBar { text-align: center; } QProgressBar::chunk { background: " + color + "; }";
        if (bar->styleSheet() != style)
            bar->setStyleSheet(style);
        QLabel *side = _axisSides[i];
        side->setText(percent < 50 ? "◀ ЛЕВАЯ" : percent > 50 ? "ПРАВАЯ ▶" : "середина");
        const QString sideStyle = color.isEmpty() ? "" : "background: " + color + "; color: white; font-weight: bold;";
        if (side->styleSheet() != sideStyle)
            side->setStyleSheet(sideStyle);
    }

    const QList<SimRotor> &rotors = _io->rotors();
    for (int i = 0; i < rotors.size() && i < _rotorBars.size(); ++i){
        const SimRotor &rotor = rotors[i];
        _rotorBars[i]->setValue(qRound(rotor.speed));
        _rotorBars[i]->setFormat("%p%  (задано " + QString::number(qRound(rotor.command())) + "%)");
    }

    for (const OutputView &view : _outputs){
        const QVariant v = _io->value(view.signal);
        const bool on = v.toInt() != 0;
        view.value->setText(v.toString());
        view.value->setStyleSheet(on ? "background: #3c3; color: black; font-weight: bold;" : "");
    }

    // концевики, которые ведёт модель, показываем как есть и не даём менять руками
    for (const InputView &view : _inputs){
        const bool byModel = _io->isModelEnabled() && _io->isModelSensor(view.signal);
        if (view.check){
            view.check->setEnabled(!byModel);
            view.check->setChecked(_io->value(view.signal).toBool());
        }
        else if (!view.spin->hasFocus()){
            view.spin->blockSignals(true);
            view.spin->setValue(_io->value(view.signal).toInt());
            view.spin->blockSignals(false);
        }
    }
}
