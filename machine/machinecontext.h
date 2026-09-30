#ifndef MACHINECONTEXT_H
#define MACHINECONTEXT_H

class SettingsReader;

// Общее состояние машины, которое нужно органам. Органы видят машину только через этот интерфейс
// (и MachineIo), а не через MainWindow: так орган не зависит от экрана и переносится на другую машину.
// Свой выбор (сторона, плавание и т.п.) орган хранит сам и сообщает об изменениях сигналами.
class MachineContext
{
public:
    virtual ~MachineContext() = default;

    virtual bool isCleaning() const = 0;                // уборка запущена
    virtual int sweepType() const = 0;                  // тип смёта: задаёт обороты двигателя и скорость органов
    virtual SettingsReader *settingsReader() const = 0; // настройки (таймауты, обороты и т.п.)
};

#endif // MACHINECONTEXT_H
