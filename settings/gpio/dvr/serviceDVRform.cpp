// serviceDVRform — экран «РЕГИСТРАТОР»: вкладка «Записи» и вкладка «Камеры»
// версия: Регистратор 2.1 (RPI-RES_260929_55), 2026-09-29; изменения - см. serviceDVRform.h
#include "serviceDVRform.h"
#include "ui_serviceDVRform.h"

#include "mainwindow.h"
#include "camera/cameraview.h"
#include "camera/cameraplayer.h"
#include "dvrfileplayer.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <QScroller>
#include <QScrollBar>
#include <QEvent>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHostAddress>
#include <QDebug>

// где регистратор хранит записи и свои настройки (служба amplink-dvr)
#define DVR_RECORDS_DIR  "/media/amplink-dvr/DVR"
#define DVR_STATUS_FILE  "/run/amplink-dvr/status.json"
#define DVR_CONF_FILE    "/etc/amplink-dvr/amplink-dvr.conf"
#define DVR_CRED_FILE    "/etc/amplink-dvr/cameras.cred"
#define DVR_CAMCTL       "/opt/amplink-dvr/amplink-camctl.py"
// если файла с паролем нет - пароль как в mainwindow ttt (камеры настроены RPI-RES_260925_24)
#define DVR_DEFAULT_CRED "admin:admin123456$"
// сколько последних роликов показывать в ленте
#define DVR_MAX_RECORDS  1000
// status.json старше - служба остановлена или зависла (пишет раз в 2 с)
#define DVR_STATUS_MAX_AGE_S  30
// после настройки/сброса камера перезагружается и меняет адрес - вкладку «Камеры» не блокируем
#define DVR_CAM_JOB_GRACE_MS  180000
// длина сегмента регистратора - если длительность файла еще не известна
#define DVR_SEGMENT_NS   60000000000LL

// стили кнопок (как на макете RPI-RES_260928_63)
#define ACC_BG  "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgb(247,197,72), stop:1 rgb(240,128,52))"
#define BTN_BASE "QPushButton { border:none; border-radius:%1px; font: 600 %2px \"Montserrat\"; "

ServiceDVRForm::ServiceDVRForm(MainWindow* mainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServiceDVRForm)
{
    ui->setupUi(this);
    _mainWindow = mainWindow;
    _parent = parent;

    // экран одноразовый - при закрытии удаляется сам
    setAttribute(Qt::WA_DeleteOnClose);
    qDebug() << "DVR: экран открыт";

    speed = 1.0;
    sliderDragging = false;
    storageOk = false;
    recordsTab = true;
    selectedTile = -1;
    aliasOn = false;
    statusTicks = 0;
    recordsAvail = true;
    camerasAvail = true;
    scanDone = false;
    camJobUntilMs = 0;
    readConfig();

    // ---- вкладка «Записи»: плеер (кадр 1280x720 без ISP) и лента роликов ----
    playerView = new CameraView(ui->widget_player);
    playerView->resize(ui->widget_player->size());
    ui->label_player->raise();
    ui->label_speedBadge->raise();
    ui->label_noStorage->raise();
    ui->label_speedBadge->hide();
    filePlayer = new DvrFilePlayer(this);
    connect(filePlayer, &DvrFilePlayer::frameReady, playerView, &CameraView::setFrameData, Qt::QueuedConnection);
    connect(filePlayer, &DvrFilePlayer::finished, this, &ServiceDVRForm::playerFinished);

    ui->listWidget_records->setFlow(QListView::LeftToRight);
    ui->listWidget_records->setWrapping(false);
    ui->listWidget_records->setUniformItemSizes(true);
    ui->listWidget_records->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget_records->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(ui->listWidget_records->viewport(), QScroller::LeftMouseButtonGesture);

    // ---- вкладка «Камеры»: 3 плитки ролей + 1 для камеры без роли ----
    QWidget *places[4] = {ui->widget_tile1, ui->widget_tile2, ui->widget_tile3, ui->widget_tile4};
    QLabel *labels[4] = {ui->label_tile1, ui->label_tile2, ui->label_tile3, ui->label_tile4};
    QLabel *states[4] = {ui->label_tileState1, ui->label_tileState2, ui->label_tileState3, ui->label_tileState4};
    const char *roles[4] = {"left", "right", "rear", ""};
    for (int i = 0; i < 4; i++)
    {
        Tile t;
        t.role = roles[i];
        t.ip = roleIp.value(t.role);
        t.online = !t.role.isEmpty();       // до первого поиска считаем камеры ролей доступными
        t.live = false;
        t.place = places[i];
        t.label = labels[i];
        t.state = states[i];
        t.view = new CameraView(t.place);
        t.view->resize(t.place->size());
        t.view->hide();
        t.player = new GstPlayer(t.view, this);
        connect(t.player, &GstPlayer::frameReady, t.view, &CameraView::setFrameData, Qt::QueuedConnection);
        t.label->raise();
        t.state->raise();
        // нажатие на любую часть плитки - выбор камеры
        t.place->installEventFilter(this);
        t.label->installEventFilter(this);
        t.state->installEventFilter(this);
        tiles.append(t);
    }

    camctl = new QProcess(this);
    connect(camctl, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ServiceDVRForm::camctlFinished);

    confirmTimer.setSingleShot(true);
    connect(&confirmTimer, &QTimer::timeout, this, &ServiceDVRForm::confirmTimeout);
    connect(&mainProgressTimer, &QTimer::timeout, this, &ServiceDVRForm::mainProgress);
    connect(&statusTimer, &QTimer::timeout, this, &ServiceDVRForm::statusProgress);

    setSpeed(1.0);
    statusProgress();       // заодно applyAvailability(): тексты и блокировка вкладок
    // по умолчанию «Записи», если они есть; нет записей - «Камеры» (в меню экран недоступен, если нет ни того, ни другого)
    showTab(recordsAvail || !camerasAvail);

    mainProgressTimer.start(250);
    statusTimer.start(2000);
}

