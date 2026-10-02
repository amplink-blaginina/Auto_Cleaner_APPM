// serviceDVRform — экран «РЕГИСТРАТОР»: вкладка «Записи» (плеер с перемоткой и скоростью) и вкладка «Камеры»
//                  (живые картинки, параметры камеры, назначение роли, сброс, камеры «без роли»)
// версия: Регистратор 2.1 (RPI-RES_260929_55), 2026-09-29; RPi 4B, Raspbian 12 armhf, Qt 5.15.8, GStreamer 1.22
// изменения к 2.0 (RPI-RES_260929_21): доступность экрана и вкладок - availability() по status.json службы
//         v27 (RPI-RES_260929_54): нет камер и нет записей - кнопка меню заблокирована (SettingsMainRightForm);
//         нет флешки - вкладка «ДИСКА НЕТ», флешка пустая - «ЗАПИСЕЙ НЕТ», нет камер - «КАМЕР НЕТ» (заблокированы);
//         по умолчанию - «Записи», если записи есть, иначе «Камеры»; при пропаже - переключение на доступную вкладку
// изменения к версии 1 (RPI-RES_260928_51): две вкладки по макету RPI-RES_260928_63; плеер - пауза, ±10 с,
//         пред./след. файл той же камеры, скорость x0.25..x4, ползунок перемотки, файлы камеры подряд;
//         индикатор записи и накопителя из /run/amplink-dvr/status.json; вкладка «Камеры» через amplink-camctl
//
// записи пишет служба amplink-dvr (RPI-RES_260928_25): /media/amplink-dvr/DVR/<камера>/<дата>/<камера>_<дата_время>.ts
// камеры: IP ролей и пароль - из файлов регистратора; поиск, параметры, настройка, сброс - утилита amplink-camctl
// видеоядро: на вкладке «Записи» работает только плеер, на вкладке «Камеры» - только живые потоки (без ISP)
#ifndef SERVICEDVRFORM_H
#define SERVICEDVRFORM_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QList>
#include <QStringList>
#include <QListWidgetItem>
#include <QProcess>
#include <QJsonObject>
#include <QMap>

class MainWindow;
class CameraView;
class GstPlayer;
class DvrFilePlayer;

namespace Ui {
class ServiceDVRForm;
}

