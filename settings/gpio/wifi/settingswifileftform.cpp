#include "settingswifileftform.h"
#include "ui_settingswifileftform.h"

#include "mainwindow.h"

SettingsWifiLeftForm::SettingsWifiLeftForm(QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::SettingsWifiLeftForm)
{
    ui->setupUi(this);

    parent = parent_;

    ui->label_password->setText("");
    scanWifi();
}

SettingsWifiLeftForm::~SettingsWifiLeftForm()
{
    delete ui;
}

void SettingsWifiLeftForm::on_pushButton_1_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "1");
}

void SettingsWifiLeftForm::on_pushButton_2_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "2");
}

void SettingsWifiLeftForm::on_pushButton_3_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "3");
}

void SettingsWifiLeftForm::on_pushButton_4_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "4");
}

void SettingsWifiLeftForm::on_pushButton_5_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "5");
}

void SettingsWifiLeftForm::on_pushButton_6_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "6");
}

void SettingsWifiLeftForm::on_pushButton_7_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "7");
}

void SettingsWifiLeftForm::on_pushButton_8_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "8");
}

void SettingsWifiLeftForm::on_pushButton_9_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "9");
}

void SettingsWifiLeftForm::on_pushButton_0_clicked()
{
    if (ui->label_password->text().length() < 15)
        ui->label_password->setText(ui->label_password->text() + "0");
}

void SettingsWifiLeftForm::on_pushButton_backspace_clicked()
{
    if (ui->label_password->text().length() > 0)
        ui->label_password->setText(ui->label_password->text().left(ui->label_password->text().length() - 1));
}

void SettingsWifiLeftForm::on_pushButton_save_clicked()
{
    QModelIndex index = ui->listView_wifis->currentIndex();

    if (index.isValid() && ui->label_password->text().length() > 0)
    {
        QString ssid = index.data(Qt::UserRole).toString();
        QProcess p;
        // присоединяемся к точке
        p.start("nmcli", {
                    "device", "wifi", "connect",
                    ssid,
                    "password", ui->label_password->text()
                });
        p.waitForFinished();
        // меняем метрику
        p.start("nmcli connection modify " + ssid + " ipv4.route-metric 50");
        p.waitForFinished();
        // обновляем параметры соединения
        p.start("nmcli connection up " + ssid);
        p.waitForFinished();

        refreshVersions();
    }
}

void SettingsWifiLeftForm::on_pushButton_refresh_clicked()
{
    scanWifi();
}

void SettingsWifiLeftForm::scanWifi()
{
    wifiList.clear();

    QProcess proc;
    proc.start("nmcli", {
                   "-t",
                   "-f", "IN-USE,SSID,SIGNAL,SECURITY",
                   "dev", "wifi", "rescan"
               });
    proc.waitForFinished();
    proc.start("nmcli", {
                   "-t",
                   "-f", "IN-USE,SSID,SIGNAL,SECURITY",
                   "dev", "wifi", "list"
               });
    proc.waitForFinished();

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QStringList parts = line.split(':');

        // nmcli иногда отдаёт кривые строки
        if (parts.size() < 4)
            continue;

        WifiNetwork net;
        net.inUse    = (parts[0] == "*");
        net.ssid     = parts[1];
        net.signal   = parts[2].toInt();
        net.security = parts[3];

        // скрытые сети можно пропустить
        if (net.ssid.isEmpty())
            continue;

        wifiList.append(net);
    }
    QStandardItemModel *model = new QStandardItemModel(this);

    for (const auto &net : wifiList) {
        QString text = net.ssid;

        text += QString("  (%1%)").arg(net.signal);

        if (net.security.isEmpty())
            text += " (free)";
        else
            text += "";

        if (net.inUse)
            text = "✔️ " + text;

        QStandardItem *item = new QStandardItem;

        item->setText(text);

        item->setData(net.ssid, Qt::UserRole);
        item->setData(net.inUse, Qt::UserRole+1);

        model->appendRow(item);
    }

    ui->listView_wifis->setModel(model);
}

void SettingsWifiLeftForm::on_pushButton_install_clicked()
{
    QModelIndex index = ui->listView_versions->currentIndex();

    if (index.isValid())
    {
        QString version_to_work = index.data(Qt::UserRole).toString();
        QString dir = "/home/knight/nextcloud_update/Update/APPM/" + version_to_work;

        QProcess p;
        p.setWorkingDirectory(dir);
        p.start("/bin/bash", QStringList() << "./updater.sh");
        p.waitForFinished(1000 * 120);
        //p.start("umount /home/knight/nextcloud_update");
        //p.waitForFinished();
        //system("sync");
        //system("reboot");
    }
}

void SettingsWifiLeftForm::on_pushButton_restore_clicked()
{
    QModelIndex index = ui->listView_versions->currentIndex();

    if (index.isValid())
    {
        QString version_to_work = index.data(Qt::UserRole).toString();
        QProcess p;
        p.setWorkingDirectory("/home/knight/nextcloud_update/Update/APPM/" + version_to_work);
        p.start("./restorer.sh");
        p.waitForFinished(1000 * 120);
        //p.start("umount /home/knight/nextcloud_update");
        //p.waitForFinished();
        //system("sync");
        //system("reboot");
    }
}

void SettingsWifiLeftForm::refreshVersions()
{
    ui->label_status->setText("");
    QProcess p;
    p.start("umount /home/knight/nextcloud_update");
    p.waitForFinished(1000 * 120);
    // монтируем каталог
    p.start("/bin/bash", QStringList() << "-c" << "rm -f /var/run/mount.davfs/home-knight-nextcloud_update.pid");
    p.waitForFinished(1000 * 120);
    QString command = "printf \"Update\\njaxdo2-kobvep-deqkoP\\ny\\n\" | mount -t davfs https://nc.amplink.ru/remote.php/dav/files/Update /home/knight/nextcloud_update";
    p.start("/bin/bash", QStringList() << "-c" << command);
    p.waitForFinished(1000 * 120);
    // проверяем что смонтировалось
    QStringList versions;
    QDir dir("/home/knight/nextcloud_update/Update/APPM/");
    // Проверяем, существует ли папка и примонтирована ли она
    if (!dir.exists()) {
        qDebug() << "Путь не найден. Возможно, монтирование не удалось.";
    }
    else
    {
        QProcess p;
        p.setWorkingDirectory("/home/knight/nextcloud_update/Update/serials/");
        p.start("./get_serial.sh");
        p.waitForFinished();
        ui->label_serial->setText("Серийный номер: " + p.readAllStandardOutput());
        // Устанавливаем фильтры: только папки, без "." и ".."
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        // Можно отсортировать версии (например, по имени или дате создания)
        dir.setSorting(QDir::Name | QDir::Reversed);
        versions = dir.entryList();
        if (versions.size() != 0)
        {// есть версии
            QStandardItemModel *model = new QStandardItemModel(this);
            for (const QString &line : versions)
            {
                QStandardItem *item = new QStandardItem;

                item->setText(line);
                item->setData(line, Qt::UserRole);

                model->appendRow(item);
            }

            ui->listView_versions->setModel(model);
        }
    }
}

void SettingsWifiLeftForm::on_pushButton_versionsRefresh_clicked()
{
    refreshVersions();
}