ServiceDVRForm::~ServiceDVRForm()
{
    mainProgressTimer.stop();
    statusTimer.stop();

    // потоки останавливаем до удаления окон, в которые они рисуют
    filePlayer->stop();
    stopAllLive();

    if (camctl->state() != QProcess::NotRunning)
    {// сюда попадаем только с поиском/чтением: на время настройки и сброса выход заблокирован
        camctl->kill();
        camctl->waitForFinished(1000);
    }
    if (aliasOn)// временный адрес заводской подсети снимаем
        QProcess::startDetached("python3", QStringList() << DVR_CAMCTL << "alias" << "off");

    delete ui;
    qDebug() << "DVR: экран закрыт";
}

// ============================================================ общее

void ServiceDVRForm::readConfig()
{// IP ролей и пароль - из файлов регистратора, чтобы менять в одном месте
    QFile conf(DVR_CONF_FILE);
    if (conf.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&conf);
        QString section;
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (line.startsWith("[") && line.endsWith("]"))
                section = line.mid(1, line.length() - 2);
            else if (section.startsWith("camera:") && line.startsWith("ip"))// [camera:left] ip = 192.168.0.12
                roleIp[section.mid(7)] = line.section('=', 1).trimmed();
        }
    }
    else
        qWarning() << "DVR: нет" << DVR_CONF_FILE << "- камеры по умолчанию";
    if (!roleIp.contains("left"))
        roleIp["left"] = "192.168.0.12";
    if (!roleIp.contains("right"))
        roleIp["right"] = "192.168.0.27";
    if (!roleIp.contains("rear"))
        roleIp["rear"] = "192.168.0.28";

    credentials = DVR_DEFAULT_CRED;
    QFile cred(DVR_CRED_FILE);
    if (cred.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString text = QString::fromUtf8(cred.readAll()).trimmed();
        if (text.contains(':'))
            credentials = text;
    }
}

QString ServiceDVRForm::cameraUrl(const QString &ip)
{// дополнительный поток 704x576 (для миниатюр достаточно); пароль с '$' кодируем для URL
    QString user = credentials.section(':', 0, 0);
    QString pass = credentials.section(':', 1);
    return "rtsp://" + QString(QUrl::toPercentEncoding(user)) + ":" + QString(QUrl::toPercentEncoding(pass)) +
           "@" + ip + ":554/cam/realmonitor?channel=1&subtype=1";
}

void ServiceDVRForm::setButtonStyle(QPushButton *btn, const QString &kind, bool on)
{// kind: tab - вкладка; btn - кнопка панели; red - сброс; confirm - подтверждение; dim - недоступно
    QString s;
    if (kind == "tab" && !btn->isEnabled())
        s = QString(BTN_BASE).arg(22).arg(16) + "color: rgb(110,116,120); background: rgb(31,38,43); }";// нет диска / камер
    else if (kind == "tab")
        s = QString(BTN_BASE).arg(22).arg(16) + (on ? QString("color: rgb(0,0,0); background: %1; }").arg(ACC_BG)
                                                     : QString("color: rgb(223,227,230); background: rgb(42,50,54); }"));
    else if (kind == "red")
        s = QString(BTN_BASE).arg(8).arg(14) + "color: rgb(255,154,157); background: rgb(74,36,38); border: 1px solid rgb(122,46,49); }";
    else if (kind == "confirm")
        s = QString(BTN_BASE).arg(8).arg(14) + "color: rgb(255,255,255); background: rgb(67,160,71); }";
    else if (kind == "dim")
        s = QString(BTN_BASE).arg(8).arg(14) + "color: rgb(110,116,120); background: rgb(31,38,43); }";
    else
        s = QString(BTN_BASE).arg(8).arg(14) + (on ? QString("color: rgb(0,0,0); background: %1; }").arg(ACC_BG)
                                                    : QString("color: rgb(255,255,255); background: rgb(31,38,43); }"));
    _mainWindow->getView()->setStyle(btn, s);
}

void ServiceDVRForm::showTab(bool records)
{// видеоядро делим между вкладками: на «Записях» только плеер, на «Камерах» только живые потоки
    recordsTab = records;
    ui->frame_pageRecords->setVisible(records);
    ui->frame_pageCameras->setVisible(!records);
    setButtonStyle(ui->pushButton_tabRecords, "tab", records);
    setButtonStyle(ui->pushButton_tabCameras, "tab", !records);

    if (records)
    {
        stopAllLive();
        if (aliasOn && camctl->state() == QProcess::NotRunning)
            runCamctl("alias", QStringList() << "off");
        fillRecords();
        updatePlayButtons();
    }
    else
    {
        filePlayer->stop();
        updatePlayButtons();
        updateTiles();
        if (selectedTile < 0)
            selectTile(0);
        else
            runCamctl("scan", QStringList());
    }
}

