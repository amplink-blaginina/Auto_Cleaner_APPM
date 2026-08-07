#ifndef SETTINGSREADER_H
#define SETTINGSREADER_H

#include <qsettings.h>
#include <qvariant.h>


class SettingsReader
{
public:
    SettingsReader(QSettings *settings);
    QVariant readSettingsValue(QString name);
    void setDefaults();
    void updateStartDate(QDate value);
private:
    QSettings *_settings;
    QMap<QString, QVariant> _defaultValues;
};

#endif // SETTINGSREADER_H
