#include "logger.h"
#include "mainwindow.h"
#include <QDebug>
#include <QStorageInfo>

QString logPath = "/home/knight/";

Logger::Logger(QObject *parent_) : QObject(NULL)
{
    writePeriod = WRITE_PERIOD;// 10 раз в секунду

    // в каком порядке добавлены поля, в таком и будут лежать в логе
    userFieldsMap.insert(UF_WORK_MODE, new UserField("Режим работы", false, 1));
    userFieldsMap.insert(UF_SERVICE_PRESSED, new UserField("Кнопка входа в сервис", true, 1));

    quint8 bitCounter = 0;
    quint8 byteCounter = 0;
    foreach (userFields key, userFieldsMap.keys())
    {
        if (bitCounter + userFieldsMap[key]->bitSize >= 8)
        {
            bitCounter = 0;
            byteCounter ++;
        }
        bitCounter += userFieldsMap[key]->bitSize;
    }
    userFieldsData.resize(byteCounter + 1);

    stop = false;
    haveSystemConfig = false;
    noFreeSpaceAlert = false;
    parent = parent_;
    mThread = new QThread();
    moveToThread(mThread);
    connect(mThread, SIGNAL(started()), this, SLOT(loggerInit()));//, Qt::DirectConnection
    connect(this, SIGNAL(destroyed(QObject*)), mThread, SLOT(quit()));
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    mThread->start();
}

Logger::~Logger()
{

}

void Logger::fillSystemConfigure(SystemConfigure* srcC, QMap<int, SystemElement*>* srcE)
{
    QMutexLocker m(&logInfoMutex);

    systemConfigure.clear();

    srcC->copy(&systemConfigure);
    foreach (int key, srcE->keys())
    {
        systemElements.insert(key, new SystemElement(srcE->value(key)));
    }

    haveSystemConfig = true;
    createBlackBox();
}

void Logger::loggerInit()
{
    mThread->setPriority(QThread::LowPriority);

    //создаем файлы test
    //createBlackBox();

    // почистим старые логи (все что старше 1 месяца надо удалить)
    QDir d(logPath + "/logs/");
    QStringList filters;
    filters << "??_??_????";
    QStringList dirs = d.entryList(filters);
    qDebug() << dirs;
    foreach (QString entry, dirs)
    {
        qDebug() << entry;
        if (QDate::fromString(entry, "dd_MM_yyyy").daysTo(QDate::currentDate()) > 30)
        {
            QDir dir_to_del(logPath + "/logs/" + entry);
            dir_to_del.removeRecursively();
        }
    }



    loggerTimer = new QTimer();
    connect(loggerTimer,SIGNAL(timeout()),this,SLOT(timeToWrite()));
    loggerTimer->setInterval(writePeriod);
    // test
    loggerTimer->start();

    flushTimer = new QTimer();
    connect(flushTimer,SIGNAL(timeout()),this,SLOT(timeToFlush()));
    flushTimer->setInterval(60000 * 5);// 5 минут запись логов
    // test
    flushTimer->start();
}