void ServiceDVRForm::statusProgress()
{// индикатор записи и накопителя - по файлу состояния службы регистратора
    QString text;
    bool error = false;
    QFile f(DVR_STATUS_FILE);
    if (f.open(QIODevice::ReadOnly))
    {
        const QJsonObject st = QJsonDocument::fromJson(f.readAll()).object();
        const QJsonObject storage = st.value("storage").toObject();
        const QString mode = st.value("mode").toString();
        storageOk = !storage.value("path").toString().isEmpty();
        const double freeGb = storage.value("free_mb").toDouble() / 1000.0;
        const double totalGb = storage.value("total_mb").toDouble() / 1000.0;
        if (mode == "recording")
            text = QString("<span style='color:#ff5a5f'>●</span> Запись · свободно %1 из %2 ГБ")
                       .arg(freeGb, 0, 'f', 1).arg(totalGb, 0, 'f', 1);
        else if (mode == "no_storage" || !storageOk)
        {
            text = "Нет накопителя — запись не ведётся";
            error = true;
        }
        else if (mode == "stopped_ignition")
            text = "Запись остановлена: зажигание выключено";
        else
            text = "Регистратор: " + mode;
    }
    else
    {
        text = "Служба регистратора не запущена";
        error = true;
        storageOk = QFileInfo(DVR_RECORDS_DIR).isDir();
    }
    ui->label_status->setTextFormat(Qt::RichText);
    _mainWindow->getView()->setText(ui->label_status, text);
    _mainWindow->getView()->setStyle(ui->label_status, error
        ? "color: rgb(255,154,157); font: 13px \"Montserrat\"; background: rgb(74,36,38); border-radius: 16px; padding: 0px 12px;"
        : "color: rgb(223,227,230); font: 13px \"Montserrat\"; background: rgb(42,50,54); border-radius: 16px; padding: 0px 12px;");

    applyAvailability();

    // раз в 10 с: лента роликов или поиск камер
    if (++statusTicks % 5 != 0 || !isVisible())
        return;
    if (recordsTab)
        fillRecords();
    else if (camctl->state() == QProcess::NotRunning)
        runCamctl("scan", QStringList());
}

ServiceDVRForm::Availability ServiceDVRForm::availability()
{// по файлу состояния службы (v26: any_camera, storage.has_records); без службы - только каталог записей
    Availability a = {false, false, false, false};
    QFile f(DVR_STATUS_FILE);
    if (f.open(QIODevice::ReadOnly))
    {
        const QJsonObject st = QJsonDocument::fromJson(f.readAll()).object();
        const QDateTime t = QDateTime::fromString(st.value("time").toString(), Qt::ISODate);
        a.service = t.isValid() && st.value("mode").toString() != "stopped" &&
                    qAbs(t.secsTo(QDateTime::currentDateTime())) < DVR_STATUS_MAX_AGE_S;
        if (a.service)
        {
            const QJsonObject storage = st.value("storage").toObject();
            a.storage = !storage.value("path").toString().isEmpty();
            if (st.contains("any_camera"))
                a.cameras = st.value("any_camera").toBool();
            else
            {// служба v25: признака нет - камера на связи, если пишется
                for (const QJsonValue &v : st.value("cameras").toArray())
                    if (v.toObject().value("state").toString() == "recording")
                        a.cameras = true;
            }
            a.records = a.storage && (storage.contains("has_records") ? storage.value("has_records").toBool()
                                                                      : hasRecordFiles());
        }
    }
    if (!a.service)
    {// служба не работает: камеры не проверить (их опрашивает служба), записи - по каталогу
        a.storage = QFileInfo(DVR_RECORDS_DIR).isDir();
        a.records = a.storage && hasRecordFiles();
    }
    return a;
}

bool ServiceDVRForm::hasRecordFiles()
{// ранний выход на первом ролике
    QDirIterator it(DVR_RECORDS_DIR, QStringList() << "*.ts", QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext();
}

void ServiceDVRForm::applyAvailability()
{
    const Availability a = availability();
    storageOk = a.storage;

    bool cams = a.cameras;
    if (scanDone)
    {// свой поиск на вкладке «Камеры» свежее данных службы
        for (const Tile &t : tiles)
            if (t.online && !t.ip.isEmpty())
                cams = true;
    }
    const bool camJob = camctlCommand == "setup" || camctlCommand == "reset" || camctlCommand == "activate";
    if (camJob || QDateTime::currentMSecsSinceEpoch() < camJobUntilMs)
        cams = true;// камеру перенастраивают - не выбрасываем из вкладки на время перезагрузки

    recordsAvail = a.storage && a.records;
    camerasAvail = cams;

    ui->pushButton_tabRecords->setText(!a.storage ? "ДИСКА НЕТ" : !a.records ? "ЗАПИСЕЙ НЕТ" : "ЗАПИСИ");
    ui->pushButton_tabRecords->setEnabled(recordsAvail);
    ui->pushButton_tabCameras->setText(camerasAvail ? "КАМЕРЫ" : "КАМЕР НЕТ");
    ui->pushButton_tabCameras->setEnabled(camerasAvail);

    // текущая вкладка пропала, а другая есть - переходим; пропало все - остаемся (подсказка на странице)
    // до show() не переключаем: стартовую вкладку выбирает конструктор
    if (isVisible() && recordsTab && !recordsAvail && camerasAvail)
        showTab(false);
    else if (isVisible() && !recordsTab && !camerasAvail && recordsAvail)
        showTab(true);
    else
    {
        setButtonStyle(ui->pushButton_tabRecords, "tab", recordsTab);
        setButtonStyle(ui->pushButton_tabCameras, "tab", !recordsTab);
    }
}

void ServiceDVRForm::on_pushButton_tabRecords_clicked()
{
    if (!recordsTab)
        showTab(true);
}

void ServiceDVRForm::on_pushButton_tabCameras_clicked()
{
    if (recordsTab)
        showTab(false);
}

void ServiceDVRForm::on_pushButton_exit_clicked()
{
    close();// деструктор остановит видео и служебные процессы
}

// ============================================================ вкладка «Записи»

QStringList ServiceDVRForm::findRecords()
{// DVR/<камера>/<дата>/<камера>_<ГГГГММДД_ЧЧММСС>.ts; сортировка по времени из имени, новые первыми
    QList<QPair<QString, QString>> found;  // (время, путь)
    QDir root(DVR_RECORDS_DIR);
    for (const QString &cam : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QDir camDir(root.filePath(cam));
        for (const QString &day : camDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            QDir dayDir(camDir.filePath(day));
            for (const QString &f : dayDir.entryList(QStringList() << "*.ts", QDir::Files))
                found.append(qMakePair(f.section('_', 1), dayDir.filePath(f)));
        }
    }
    std::sort(found.begin(), found.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
        return a.first > b.first;
    });

    QStringList result;
    for (int i = 0; i < found.size() && i < DVR_MAX_RECORDS; i++)
        result.append(found[i].second);
    return result;
}

