#ifndef MACHINEPROFILE_H
#define MACHINEPROFILE_H

#include <QString>
#include <QStringList>
#include <QVector>

// Профиль машины: какие режимы работы есть (например, лето/зима) и какие органы доступны в каждом.
// Читается из machineProfile.json рядом с программой; без файла - встроенный профиль АППМ
// (один режим со всеми органами), поведение как до появления профилей.
//
// Пример файла:
// {
//     "machine": "Уборочная машина",
//     "modes": [
//         { "id": "summer", "name": "Лето", "organs": ["centralBroom", "blower", "magnet"] },
//         { "id": "winter", "name": "Зима", "organs": ["dump", "centralBroom"] }
//     ]
// }
// Органы - типы, реализованные в программе: knownOrgans().
struct MachineProfile
{
    struct Mode {
        QString id;          // для сохранения выбранного режима в настройках
        QString name;        // на экране и в журнале
        QStringList organs;
    };

    QString machine;
    QVector<Mode> modes;

    static QStringList knownOrgans();
    static MachineProfile appm();
    // warnings - что в файле не так (неизвестный орган, нет режимов); профиль при этом всё равно рабочий
    static MachineProfile load(const QString &path, QStringList *warnings);

    int modeIndex(const QString &id) const;// -1, если такого режима нет
};

#endif // MACHINEPROFILE_H