// создаем болванку для записи
void Logger::createBlackBox()
{
    BBdir = logPath + "/logs/"+QDateTime::currentDateTime().toString("dd_MM_yyyy")+"/";
    QDir().mkdir(BBdir);
    BBdir = BBdir+QDateTime::currentDateTime().toString("hh_mm_ss_zzz")+"/";
    QDir().mkdir(BBdir);

    QString BBfile = BBdir+"/AutoCleaner318.dat";

    QFile bFile(BBfile);
    bFile.open(QIODevice::WriteOnly);
    QDataStream Inbox(&bFile);
    Inbox.setVersion(QDataStream::Qt_4_8);
    Inbox << (quint16)LOG_VERSION;

    // предварительно надо пробежаться и посчитать сколько у меня элементов двойных
    quint16 double_element = 0;
    for (int i = 1; i < 9; i++)
        for (int k = 0; k < 12; k++)
            if (systemConfigure.id[i][k] > 0)
                if (systemConfigure.channelsOutFrameId[i][k] > 0)
                    double_element++;

    Inbox << (quint16)(systemElements.count() + double_element + userFieldsMap.size()); // количество параметров
    Inbox << (quint8)9; // количество блоков информации
    Inbox << (quint8)userFieldsData.size(); // длинна 1 блока A0
    Inbox << (quint8)8; // длинна 2 блока B1
    Inbox << (quint8)8; // длинна 3 блока B2
    Inbox << (quint8)8; // длинна 4 блока B3
    Inbox << (quint8)8; // длинна 5 блока B4
    Inbox << (quint8)8; // длинна 6 блока B5
    Inbox << (quint8)8; // длинна 7 блока B6
    Inbox << (quint8)8; // длинна 8 блока B7
    Inbox << (quint8)8; // длинна 9 блока B8

    // юзерские
    quint8 bitCounter = 0;
    quint8 byteCounter = 0;
    foreach (userFields key, userFieldsMap.keys())
    {
        Inbox << userFieldsMap[key]->name;
        Inbox << (quint8)0;
        Inbox << byteCounter;
        Inbox << bitCounter;
        Inbox << (float)1;
        Inbox << (qint16)0;
        Inbox << (QString)"нечто";
        if (bitCounter + userFieldsMap[key]->bitSize >= 8)
        {
            bitCounter = 0;
            byteCounter ++;
        }
        bitCounter += userFieldsMap[key]->bitSize;
    }

    // в 318 машине применил новый метод записи лога который сам себя разбирает на парсинге
    for (int i = 1; i < 9; i++)
    {
        for (int k = 0; k < 12; k++)
        {
            if (systemConfigure.id[i][k] > 0)
            {
                if (systemConfigure.channelsOutFrameId[i][k] > 0)
                {
                    Inbox << (QString)(systemElements[systemConfigure.id[i][k]]->name + "(вых)");
                    Inbox << (quint8)(systemConfigure.channelsOutFrameId[i][k] + 5); // 3 выходные команды будут идти после 5 входных
                    Inbox << (quint8)(systemConfigure.channelsOutByteId[i][k] + userFieldsData.size() + (systemConfigure.channelsOutFrameId[i][k] - 1 + 5) * 8);
                    if (systemConfigure.channelsType[i][k] == OUT_MODE_NORMAL || systemConfigure.channelsType[i][k] == IN_MODE_NORMAL)
                        Inbox << systemConfigure.channelsOutBitId[i][k];
                    else if (systemConfigure.channelsType[i][k] == IN_MODE_ANALOG_16 || systemConfigure.channelsType[i][k] == IN_MODE_EXTI_16)
                        Inbox << (quint8)9;
                    else
                        Inbox << (quint8)8;
                    Inbox << (float)1;
                    Inbox << (qint16)0;
                    Inbox << (QString)"нечто";
                }
                Inbox << systemElements[systemConfigure.id[i][k]]->name;
                Inbox << systemConfigure.channelsFrameId[i][k];
                Inbox << (quint8)(systemConfigure.channelsByteId[i][k] + userFieldsData.size() + (systemConfigure.channelsFrameId[i][k] - 1) * 8);
                if (systemConfigure.channelsType[i][k] == OUT_MODE_NORMAL || systemConfigure.channelsType[i][k] == IN_MODE_NORMAL)
                    Inbox << systemConfigure.channelsBitId[i][k];
                else if (systemConfigure.channelsType[i][k] == IN_MODE_ANALOG_16 || systemConfigure.channelsType[i][k] == IN_MODE_EXTI_16)
                    Inbox << (quint8)9;
                else
                    Inbox << (quint8)8;
                Inbox << (float)1;
                Inbox << (qint16)0;
                Inbox << (QString)"нечто";
            }
        }
    }

//    Inbox << (QString)"название элемента в паке 2";
//    Inbox << (quint8)framenumber их 3, если 0 то значит это байт юзерский;
//    Inbox << (quint8)byte;
//    Inbox << (quint8)bit если 8 то значит весь байт если 9 то 2 байта;
//    Inbox << (float)множитель;
//    Inbox << (qint16)сдвиг;
//    Inbox << (QString)"единица измерения";

    // структура самих данных
    // Inbox << признак существования данных 1 байт (у юзерского байта всегда есть данные)
    // Inbox << данные 9 * 8 байт (1 юзерский и 5 входных и 3 выходных рабочих)

    // вычитываем фиксированно на одну итерацию 9 байт на юзерскую 9 по 5 прием 9 по 3 отправка

    // окончание списка полей
    Inbox << (quint16)0;
    Inbox << (quint16)0;
    Inbox << (quint16)0;
    // время начала записи (всегда используется сдвиг....хз почему)
    Inbox << QDateTime::currentDateTime().addSecs(18000);
    // период записи
    Inbox << (quint16)writePeriod;

    bFile.close();
}