void ServiceDVRForm::fillRecords()
{
    QStringList fresh = storageOk ? findRecords() : QStringList();
    // нет флешки - заглушка вместо плеера; флешка есть, но пусто - подсказка
    if (!storageOk)
        _mainWindow->getView()->setText(ui->label_noStorage, "Флешка не подключена\nВставьте USB-накопитель (ext4 или exFAT). Запись начнётся сама");
    else if (fresh.isEmpty())
        _mainWindow->getView()->setText(ui->label_noStorage, "Записей пока нет");
    ui->label_noStorage->setVisible(fresh.isEmpty() && !filePlayer->isPlaying());
    if (fresh == records)
        return;// ничего нового - ленту не трогаем, чтобы не сбить прокрутку

    const int scroll = ui->listWidget_records->horizontalScrollBar()->value();
    const bool atStart = scroll == 0;
    records = fresh;

    ui->listWidget_records->clear();
    for (const QString &path : records)
    {
        QListWidgetItem *item = new QListWidgetItem(recordTitle(path));
        item->setData(Qt::UserRole, path);
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(QSize(112, 72));
        ui->listWidget_records->addItem(item);
        if (path == currentRecord)
            item->setSelected(true);
    }
    // новые ролики появляются слева; если пользователь листал - оставляем его место
    if (!atStart)
        ui->listWidget_records->horizontalScrollBar()->setValue(scroll);
    // на eglfs рядом с QOpenGLWidget список сам не перерисовывался до касания - просим явно
    ui->listWidget_records->viewport()->update();
}

QString ServiceDVRForm::cameraTitle(const QString &name)
{
    if (name == "left")
        return "Левая";
    if (name == "right")
        return "Правая";
    if (name == "rear")
        return "Задняя";
    return name;
}

QString ServiceDVRForm::roleAccusative(const QString &name)
{// для подписей «настроить как левую»
    if (name == "left")
        return "левую";
    if (name == "right")
        return "правую";
    if (name == "rear")
        return "заднюю";
    return name;
}

QString ServiceDVRForm::recordTitle(const QString &fileName)
{// left_20260928_133417.ts -> "Левая\n13:34:17\n28.09"
    QString base = QFileInfo(fileName).completeBaseName();
    QString cam = base.section('_', 0, 0);
    QString date = base.section('_', 1, 1);
    QString time = base.section('_', 2, 2);
    if (date.length() != 8 || time.length() != 6)
        return base;
    return cameraTitle(cam) + "\n" +
           time.mid(0, 2) + ":" + time.mid(2, 2) + ":" + time.mid(4, 2) + "\n" +
           date.mid(6, 2) + "." + date.mid(4, 2);
}

QString ServiceDVRForm::neighbour(const QString &fileName, int step)
{// соседний по времени файл той же камеры: step -1 - раньше, +1 - позже
    const QString cam = QFileInfo(fileName).completeBaseName().section('_', 0, 0);
    QStringList same;
    for (int i = records.size() - 1; i >= 0; i--)// records - новые первыми, идем от старых
        if (QFileInfo(records[i]).completeBaseName().section('_', 0, 0) == cam)
            same.append(records[i]);
    const int idx = same.indexOf(fileName);
    if (idx < 0 || idx + step < 0 || idx + step >= same.size())
        return QString();
    return same[idx + step];
}

void ServiceDVRForm::selectListItem(const QString &fileName)
{
    for (int i = 0; i < ui->listWidget_records->count(); i++)
    {
        QListWidgetItem *item = ui->listWidget_records->item(i);
        if (item->data(Qt::UserRole).toString() == fileName)
        {
            ui->listWidget_records->setCurrentItem(item);
            ui->listWidget_records->scrollToItem(item);
            break;
        }
    }
}

void ServiceDVRForm::playRecord(const QString &fileName, qint64 startNs)
{
    currentRecord = fileName;
    const QString title = recordTitle(fileName);
    _mainWindow->getView()->setText(ui->label_player, title.section('\n', 0, 0) + " · " +
                                    title.section('\n', 2, 2) + " " + title.section('\n', 1, 1));
    _mainWindow->getView()->setText(ui->label_fileName, QFileInfo(fileName).fileName());
    ui->label_noStorage->hide();
    ui->slider_position->setValue(0);
    filePlayer->play(fileName, speed, startNs);
    selectListItem(fileName);
    updatePlayButtons();
}

void ServiceDVRForm::on_listWidget_records_itemClicked(QListWidgetItem *item)
{
    playRecord(item->data(Qt::UserRole).toString());
}

void ServiceDVRForm::playerFinished(bool ok)
{// файл кончился - сразу следующий файл той же камеры, записи идут подряд
    if (ok)
    {
        const QString next = neighbour(currentRecord, +1);
        if (!next.isEmpty())
        {
            playRecord(next);
            return;
        }
        _mainWindow->getView()->setText(ui->label_player, "Конец записей этой камеры");
    }
    else
        _mainWindow->getView()->setText(ui->label_player, "Ошибка воспроизведения");
    updatePlayButtons();
}

void ServiceDVRForm::updatePlayButtons()
{
    const bool running = filePlayer->isPlaying() && !filePlayer->isPaused();
    ui->pushButton_play->setText(running ? "ПАУЗА" : "ПУСК");
    setButtonStyle(ui->pushButton_play, "btn", true);
}

void ServiceDVRForm::on_pushButton_play_clicked()
{
    if (!filePlayer->isPlaying())
    {// после конца или ошибки - заново текущий файл, если его нет - самый новый
        if (!currentRecord.isEmpty())
            playRecord(currentRecord);
        else if (!records.isEmpty())
            playRecord(records.first());
    }
    else if (filePlayer->isPaused())
        filePlayer->resume();
    else
        filePlayer->pause();
    updatePlayButtons();
}

