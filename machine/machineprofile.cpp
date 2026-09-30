#include "machineprofile.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QStringList MachineProfile::knownOrgans(){
    return {"dump", "centralBroom", "blower", "magnet"};
}

MachineProfile MachineProfile::appm(){
    MachineProfile profile;
    profile.machine = "АППМ";
    profile.modes.append({"work", "Уборка", knownOrgans()});
    return profile;
}

MachineProfile MachineProfile::load(const QString &path, QStringList *warnings){
    QFile file(path);
    if (!file.exists())
        return appm();
    if (!file.open(QIODevice::ReadOnly)){
        warnings->append("Профиль машины: не удалось открыть " + path + ", используется профиль АППМ");
        return appm();
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()){
        warnings->append("Профиль машины: ошибка в файле (" + error.errorString() + "), используется профиль АППМ");
        return appm();
    }

    MachineProfile profile;
    const QJsonObject root = doc.object();
    profile.machine = root.value("machine").toString();
    for (const QJsonValue &value : root.value("modes").toArray()){
        const QJsonObject object = value.toObject();
        Mode mode;
        mode.id = object.value("id").toString();
        mode.name = object.value("name").toString(mode.id);
        for (const QJsonValue &organ : object.value("organs").toArray()){
            const QString id = organ.toString();
            if (knownOrgans().contains(id) && !mode.organs.contains(id))
                mode.organs.append(id);
            else if (!knownOrgans().contains(id))
                warnings->append("Профиль машины: неизвестный орган \"" + id + "\" в режиме " + mode.name);
        }
        if (mode.id.isEmpty() || profile.modeIndex(mode.id) >= 0){
            warnings->append("Профиль машины: у режима " + mode.name + " нет id или он повторяется, режим пропущен");
            continue;
        }
        profile.modes.append(mode);
    }
    if (profile.modes.isEmpty()){
        warnings->append("Профиль машины: нет ни одного режима, используется профиль АППМ");
        return appm();
    }
    return profile;
}

int MachineProfile::modeIndex(const QString &id) const{
    for (int i = 0; i < modes.size(); ++i)
        if (modes[i].id == id)
            return i;
    return -1;
}