void Logger::setStop(bool val)
{
    QMutexLocker m(&logInfoMutex);
    stop = val;
}

// записываем параметры в буфер для последующего слива на карточку
void Logger::timeToWrite()
{
    foreach ( const auto& key, logDataTimeout.keys() )
    {// удаляем пакеты которые долго не обновлялись (5 сек)
        logDataTimeout[key]--;
        if (logDataTimeout[key] == 0)
        {
            qDebug() << "remove" << QString::number(key, 16);
            logData.remove(key);
            logDataTimeout.remove(key);
        }
    }
    logInfoMutex.lock();
    bool local_stop = stop;
    stop = false;
    QMap<unsigned int, QByteArray> tmp_data = logData;
//    logData.clear();
    logInfoMutex.unlock();
    // проверяем нужно ли экстренно слить все
    if (local_stop)
    {
        timeToFlush();
    }
    // выдуманный пакет A0 там лежат взаимодействия юзера с интерфейсом и панелью ПВИ
    // тут опишем его протокол
    // 0 байт
    // 0 - бит. нажатие ПЛЭЙ
    // 1 - бит. нажатие солнышка
    // 2 - бит. нажатие снежинки
    // 3 - бит. нажатие диагностики
    // 4 - бит. нажатие настроек
    // 5 - бит. нажатие левой рейки
    // 6 - бит. нажатие левой щетки
    // 7 - бит. нажатие задней рейки
    // 1 байт
    // 0 - бит. нажатие правой щетки
    // 1 - бит. нажатие правой рейки
    // 2 - бит. нажатие подменю воды
    // 3 - бит. нажатие подменю лейки
    // 4 - бит. нажатие подменю охлаждения
    // 5 - бит. нажатие подменю обычной рейки
    // 6 - бит. нажатие супердиагностики
    // 7 - бит.
    // 2 байт
    // 0 - бит. нажатие кнопки питания ПВИ
    // 1 - бит. нажатие кнопки плэй на ПВИ
    // 2 - бит. нажатие кнопки смены сторон на ПВИ
    // 3 - бит. нажатие левой качели вверх
    // 4 - бит. нажатие левой качели вниз
    // 5 - бит. нажатие правой качели вверх
    // 6 - бит. нажатие правой качели вниз
    // 7 - бит. нажатие грибка
    // 3 байт
    // 0 - бит. нажатие нажатие аварийной кнопки
    // 1 - бит. состояние уборки (переменная startstop)
    if (tmp_data.contains(0x0000A000))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000A000));
        // фишка в том чтобы занулять каждый раз взаимодействие с интерфейсом (чтобы была имитация нажатия и отжатия)
        logInfoMutex.lock();
        logData.remove(0x0000A000);
        //logData[0x0000A000].fill(0);
        logInfoMutex.unlock();
    }
    else
        dataForFlush.append(QByteArray(userFieldsData.size() + 1, 0));
    if (tmp_data.contains(0x0000B100))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000B100));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000B200))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000B200));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000B300))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000B300));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000B400))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000B400));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000B500))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000B500));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000A100|CAN_EFF_FLAG))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000A100|CAN_EFF_FLAG));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000A200|CAN_EFF_FLAG))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000A200|CAN_EFF_FLAG));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
    if (tmp_data.contains(0x0000A300|CAN_EFF_FLAG))
    {
        dataForFlush.append(QByteArray(1, 0xFF));
        dataForFlush.append(tmp_data.value(0x0000A300|CAN_EFF_FLAG));
    }
    else
        dataForFlush.append(QByteArray(9, 0));
}