void ServiceDVRForm::on_pushButton_back10_clicked()
{
    const qint64 pos = filePlayer->position();
    if (pos < 0)
        return;
    if (pos < 10000000000LL)
    {// в начале файла - в конец предыдущего файла камеры
        const QString prev = neighbour(currentRecord, -1);
        if (!prev.isEmpty())
        {
            playRecord(prev, DVR_SEGMENT_NS - 10000000000LL + pos);
            return;
        }
    }
    filePlayer->seek(qMax<qint64>(pos - 10000000000LL, 0));
}

void ServiceDVRForm::on_pushButton_fwd10_clicked()
{
    const qint64 pos = filePlayer->position();
    const qint64 dur = filePlayer->duration();
    if (pos < 0)
        return;
    if (dur > 0 && pos + 10000000000LL >= dur)
    {// за концом файла - в начало следующего
        on_pushButton_next_clicked();
        return;
    }
    filePlayer->seek(pos + 10000000000LL);
}

void ServiceDVRForm::on_pushButton_prev_clicked()
{
    const QString prev = neighbour(currentRecord, -1);
    if (!prev.isEmpty())
        playRecord(prev);
}

void ServiceDVRForm::on_pushButton_next_clicked()
{
    const QString next = neighbour(currentRecord, +1);
    if (!next.isEmpty())
        playRecord(next);
}

void ServiceDVRForm::setSpeed(double rate)
{
    speed = rate;
    filePlayer->setRate(rate);
    setButtonStyle(ui->pushButton_speed025, "btn", rate == 0.25);
    setButtonStyle(ui->pushButton_speed050, "btn", rate == 0.5);
    setButtonStyle(ui->pushButton_speed100, "btn", rate == 1.0);
    setButtonStyle(ui->pushButton_speed200, "btn", rate == 2.0);
    setButtonStyle(ui->pushButton_speed400, "btn", rate == 4.0);
    // значок скорости на картинке - только если не x1
    const QString badge = rate == 0.25 ? "×¼" : rate == 0.5 ? "×½" : QString("×%1").arg(rate);
    _mainWindow->getView()->setText(ui->label_speedBadge, badge);
    ui->label_speedBadge->setVisible(rate != 1.0);
}

void ServiceDVRForm::on_pushButton_speed025_clicked() { setSpeed(0.25); }
void ServiceDVRForm::on_pushButton_speed050_clicked() { setSpeed(0.5); }
void ServiceDVRForm::on_pushButton_speed100_clicked() { setSpeed(1.0); }
void ServiceDVRForm::on_pushButton_speed200_clicked() { setSpeed(2.0); }
void ServiceDVRForm::on_pushButton_speed400_clicked() { setSpeed(4.0); }

void ServiceDVRForm::on_slider_position_sliderPressed()
{
    sliderDragging = true;// пока тянут ползунок, таймер его не двигает
}

void ServiceDVRForm::on_slider_position_sliderReleased()
{
    sliderDragging = false;
    qint64 dur = filePlayer->duration();
    if (dur <= 0)
        dur = DVR_SEGMENT_NS;
    filePlayer->seek(dur * ui->slider_position->value() / ui->slider_position->maximum());
}

void ServiceDVRForm::mainProgress()
{// ползунок и время: позиция в файле, время по часам записи, длина файла
    if (!isVisible() || !recordsTab || !filePlayer->isPlaying() || sliderDragging)
        return;
    const qint64 pos = filePlayer->position();
    qint64 dur = filePlayer->duration();
    if (pos < 0)
        return;
    if (dur <= 0)
        dur = DVR_SEGMENT_NS;
    ui->slider_position->setValue(int(qBound<qint64>(0, pos * ui->slider_position->maximum() / dur, ui->slider_position->maximum())));
    auto mmss = [](qint64 ns) {
        const qint64 s = ns / 1000000000LL;
        return QString("%1:%2").arg(s / 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
    };
    _mainWindow->getView()->setText(ui->label_posTime, mmss(pos));
    _mainWindow->getView()->setText(ui->label_duration, mmss(dur));
    // время по часам: начало файла из имени + позиция
    const QString base = QFileInfo(currentRecord).completeBaseName();
    const QDateTime start = QDateTime::fromString(base.section('_', 1, 2), "yyyyMMdd_HHmmss");
    if (start.isValid())
        _mainWindow->getView()->setText(ui->label_clock, start.addMSecs(pos / 1000000).toString("HH:mm:ss"));
}

// ============================================================ вкладка «Камеры»

void ServiceDVRForm::startLive(Tile &t)
{
    if (t.live || t.ip.isEmpty())
        return;
    t.view->show();
    t.state->hide();
    t.player->play(cameraUrl(t.ip));
    t.live = true;
}

void ServiceDVRForm::stopLive(Tile &t)
{
    if (!t.live)
        return;
    t.player->stop();
    t.view->hide();
    t.live = false;
}

void ServiceDVRForm::stopAllLive()
{
    for (int i = 0; i < tiles.size(); i++)
        stopLive(tiles[i]);
}

void ServiceDVRForm::updateTiles()
{// подписи, «нет связи», рамка выбранной; живой поток - у камер ролей на связи
    for (int i = 0; i < tiles.size(); i++)
    {
        Tile &t = tiles[i];
        const bool noRole = t.role.isEmpty();
        const bool visible = !noRole || !t.ip.isEmpty();
        t.place->setVisible(visible);
        t.label->setVisible(visible);
        if (!visible)
        {
            stopLive(t);
            t.state->hide();
            continue;
        }
        const QString border = (i == selectedTile) ? "border: 3px solid rgb(247,197,72);" : "border: none;";
        _mainWindow->getView()->setStyle(t.place, QString("QWidget#%1 { background: %2; %3 }").arg(t.place->objectName())
                                         .arg(t.online ? "rgb(0,0,0)" : "rgb(31,38,43)").arg(border));
        if (noRole)
        {
            _mainWindow->getView()->setText(t.label, t.ip + " · БЕЗ РОЛИ");
            _mainWindow->getView()->setStyle(t.label, "color: rgb(0,0,0); font: 600 13px \"Montserrat\"; background: rgb(240,128,52); border-radius: 4px; padding-left: 6px;");
        }
        else
            _mainWindow->getView()->setText(t.label, t.ip + " · " + cameraTitle(t.role));
        t.label->adjustSize();

        if (!recordsTab && t.online && !noRole)
            startLive(t);
        if (!t.online)
        {
            stopLive(t);
            _mainWindow->getView()->setText(t.state, "нет связи");
            t.state->show();
        }
    }
}

void ServiceDVRForm::selectTile(int index)
{
    if (index < 0 || index >= tiles.size() || tiles[index].ip.isEmpty())
        return;
    selectedTile = index;
    pendingConfirm.clear();
    confirmTimer.stop();
    const Tile &t = tiles[index];
    // роль по умолчанию: своя у камеры роли, первая свободная у камеры без роли
    targetRole = t.role;
    if (targetRole.isEmpty())
    {
        const char *order[3] = {"left", "right", "rear"};
        for (int r = 0; r < 3 && targetRole.isEmpty(); r++)
            if (!tiles[r].online)
                targetRole = order[r];
    }
    selectedInfo = QJsonObject();
    clearCameraInfo("Чтение параметров камеры…");
    updateTiles();
    runCamctl("info", QStringList() << "--ip" << t.ip);
}

bool ServiceDVRForm::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && !recordsTab)
    {
        for (int i = 0; i < tiles.size(); i++)
            if (obj == tiles[i].place || obj == tiles[i].label || obj == tiles[i].state)
            {
                selectTile(i);
                return true;
            }
    }
    return QWidget::eventFilter(obj, event);
}