// запускается из настроек как «GPIO ПУ»: new ServiceDVRForm(_mainWindow, _parent), show(), raise();
// удаляется сама при закрытии, все потоки видео и служебные процессы при этом останавливаются
class ServiceDVRForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceDVRForm(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~ServiceDVRForm();

    // доступность регистратора: для кнопки меню (SettingsMainRightForm) и вкладок этого экрана
    struct Availability
    {
        bool service;               // служба amplink-dvr работает (status.json свежий)
        bool cameras;               // хоть одна камера на связи (роли или «без роли»)
        bool storage;               // накопитель смонтирован
        bool records;               // на накопителе есть хоть один ролик
    };
    static Availability availability();
    static bool hasRecordFiles();   // первый *.ts в каталоге записей (без службы)

    QTimer mainProgressTimer;       // 250 мс: положение ползунка, время
    QTimer statusTimer;             // 2 с: индикатор записи; 10 с: список роликов / поиск камер
    QTimer confirmTimer;            // 3 с: окно подтверждения опасных действий

    MainWindow *_mainWindow;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;    // нажатие на плитку камеры

private slots:
    void mainProgress();
    void statusProgress();
    void playerFinished(bool ok);
    void camctlFinished(int exitCode, QProcess::ExitStatus status);
    void confirmTimeout();

    void on_pushButton_tabRecords_clicked();
    void on_pushButton_tabCameras_clicked();
    void on_listWidget_records_itemClicked(QListWidgetItem *item);
    void on_pushButton_play_clicked();
    void on_pushButton_back10_clicked();
    void on_pushButton_fwd10_clicked();
    void on_pushButton_prev_clicked();
    void on_pushButton_next_clicked();
    void on_pushButton_speed025_clicked();
    void on_pushButton_speed050_clicked();
    void on_pushButton_speed100_clicked();
    void on_pushButton_speed200_clicked();
    void on_pushButton_speed400_clicked();
    void on_slider_position_sliderPressed();
    void on_slider_position_sliderReleased();
    void on_pushButton_roleLeft_clicked();
    void on_pushButton_roleRight_clicked();
    void on_pushButton_roleRear_clicked();
    void on_pushButton_apply_clicked();
    void on_pushButton_reset_clicked();
    void on_pushButton_exit_clicked();

private:
    // одна плитка вкладки «Камеры»: 3 роли + 1 место для камеры «без роли»
    struct Tile
    {
        QString role;               // left / right / rear; пусто - камера без роли
        QString ip;
        bool online;
        bool live;                  // идет живой поток
        QWidget *place;
        QLabel *label;              // IP и роль поверх картинки
        QLabel *state;              // «нет связи» и т.п. по центру плитки
        CameraView *view;
        GstPlayer *player;
    };

    // ---- общее ----
    void readConfig();                                          // IP ролей и пароль из файлов регистратора
    QString cameraUrl(const QString &ip);
    void showTab(bool records);
    void setButtonStyle(QPushButton *btn, const QString &kind, bool on);
    void applyAvailability();                                   // вкладки: доступность, текст, переключение

    // ---- вкладка «Записи» ----
    QStringList findRecords();
    void fillRecords();
    QString recordTitle(const QString &fileName);
    QString cameraTitle(const QString &name);
    QString roleAccusative(const QString &name);           // «левую», «правую», «заднюю»
    QString neighbour(const QString &fileName, int step);       // соседний файл той же камеры
    void playRecord(const QString &fileName, qint64 startNs = 0);
    void selectListItem(const QString &fileName);
    void setSpeed(double rate);
    void updatePlayButtons();

    // ---- вкладка «Камеры» ----
    void startLive(Tile &t);
    void stopLive(Tile &t);
    void stopAllLive();
    void updateTiles();
    void selectTile(int index);
    void runCamctl(const QString &command, const QStringList &args);
    void handleCamctl(const QString &command, const QJsonObject &res);
    void showCameraInfo(const QJsonObject &info);
    void clearCameraInfo(const QString &message);
    void updateRoleButtons();
    void setCamMessage(const QString &text, bool error = false);

    Ui::ServiceDVRForm *ui;
    QWidget *_parent;

    QMap<QString, QString> roleIp;  // left/right/rear -> IP
    QString credentials;            // user:password из cameras.cred
    bool recordsTab;

    // «Записи»
    CameraView *playerView;
    DvrFilePlayer *filePlayer;
    QStringList records;            // полные пути в порядке ленты (новые первыми)
    QString currentRecord;
    double speed;
    bool sliderDragging;
    bool storageOk;

    // «Камеры»
    QList<Tile> tiles;
    int selectedTile;
    QString targetRole;             // выбранная кнопкой роль для «Настроить как …»
    QJsonObject selectedInfo;       // последний ответ camctl info по выбранной камере
    QProcess *camctl;
    QString camctlCommand;          // что сейчас выполняется: scan / info / activate / setup / reset / alias
    QString camctlIp;
    QString pendingConfirm;         // "apply" / "reset" - ждем повторного нажатия
    bool aliasOn;                   // временный адрес 192.168.1.2/24 для камер в заводской подсети
    int statusTicks;

    // доступность вкладок
    bool recordsAvail;
    bool camerasAvail;
    bool scanDone;                  // был хоть один ответ camctl scan (до него плитки ролей условно на связи)
    qint64 camJobUntilMs;           // до этого времени камеры считаем доступными: перезагрузка после настройки/сброса
};

#endif // SERVICEDVRFORM_H