// сливаем накопившийся буфер на карточку
void Logger::timeToFlush()
{
    qDebug() << "flush";
    // проверка свободного места и запаса
    QStorageInfo info(BBdir);
    if (info.bytesAvailable() > 10000000)
    {// больше 10 MB
        if (noFreeSpaceAlert)
        {
            createBlackBox();
            QMetaObject::invokeMethod( parent, "addLog", Qt::QueuedConnection, Q_ARG( QString, "Сводобное место на карточке восстановилось" ), Q_ARG(int, MainWindow::WarningStatus) );
            noFreeSpaceAlert = false;
            qDebug() << "free space restored";
        }

        QDateTime tmp_time = QDateTime::currentDateTime();
        QFile bFile(BBdir + "/AutoCleaner318.dat");
        bFile.open(QIODevice::Append);
        QDataStream Inbox(&bFile);
        Inbox.setVersion(QDataStream::Qt_4_8);
        Inbox.writeRawData(dataForFlush.data(), dataForFlush.size());
        bFile.close();
        //system("sync"); // хз надо ли
        qDebug() << "flush time" << tmp_time.msecsTo(QDateTime::currentDateTime());
    }
    else
    {
        if (!noFreeSpaceAlert)
        {
            QMetaObject::invokeMethod( parent, "addLog", Qt::QueuedConnection, Q_ARG( QString, "Сводобное место на карточке кончилось" ), Q_ARG(int, MainWindow::FatalStatus) );
            noFreeSpaceAlert = true;
            qDebug() << "no free space to write blackbox";
        }
    }
    dataForFlush.clear();
}

// функция для обновления параметров которые будут записываться
void Logger::addLogInfo(LogType logType, unsigned int header, QByteArray data)
{
    QMutexLocker m(&logInfoMutex);
    logData.insert(header, data);
    logDataTimeout.insert(header, (1000 / WRITE_PERIOD) * 5);// (1000 / 100) * 5 = 50 раз вызовется timeToWrite с периодом 100 ( итого время жизни пакета 5 сек )
}

void Logger::addUserLogInfo(userFields field, int value)
{
    QMutexLocker m(&logInfoMutex);
    // работаем с пакетом 0x0000A000
    quint8 bitCounter = 0;
    quint8 byteCounter = 0;
    foreach (userFields key, userFieldsMap.keys())
    {
        if (key == field)
        {// нашли, записываем
            // пока будем использовать отлько 1 бит и 8 бит значния, а то шляпа полная
            if (userFieldsMap[key]->bitSize == 1)
            {
                if ((bool)value)
                {
                    userFieldsData[byteCounter] = userFieldsData[byteCounter] | (1 << bitCounter);
                }
                else
                {
                    if (!userFieldsMap[key]->isImpulse)
                        userFieldsData[byteCounter] = userFieldsData[byteCounter] & (~(1 << bitCounter));
                }

            }
            else
            {
                if (userFieldsMap[key]->isImpulse)
                    userFieldsData[byteCounter] = userFieldsData[byteCounter] | (quint8)value;
                else
                    userFieldsData[byteCounter] = (quint8)value;
            }
            break;
        }
        if (bitCounter + userFieldsMap[key]->bitSize >= 8)
        {
            bitCounter = 0;
            byteCounter ++;
        }
        bitCounter += userFieldsMap[key]->bitSize;
    }
    logData.insert(0x0000A000, userFieldsData);
    logDataTimeout.insert(0x0000A000, (1000 / WRITE_PERIOD) * 5);// (1000 / 100) * 5 = 50 раз вызовется timeToWrite с периодом 100 ( итого время жизни пакета 5 сек )
}

void Logger::addLogText(QString text)
{
    QMutexLocker m(&logInfoMutex);
    QFile bFile(BBdir + "/AutoCleaner.txt");
    bFile.open(QIODevice::Append | QIODevice::Text);
    QTextStream Inbox(&bFile);
    Inbox << text;
    bFile.close();
    //system("sync"); // хз надо ли
}