void ServiceDVRForm::runCamctl(const QString &command, const QStringList &args)
{// одна команда за раз; поиск и чтение можно прервать, настройку и сброс - нет
    if (camctl->state() != QProcess::NotRunning)
    {
        if (camctlCommand == "scan" || camctlCommand == "info" || camctlCommand == "alias")
        {
            camctl->blockSignals(true);
            camctl->kill();
            camctl->waitForFinished(1000);
            camctl->blockSignals(false);
        }
        else
            return;
    }
    camctlCommand = command;
    const int ipArg = args.indexOf("--ip");
    camctlIp = ipArg >= 0 && ipArg + 1 < args.size() ? args[ipArg + 1] : QString();
    camctl->start("python3", QStringList() << DVR_CAMCTL << command << args);
    const bool longJob = command == "setup" || command == "reset" || command == "activate";
    ui->pushButton_exit->setEnabled(!longJob);// камеру нельзя бросать на середине настройки
    if (longJob)
        camJobUntilMs = QDateTime::currentMSecsSinceEpoch() + DVR_CAM_JOB_GRACE_MS;
    updateRoleButtons();
}

void ServiceDVRForm::camctlFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    const QString cmd = camctlCommand;
    camctlCommand.clear();
    ui->pushButton_exit->setEnabled(true);
    const QList<QByteArray> lines = camctl->readAllStandardOutput().trimmed().split('\n');
    QJsonObject res = QJsonDocument::fromJson(lines.isEmpty() ? QByteArray() : lines.last()).object();
    if (status != QProcess::NormalExit || res.isEmpty())
    {
        res["ok"] = false;
        res["error"] = "утилита camctl не ответила";
        qWarning() << "DVR camctl" << cmd << camctl->readAllStandardError().right(300);
    }
    handleCamctl(cmd, res);
    updateRoleButtons();
}

void ServiceDVRForm::handleCamctl(const QString &command, const QJsonObject &res)
{
    if (command == "scan")
    {// роли: на связи или нет; первая найденная камера не на IP роли - в 4-ю плитку
        if (res.value("ok").toBool())
            scanDone = true;
        const QJsonArray roles = res.value("roles").toArray();
        for (int i = 0; i < 3; i++)
            for (const QJsonValue &v : roles)
                if (v.toObject().value("role").toString() == tiles[i].role)
                    tiles[i].online = v.toObject().value("online").toBool();
        const QJsonArray other = res.value("other").toArray();
        const QString ip = other.isEmpty() ? QString() : other.first().toObject().value("ip").toString();
        Tile &t = tiles[3];
        if (ip != t.ip)
        {
            stopLive(t);
            t.ip = ip;
            t.online = !ip.isEmpty();
            if (selectedTile == 3)
                selectedTile = -1;
        }
        updateTiles();
        if (selectedTile < 0)
            selectTile(0);
        // камера в заводской подсети: нужен временный адрес пульта 192.168.1.2/24
        if (!ip.isEmpty() && !aliasOn && QHostAddress(ip).isInSubnet(QHostAddress("192.168.1.0"), 24))
            runCamctl("alias", QStringList() << "on");
        return;
    }
    if (command == "alias")
    {
        aliasOn = res.value("ok").toBool() && !aliasOn;
        return;
    }
    const bool forSelected = selectedTile >= 0 && tiles[selectedTile].ip == res.value("ip").toString();
    if (command == "info")
    {
        if (!forSelected)
            return;
        selectedInfo = res;
        showCameraInfo(res);
        Tile &t = tiles[selectedTile];
        if (t.role.isEmpty())
        {
            if (res.value("state").toString() == "factory")
            {// заводская камера не отдает видео, пока не активирована - активируем сами
                setCamMessage("Активация камеры (пароль из файла паролей камер)…");
                runCamctl("activate", QStringList() << "--ip" << t.ip);
            }
            else if (res.value("activated").toBool() && res.value("ok").toBool())
                startLive(t);
        }
        return;
    }
    if (command == "activate")
    {
        if (res.value("ok").toBool())
        {
            setCamMessage("Камера активирована");
            if (selectedTile >= 0)
                runCamctl("info", QStringList() << "--ip" << tiles[selectedTile].ip);
        }
        else
            setCamMessage("Не удалось активировать: " + res.value("error").toString(), true);
        return;
    }
    if (command == "setup")
    {
        if (res.value("ok").toBool())
        {// камера теперь на IP роли: 4-я плитка освобождается, выбираем плитку роли
            setCamMessage("Готово: камера настроена как " + roleAccusative(res.value("role").toString()) +
                          " (" + res.value("ip").toString() + ")");
            const QString role = res.value("role").toString();
            for (int i = 0; i < 3; i++)
                if (tiles[i].role == role)
                {
                    stopLive(tiles[i]);
                    tiles[i].online = true;
                    selectedTile = i;
                }
            stopLive(tiles[3]);
            tiles[3].ip.clear();
            updateTiles();
            runCamctl("info", QStringList() << "--ip" << tiles[selectedTile].ip);
        }
        else
        {
            QString msg = "Настройка не выполнена: " + res.value("message").toString(res.value("error").toString());
            const QJsonArray logTail = res.value("log").toArray();
            if (!logTail.isEmpty())
                msg += "\n" + logTail.last().toString().section(' ', 3);
            setCamMessage(msg, true);
            if (selectedTile >= 0)
                startLive(tiles[selectedTile]);
        }
        return;
    }
    if (command == "reset")
    {
        if (res.value("ok").toBool())
        {// после перезагрузки камера появится как «без роли» на 192.168.1.108
            setCamMessage(res.value("message").toString() + ". Камера появится слева как «БЕЗ РОЛИ».");
            if (selectedTile >= 0 && !tiles[selectedTile].role.isEmpty())
                tiles[selectedTile].online = false;
            updateTiles();
        }
        else
            setCamMessage("Сброс не выполнен: " + res.value("error").toString(), true);
        return;
    }
}

