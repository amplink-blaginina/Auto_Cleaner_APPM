#ifndef SettingsWifiLEFTFORM_H
#define SettingsWifiLEFTFORM_H

#include <QWidget>
#include <QDateTime>
#include <QLabel>

#include <interface_button/interfacebutton.h>

namespace Ui {
class SettingsWifiLeftForm;
}

struct WifiNetwork {
    bool inUse;
    QString ssid;
    int signal;
    QString security;
};

class SettingsWifiLeftForm : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWifiLeftForm(QWidget *parent = nullptr);
    ~SettingsWifiLeftForm();

    QVector<WifiNetwork> wifiList;

    QWidget* parent;

private slots:
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_0_clicked();
    void on_pushButton_backspace_clicked();
    void on_pushButton_save_clicked();
    void scanWifi();
    void refreshVersions();
    void on_pushButton_refresh_clicked();

    void on_pushButton_install_clicked();

    void on_pushButton_restore_clicked();

    void on_pushButton_versionsRefresh_clicked();

private:
    Ui::SettingsWifiLeftForm *ui;
};

#endif // SettingsWifiLEFTFORM_H
