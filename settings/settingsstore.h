#ifndef SETTINGSSTORE_H
#define SETTINGSSTORE_H

#include <QSettings>
#include <QString>
#include <functional>

class SettingsStore
{
public:
    SettingsStore(QSettings *settings);
    // Единственная правильная последовательность записи настроек:
    // снять протухший lock -> записать группу -> sync -> системный sync -> почистить битые копии
    void commitGroup(const QString &group, std::function<void(QSettings*)> writer);

    // Полный сброс на диск без записи значений (для saveSystemConfigure)
    void commit();

    // Удаляет файлы вида settingsAutoCleaner.ini.Zht231
    void removeBadSettings();

private:
    void removeLockFile();

    QSettings *m_settings;
    const QString m_lockPath;
};

#endif // SETTINGSSTORE_H
