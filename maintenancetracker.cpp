#include "maintenancetracker.h"
#include "settingsreader.h"

MaintenanceTracker::MaintenanceTracker(SettingsReader *settingsReader, SettingsStore *settingsStore) {
    m_reader = settingsReader;
    m_store = settingsStore;
}

void MaintenanceTracker::createRules(){
    m_rules.clear();

    const auto addRule =
        [this](const QString &key,
               const QString &displayName,
               CounterSource source)
    {
        MaintenanceRule rule;
        rule.key = key;
        rule.displayName = displayName;
        rule.source = source;

        // Значения будут загружены позже в load():
        // TO/<key>     -> intervalSeconds
        // TOCur/<key>  -> lastServiceValue
        rule.intervalSeconds = 0;
        rule.lastServiceValue = 0;
        rule.alarmActive = false;

        m_rules.append(rule);
    };

    addRule(
        "EngineOil",
        "Масло и охлаждающая жидкость двигателя",
        CounterSource::Engine
        );

    addRule(
        "HydraulicOilCheck",
        "Проверка масла гидросистемы",
        CounterSource::System
        );

    addRule(
        "CarLubrication",
        "Смазка машины",
        CounterSource::System
        );

    addRule(
        "CarTightening",
        "Затяжка резьбовых соединений",
        CounterSource::System
        );

    addRule(
        "EngineTO",
        "ТО двигателя",
        CounterSource::System
        );

    addRule(
        "PressureFilterChange",
        "Замена напорного фильтра",
        CounterSource::System
        );
}
// void MaintenanceTracker::createRules()
// {
//     m_rules = {
//         {
//             "EngineOil",
//             "Замена масла двигателя",
//             CounterSource::Engine,
//             0,    // реальное значение подгрузим из settings в load()
//             0,
//             false
//         },

//         {
//             "PneumaticCheck",
//             "Проверка пневмосистемы",
//             CounterSource::System,
//             0,
//             0,
//             false
//         },

//         // Далее переносим остальные позиции из insertValues().
//     };
// }

void MaintenanceTracker::load()
{
    m_engineSeconds =
        m_reader->readSettingsValue("TOCur/Engine").toUInt();

    m_lastSavedEngineSeconds = m_engineSeconds;

    m_systemSeconds =
        m_reader->readSettingsValue("TOCur/System").toUInt();

    m_lastSavedSystemSeconds = m_systemSeconds;

    m_engineSecondsToday =
        m_reader->readSettingsValue("TOCur/EngineToday").toUInt();

    m_currentDay =
        m_reader->readSettingsValue("TOCur/DateToday").toDate();

    for (MaintenanceRule &rule : m_rules)
    {
        rule.intervalSeconds =
            m_reader->readSettingsValue("TO/" + rule.key).toUInt();

        rule.lastServiceValue =
            m_reader->readSettingsValue("TOCur/" + rule.key).toUInt();
    }
}
