#ifndef MAINTENANCE_TRACKER_H
#define MAINTENANCE_TRACKER_H

#include <QDate>
#include <QList>
#include <QString>
#include <QtGlobal>

class SettingsReader;
class SettingsStore;

class MaintenanceTracker
{
public:
    enum class CounterSource
    {
        Engine,
        System
    };

    struct MaintenanceRule
    {
        QString key;              // ключ в settings: EngineOil, Hydraulic...
        QString displayName;      // текст для лога
        CounterSource source;     // от какого счётчика считать ресурс
        quint32 intervalSeconds;  // допустимый интервал
        quint32 lastServiceValue; // счётчик на момент последнего ТО
        bool alarmActive = false; // чтобы не логировать каждую секунду
    };

    explicit MaintenanceTracker(SettingsReader *reader,
                                SettingsStore *store);

    // Вызывать один раз при запуске после создания объекта.
    void load();

    // Здесь перечисляем регламенты ТО.
    // Это замена MainWindow::insertValues().
    void createRules();

    // Вызывать раз в секунду из MainWindow::oneSecond().
    void tick(bool engineRunning, bool systemRunning,
              const QDate &currentDate);

    // Вызывать перед выключением ПВИ.
    void saveNow();

    quint32 engineSeconds() const;
    quint32 systemSeconds() const;
    quint32 engineSecondsToday() const;

    QList<QString> takeNewDueMessages();
    bool hasDueMaintenance() const;

private:
    void processNewDay(const QDate &currentDate);
    void saveCountersIfNeeded();
    void checkMaintenance();

    quint32 sourceValue(CounterSource source) const;

    SettingsReader *m_reader = nullptr;
    SettingsStore *m_store = nullptr;

    quint32 m_engineSeconds = 0;
    quint32 m_systemSeconds = 0;

    quint32 m_lastSavedEngineSeconds = 0;
    quint32 m_lastSavedSystemSeconds = 0;

    quint32 m_engineSecondsToday = 0;
    QDate m_currentDay;

    QList<MaintenanceRule> m_rules;
    QList<QString> m_newDueMessages;

    static constexpr quint32 SaveIntervalSeconds = 5 * 60;
};

#endif
