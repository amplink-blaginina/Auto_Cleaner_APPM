#ifndef SIMPANEL_H
#define SIMPANEL_H

#include <QList>
#include <QTimer>
#include <QWidget>

#include "sim/simiobus.h"

class QCheckBox;
class QLabel;
class QProgressBar;
class QSpinBox;

// Описание сигнала для панели симуляции (берётся из таблицы сигналов MainWindow::addElement)
struct SimSignalInfo
{
    DeviceStates signal;
    QString name;
    bool input;
    bool analog;
};

// Окно симуляции: положения органов, выходы (что включила программа) и входы (задаёт человек)
class SimPanel : public QWidget
{
    Q_OBJECT
public:
    SimPanel(SimIoBus *io, const QList<SimSignalInfo> &signalList, QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    QWidget *createAxes();
    QWidget *createOutputs(const QList<SimSignalInfo> &outputs);
    QWidget *createInputs(const QList<SimSignalInfo> &inputs);

    struct OutputView { DeviceStates signal; QLabel *value; };
    struct InputView { DeviceStates signal; QCheckBox *check; QSpinBox *spin; };

    SimIoBus *_io;
    QList<QProgressBar *> _axisBars;
    QList<QLabel *> _axisSides;
    QList<QProgressBar *> _rotorBars;
    QList<OutputView> _outputs;
    QList<InputView> _inputs;
    QTimer _timer;
};

#endif // SIMPANEL_H