void ServiceDVRForm::clearCameraInfo(const QString &message)
{
    QLabel *k[9] = {ui->label_k1, ui->label_k2, ui->label_k3, ui->label_k4, ui->label_k5, ui->label_k6, ui->label_k7, ui->label_k8, ui->label_k9};
    QLabel *v[9] = {ui->label_v1, ui->label_v2, ui->label_v3, ui->label_v4, ui->label_v5, ui->label_v6, ui->label_v7, ui->label_v8, ui->label_v9};
    const char *names[9] = {"Состояние", "IP", "MAC", "Модель", "Прошивка", "Часы камеры", "Основной поток", "Доп. поток", "Изображение"};
    for (int i = 0; i < 9; i++)
    {
        _mainWindow->getView()->setText(k[i], names[i]);
        _mainWindow->getView()->setText(v[i], i == 1 && selectedTile >= 0 ? tiles[selectedTile].ip : QString("—"));
    }
    setCamMessage(message);
    updateRoleButtons();
}

void ServiceDVRForm::showCameraInfo(const QJsonObject &info)
{
    QLabel *v[9] = {ui->label_v1, ui->label_v2, ui->label_v3, ui->label_v4, ui->label_v5, ui->label_v6, ui->label_v7, ui->label_v8, ui->label_v9};
    const QString state = info.value("state").toString();
    // состояние: цвет и причина
    QString st, color = "#7bd88f";
    if (state == "role")
    {
        st = info.value("role_params_ok").toBool() ? "● в сети" : "● в сети · параметры отличаются от роли";
        if (!info.value("role_params_ok").toBool())
            color = "#f7c548";
    }
    else if (state == "factory")
    {
        st = "● без роли · заводские настройки";
        color = "#f7c548";
    }
    else if (state == "norole")
    {
        st = "● без роли · другой IP";
        color = "#f7c548";
    }
    else if (state == "badpass")
    {
        st = "● пароль не подходит — сбросьте камеру кнопкой на ней";
        color = "#ff9a9d";
    }
    else
    {
        st = "● нет связи";
        color = "#ff9a9d";
    }
    ui->label_v1->setTextFormat(Qt::RichText);
    _mainWindow->getView()->setText(v[0], QString("<span style='color:%1'>%2</span>").arg(color, st));
    _mainWindow->getView()->setText(v[1], info.value("ip").toString());
    _mainWindow->getView()->setText(v[2], info.value("mac").toString("—"));
    _mainWindow->getView()->setText(v[3], info.value("model").toString("—"));
    _mainWindow->getView()->setText(v[4], info.value("version").toString("—").section(',', 0, 0));
    // часы: камера без RTC после обесточивания уходит на 2000-01-01
    QString clock = "—";
    if (info.contains("time_delta"))
    {
        const int d = info.value("time_delta").toInt();
        if (info.value("cam_time").toString().startsWith("20") && info.value("cam_time").toString().left(4).toInt() < 2020)
            clock = "сбиты (" + info.value("cam_time").toString() + ")";
        else if (qAbs(d) <= 2)
            clock = "совпадают с пультом";
        else
            clock = QString("расходятся на %1 с").arg(d);
    }
    _mainWindow->getView()->setText(v[5], clock);
    const QJsonObject m = info.value("main").toObject();
    const QJsonObject e = info.value("extra").toObject();
    _mainWindow->getView()->setText(v[6], m.isEmpty() ? "—" : QString("%1 · %2×%3 · %4 к/с · %5 %6 кбит/с")
                                    .arg(m.value("codec").toString(), m.value("w").toString(), m.value("h").toString(),
                                         m.value("fps").toString(), m.value("brc").toString(), m.value("bitrate").toString()));
    _mainWindow->getView()->setText(v[7], e.isEmpty() ? "—" : QString("%1 · %2×%3 · %4 к/с")
                                    .arg(e.value("codec").toString(), e.value("w").toString(), e.value("h").toString(), e.value("fps").toString()));
    const QString mirror = info.value("mirror").toString();
    _mainWindow->getView()->setText(v[8], mirror.isEmpty() ? "—" : (mirror.toLower() == "true" ? "зеркало вкл" : "зеркало выкл"));
    if (!info.value("ok").toBool() && state != "badpass")
        setCamMessage(info.value("error").toString(), true);
    else if (state == "role" || state == "norole")
        setCamMessage("");
}

