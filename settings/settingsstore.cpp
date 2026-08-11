#include "settingsstore.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <cstdlib>

SettingsStore::SettingsStore(QSettings *settings)
    : m_settings(settings)
    , m_lockPath(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock")
{}

void SettingsStore::removeLockFile()
{
    if (QFile::exists(m_lockPath))
        QFile::remove(m_lockPath);
}

void SettingsStore::removeBadSettings()
{
    QDir dir(QCoreApplication::applicationDirPath(), {"settingsAutoCleaner.ini.*"});
    for (const QString &filename : dir.entryList())
        dir.remove(filename);
}

void SettingsStore::commit()
{
    m_settings->sync();
    std::system("sync");
    removeBadSettings();
}

void SettingsStore::commitGroup(const QString &group, std::function<void(QSettings*)> writer)
{
    removeLockFile();              // lock снимаем ДО записи — как в исходном коде
    m_settings->beginGroup(group);
    writer(m_settings);
    m_settings->endGroup();
    commit();
}
