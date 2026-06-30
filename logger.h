#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QMap>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDataStream>

#include "configure.h"

// вермия лога
// 1 - обычная (список полей и все)
// 2 - список полей с описанием (название, единица измерения, смещение, длинна, множитель, сдвиг)
#define LOG_VERSION 2
#define WRITE_PERIOD 100

// модуль для складирования пакетов от ПО ГО, а так же команд им
class Logger : public QObject
{
    Q_OBJECT
public:
    enum LogType
    {
        canData = 1,
        canJ1939Data = 2
    };
    enum logFields
    {
        ID_BU_T_1 = 1,
        ID_BU_R_1 = 2,
        ID_BU_R_2 = 3,
        ID_BU_R_3 = 4,
        ID_BU_R_4 = 5,
        ID_BU_R_5 = 6,
        ID_BU_U_1 = 7,
        ID_BU_T_2 = 8,
        ID_BU_T_3 = 9,
        ID_J1939_DM01 = 10
    };

    enum userFields
    {
        UF_WORK_MODE,
        UF_SERVICE_PRESSED
    };

    struct UserField
    {
    public:
        UserField(QString name_, bool isImpulse_, int bitSize_)
        {
            name = name_;
            isImpulse = isImpulse_;
            bitSize = bitSize_;
        }
        QString name; // имя поля
        bool isImpulse; // импульсный ли он (например нажатие кнопки - импульсное)
        int value; // само значение поля
        int bitSize; // сколько бит займет значение в логе
    };


    explicit Logger(QObject *parent_ = nullptr);
    ~Logger();

    void createBlackBox();

    QObject * parent;
    bool noFreeSpaceAlert;

    QByteArray userFieldsData;
    QMap<userFields, UserField*> userFieldsMap;
    bool haveSystemConfig;
    SystemConfigure systemConfigure;
    QMap<int, SystemElement*> systemElements;
    void fillSystemConfigure(SystemConfigure* srcC, QMap<int, SystemElement*>* srcE);

    // сеттер для добавления информации в буфер записи
    // модули в удобное для них время складируют информацию в лог, а логгер уже в нужные моменты записи перетаскивает это в свои буферы
    void addLogInfo(LogType logType, unsigned int header, QByteArray data);
    void addUserLogInfo(userFields field, int value);
    void addLogText(QString text);
    void setStop(bool val);
    bool stop;
    QMap<unsigned int, QByteArray> logData;
    QMap<unsigned int, unsigned int> logDataTimeout;
    // готовый набор тела файла, который нужно будет слить на карту
    QByteArray dataForFlush;
    QString BBdir;

    QThread *mThread;
    QTimer *loggerTimer;// таймер для периодической записи в буферы
    QTimer *flushTimer; // таймер для слива информации на sd карту
    quint16 writePeriod;// период записи в лог

    QMutex logInfoMutex;

public slots:
    void loggerInit();
    // слот для записи накопившегося
    void timeToWrite();
    void timeToFlush();
};

#endif // LOGGER_H