void ServiceDVRForm::updateRoleButtons()
{
    const bool busy = camctl->state() != QProcess::NotRunning &&
                      (camctlCommand == "setup" || camctlCommand == "reset" || camctlCommand == "activate");
    const bool haveCam = selectedTile >= 0;
    const QString state = selectedInfo.value("state").toString();
    const bool usable = haveCam && (state == "role" || state == "norole");   // с камерой можно работать нашим паролем
    QPushButton *btns[3] = {ui->pushButton_roleLeft, ui->pushButton_roleRight, ui->pushButton_roleRear};
    const char *roles[3] = {"left", "right", "rear"};
    const char *titles[3] = {"ЛЕВАЯ", "ПРАВАЯ", "ЗАДНЯЯ"};
    for (int r = 0; r < 3; r++)
    {
        // роль занята, если на ее IP на связи другая камера
        const bool occupied = tiles[r].online && r != selectedTile;
        const QString ipTail = "." + roleIp.value(roles[r]).section('.', 3, 3);
        btns[r]->setText(QString(titles[r]) + " " + ipTail + (occupied ? " · занято" : ""));
        if (occupied)
            setButtonStyle(btns[r], "dim", false);
        else
            setButtonStyle(btns[r], "btn", targetRole == roles[r]);
        btns[r]->setEnabled(!busy);
    }
    // «Настроить как …» / подтверждение
    const bool sameRole = haveCam && !tiles[selectedTile].role.isEmpty() && tiles[selectedTile].role == targetRole;
    if (pendingConfirm == "apply")
    {
        ui->pushButton_apply->setText("ПОДТВЕРДИТЬ");
        setButtonStyle(ui->pushButton_apply, "confirm", true);
    }
    else
    {
        ui->pushButton_apply->setText(targetRole.isEmpty() ? "ВЫБЕРИТЕ РОЛЬ"
                                      : sameRole ? "ПРИМЕНИТЬ ПАРАМЕТРЫ РОЛИ"
                                                 : "НАСТРОИТЬ КАК " + roleAccusative(targetRole).toUpper());
        const bool canApply = usable && !busy && !targetRole.isEmpty();
        setButtonStyle(ui->pushButton_apply, canApply ? "btn" : "dim", canApply);
    }
    ui->pushButton_apply->setEnabled(usable && !busy && !targetRole.isEmpty());
    if (pendingConfirm == "reset")
    {
        ui->pushButton_reset->setText("ПОДТВЕРДИТЬ СБРОС");
        setButtonStyle(ui->pushButton_reset, "confirm", true);
    }
    else
    {
        ui->pushButton_reset->setText("СБРОС К ЗАВОДСКИМ");
        setButtonStyle(ui->pushButton_reset, usable && !busy ? "red" : "dim", false);
    }
    ui->pushButton_reset->setEnabled(usable && !busy);
}

void ServiceDVRForm::setCamMessage(const QString &text, bool error)
{
    _mainWindow->getView()->setStyle(ui->label_camMessage, QString("color: %1; font: 13px \"Montserrat\"; background: transparent;")
                                     .arg(error ? "rgb(255,154,157)" : "rgb(150,156,162)"));
    _mainWindow->getView()->setText(ui->label_camMessage, text);
}

void ServiceDVRForm::on_pushButton_roleLeft_clicked()
{
    if (tiles[0].online && selectedTile != 0)
        return;// занято другой камерой
    targetRole = "left";
    pendingConfirm.clear();
    updateRoleButtons();
}

void ServiceDVRForm::on_pushButton_roleRight_clicked()
{
    if (tiles[1].online && selectedTile != 1)
        return;
    targetRole = "right";
    pendingConfirm.clear();
    updateRoleButtons();
}

void ServiceDVRForm::on_pushButton_roleRear_clicked()
{
    if (tiles[2].online && selectedTile != 2)
        return;
    targetRole = "rear";
    pendingConfirm.clear();
    updateRoleButtons();
}

void ServiceDVRForm::on_pushButton_apply_clicked()
{// опасное действие - повторным нажатием в течение 3 с
    if (selectedTile < 0 || targetRole.isEmpty())
        return;
    if (pendingConfirm != "apply")
    {
        pendingConfirm = "apply";
        confirmTimer.start(3000);
        updateRoleButtons();
        return;
    }
    pendingConfirm.clear();
    confirmTimer.stop();
    Tile &t = tiles[selectedTile];
    stopLive(t);// камера перенастраивается и может сменить IP
    setCamMessage("Настройка камеры " + t.ip + " как " + roleAccusative(targetRole) + "… до 2 минут, экран не закрывайте");
    runCamctl("setup", QStringList() << "--ip" << t.ip << "--role" << targetRole);
}

void ServiceDVRForm::on_pushButton_reset_clicked()
{
    if (selectedTile < 0)
        return;
    if (pendingConfirm != "reset")
    {
        pendingConfirm = "reset";
        confirmTimer.start(3000);
        updateRoleButtons();
        return;
    }
    pendingConfirm.clear();
    confirmTimer.stop();
    Tile &t = tiles[selectedTile];
    stopLive(t);
    setCamMessage("Сброс камеры " + t.ip + " к заводским настройкам…");
    runCamctl("reset", QStringList() << "--ip" << t.ip);
}

void ServiceDVRForm::confirmTimeout()
{
    pendingConfirm.clear();
    updateRoleButtons();
}
