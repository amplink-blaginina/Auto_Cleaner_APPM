#ifndef MYCAN_H
#define MYCAN_H

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <linux/can.h>
#include <sys/ioctl.h>

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>

#include "logger.h"
#include "../configure.h"

#define MEDIAN_FILTER_SIZE 5

#define BOARD_CP	 	0
#define BOARD_OUT 		1
#define BOARD_IN_AN 	2
#define BOARD_IN		3
#define BOARD_OUT_AN 	4
#define BOARD_IN_KM 	5
#define BOARD_UNKNOWN  15

#define IN_MODE_NORMAL 			0
#define IN_MODE_ANALOG_8 		1
#define IN_MODE_ANALOG_16		2
#define IN_MODE_EXTI_8 			3
#define IN_MODE_EXTI_16			4
#define IN_MODE_PWM				5

// на платах они нумеруются отдельно с нуля 0 1 2 3
#define OUT_MODE_NORMAL     6
#define OUT_MODE_PWM		7
#define OUT_MODE_FC			8
#define OUT_MODE_PFM        9

//// замок зажигания
//#define IN1_VALUE			gpio_input_bit_get(GPIOA, GPIO_PIN_12)
//// аварийная кнопка
//#define IN2_VALUE			gpio_input_bit_get(GPIOA, GPIO_PIN_11)
//// вкл ПВИ
//#define IN3_VALUE			gpio_input_bit_get(GPIOA, GPIO_PIN_10)
//// стартер (тут вроде как резерв)
//#define IN4_VALUE			gpio_input_bit_get(GPIOA, GPIO_PIN_9)
//// подхват питания
//#define OUT1 					gpio_input_bit_get(GPIOA, GPIO_PIN_8) == SET
//// глушение двигателя (тут разрешение на работу движка или ЗАЖИГАНИЕ)
//#define OUT2 					gpio_input_bit_get(GPIOC, GPIO_PIN_9) == SET
//// внешнее питание плат
//#define OUT3 					gpio_input_bit_get(GPIOC, GPIO_PIN_8) == SET
//// питание ПВИ
//#define OUT4 					gpio_input_bit_get(GPIOC, GPIO_PIN_7) == SET

enum DeviceStates
{
    //StateCoolHydroVentK3                    = 0,
    StateValveE1                            = 1,
    StateValveE2                            = 2,
    StateValveE3                            = 3,
    //StateValveE4                            = 4,
    StateValveE5                            = 5,
    StateValveE6                            = 6,
    StateValveE7                            = 7,
    //StateValveE8                            = 8,
    //StatePowerValveB1                       = 9,
    StateValveC1                            = 10,
    StateValveC2                            = 11,
    StateValveC3                            = 12,
    StateValveC4                            = 13,
    //StateValveC5                            = 14,
    //StateValveC6                            = 15,
    //StateValveC7                            = 16,
    //StateValveC8                            = 17,
    //StateValveC9                            = 18,
    //StateValveC10                           = 19,
    //StateValveC11                           = 20,
    //StateValveC12                           = 21,
    //StateValveC13                           = 22,
    //StateValveC14                           = 23,
    StateValveF1                            = 24,
    StateValveF2                            = 25,
    StateValveF3                            = 26,
    StateValveF4                            = 27,
    //StateValveF5                            = 28,
    StateValveF6                            = 29,
    StateValveF7                            = 30,
    StateValveF8                            = 31,
    StateValveF9                            = 32,
    StateValveF10                           = 33,
    //StateValveF11                           = 34,
    StateValveF12                           = 35,
    //StateHydroDriveValveA4                  = 36,
    //StateBroomPressValveA5                  = 37,
    //StatePowerValveA1                       = 38,
    //StateWaterPumpValveA6                   = 39,
    //StateNozzlesFrontRailValveW7            = 40,
    //StateSprayersFrontRailValveW8           = 41,
    //StateHPPistolWaterValveW9               = 42,
    //StateNozzlesBackRailFirstLineValveW1    = 43,
    //StateNozzlesLeftRailFirstLineValveW2    = 44,
    //StateNozzlesRightRailFirstLineValveW3   = 45,
    //StateNozzlesBackRailSecondLineValveW4   = 46,
    //StateNozzlesLeftRailSecondLineValveW5   = 47,
    //StateNozzlesRightRailSecondLineValveW6  = 48,
    //StateBeaconsL1                          = 49,
    //StateSideLightsL2                       = 50,
    //StateBroomLightL3                       = 51,
    //StateBackRailLightL4                    = 52,
    //StateHoodLightL5                        = 53,
    //StateKOMOnK1                            = 54,
    //StateKOMOffK2                           = 55,
    //StateSoundZ1                            = 56,
    //StateTankMinLevelD20                    = 57,
    //StateTank1LevelD20                      = 58,
    //StateTank2LevelD20                      = 59,
    //StateTankMaxLevelD20                    = 60,
    StateDrainFilterD28                     = 61,
    //StateSuckFilterD29                      = 62,
    //StateFrontRailLevelD16                  = 63,
    //StateDKPFrontLeftRailD17                = 64,
    //StateDKPFrontRightRailD18               = 65,
    //StateDKPBackLeftRailD10                 = 66,
    //StateDKPBackRightRailD11                = 67,
    StateHydroTankLevelD27                  = 68,
    //StateKOMD3                              = 69,
    //StateBackRailLevelD9                    = 70,
    //StateSideLeftRailLevelD12               = 71,
    //StateSideRightRailLevelD13              = 72,
    //StateSideLeftRailExtractD14             = 73,
    //StateSideRightRailExtractD15            = 74,
    //StateFrontRailRotateD19                 = 75,
    //StateHydroSpeedD8                       = 76,
    //StateBroomLevelD6                       = 77,
    //StateBroomPressLevelD7                  = 78,
    //StateBroomRotateD5                      = 79,
    //StateWaterPressureD21                   = 80,
    //StateHydroDistributor1_3PressureD22     = 81,
    //StateHydroDistributor2PressureD23       = 82,
    //StateWaterPumpHydroPressureD24          = 83,
    //StateHydroBroomPressureD25              = 84,
    //StateHydroPreloadPressureD26            = 85,
    //StateHydroTempD30                       = 86,
    //StateEnvTempD31                         = 87,
    //StateKOMTempD4                          = 88,
    //StateStarter                            = 89,
    //StateTemperatureReleEngine              = 90,
    //StateTemperatureEngine                  = 91,
    //StateOilPressure                        = 92,
    //StateTemperatureEngineAN                = 93,
    StateValveA1                            = 94,
    //StateValveA2                            = 95,
    //StateValveB1                            = 96,
    //StateValveB2                            = 97,
    //StateValveB3                            = 98,
    //StateValveB4                            = 99,
    //StateValveB5                            = 100,
    //StateValveB6                            = 101,
    //StateValveB7                            = 102,
    //StateValveB8                            = 103,
    //StateValveB9                            = 104,
    //StateValveB10                           = 105,
    //StateThrotleAllow                       = 106,
    //StateThrotleMode                        = 107,
    //StateEngineStopValve                    = 109,
    //StateValveK1                            = 110,
    StateValveD1                            = 111,
    //StateFuelAN                             = 112,
    StateHydraulicOilTemperature            = 113,
    //StateTahometerEngineIMP                 = 114,
    //StateITPower                            = 115,
    //StateBoardPower                         = 116,
    //StatePVIPower                           = 117,
    StateFRMBroomL1                         = 118,
    StateFRMBackL2                          = 119,
    //StateBeaconsL3                          = 120,
    //StateBackConturL4                       = 121,
    StateKungL5                             = 122,
    Board0IN1                               = 123,
    Board0IN2                               = 124,
    Board0IN3                               = 125,
    Board0IN4                               = 126,
    //StateTemperatureElectricEngineAN        = 127,
    //StateTemperatureGeneratorAN             = 128,
    StateDKPBackMagnetUp                    = 129,
    //StateDKPBlowerUp                        = 130,
    StateDKPBroomUp                         = 131,
    StateDKPBroomLeft                       = 132,
    StateDKPBroomRight                      = 133,
    StateDKPDumpUp                          = 134,
    StateDKPDumpLeft                        = 135,
    StateDKPDumpRight                       = 136,
    //StateEngineVentilation                  = 137,
    Board1Configured                        = 138,
    Board2Configured                        = 139,
    Board3Configured                        = 140,
    Board4Configured                        = 141,
    Board5Configured                        = 142,
    Board6Configured                        = 143,
    Board7Configured                        = 144,
    Board8Configured                        = 145,
    Board1Type                              = 146,
    Board2Type                              = 147,
    Board3Type                              = 148,
    Board4Type                              = 149,
    Board5Type                              = 150,
    Board6Type                              = 151,
    Board7Type                              = 152,
    Board8Type                              = 153,
    Board0OUT1                              = 154,
    Board0OUT2                              = 155,
    Board0OUT3                              = 156,
    Board0OUT4                              = 157,
    //StateDKPKungUp                          = 158,
    StateValveD2                            = 159,
    StateValveD3                            = 160,
    StatePressureFilter1                    = 161,
    StatePressureFilter2                    = 162,
    StatePressureFilter3                    = 163,
    StateHydraulicDistibutorPressure        = 164,
    StateHydraulicBroomPressure             = 165,
    StateHydraulicFanPressure               = 166,
    StateHydraulicBroomPressPressure        = 167,
    StateEngineReady                        = 168,
    StateSensorsPower                       = 169,
    StateHydraulicFan                       = 170,
    StateDKPBlowerUp1                       = 171,
    StateDKPBlowerUp2                       = 172,
    StateIgnitionOut                        = 173,
    StatePVIPowerIn                         = 174,
    StateAlarmIn                            = 175,
    StateStarterAllow                       = 176,
    StateStarterRoll                        = 177,
    State24Volt                             = 178,
    StateWaterSensor                        = 179,
    StateRollIn                             = 180,
    StateAirFilterBad                       = 181,
    StateOilFilterBad                       = 182,
    StateOilRele                            = 183,
    StateHeatRele                           = 184,
    StateUnknown                            = 255
};

class MyCan : public QObject
{
    Q_OBJECT
public:
    explicit MyCan(QString camName_, Logger * logger_, bool ableToRestart_, QObject *parent = nullptr);

    void CAN_reset();
    void CAN_clear_frames();
    void canSend(struct can_frame frame_);

    QVariant getDataFromFrame(struct can_frame* frame, quint8 board_, quint8 channel_);
    QVariant getOriginalState(quint8 board_, quint8 channel_);
    QVariant getState(DeviceStates dev, bool is_raw = false);
    void toggleState(DeviceStates dev);

    bool isBitIn(quint8 i, quint8 k);
    bool isByteIn(quint8 i, quint8 k);
    bool is2ByteIn(quint8 i, quint8 k);
    bool isBitOut(quint8 i, quint8 k);
    bool isByteOut(quint8 i, quint8 k);

    void setOriginalState(quint8 board_, quint8 channel_, QVariant state, bool ignore_change_speed = false);
    void setState(DeviceStates dev, QVariant state, bool ignore_change_speed = false);

    void makeMedian(quint8* val, QList<QList<quint8>> &bRaw, QList<QList<quint8>> &bSmooth);
    bool isActive();

    void fillSystemConfigure(SystemConfigure* srcC, QMap<int, SystemElement*>* srcE);
    bool isConfigured();
    bool isHaveSystemConfig();

    QMutex stateMutex;

    bool waterPumpInversion;
    void setWaterPumpInversion(bool inversion);

    bool haveSystemConfig;
    QMutex configureMutex;
    struct can_frame frameConfig;
    SystemConfigure systemConfigure;
    QMap<int, SystemElement*> systemElements;
    bool configureStarted;
    bool BUCPConfigured;
    quint8 BUCPConfigureStep;

    Logger * logger;

    bool ableToRestart;
    QString canName;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    struct timeval tv;
    int sock;
    int nbytes;

    // тред для отделения в отдельный поток
    QThread *mThread;
    // таймер для приема и отправки
    QTimer *mTimerRecv;
    QTimer *mTimerSend;

    QList<QList<quint8>> b1Smooth; // фильтрованные значения от CAN (медианный фильтр)
    QList<QList<quint8>> b1Raw; // буфер сырых значений от CAN для проведения сортировок и фильтраций
    QList<QList<quint8>> b2Smooth;
    QList<QList<quint8>> b2Raw;
    QList<QList<quint8>> b3Smooth;
    QList<QList<quint8>> b3Raw;
    QList<QList<quint8>> b4Smooth;
    QList<QList<quint8>> b4Raw;
    QList<QList<quint8>> b5Smooth;
    QList<QList<quint8>> b5Raw;
    struct can_frame last0000B100;
    struct can_frame last0000B200;
    struct can_frame last0000B300;
    struct can_frame last0000B400;
    struct can_frame last0000B500;

    // считаем пропущенные приемы
    int incomeFailCounter;
    // считаем количество ошибок при отправке
    int sendFailCounter;
    int incomePOFailCounter;
    int incomeGOFailCounter;
    bool firstInit;
    int firstInitCounter;
    int failConfigureCounter;

    // Блок для работы с буцп
    QMutex BUCPMutex;
    struct can_frame BUCP;
    quint8 getBUCPByte(int byteNumber);
    void setBUCPByte(quint8 BUCPByte, int byteNumber);

    QMutex BUCP2Mutex;
    struct can_frame BUCP2;
    quint8 getBUCP2Byte(int byteNumber);
    void setBUCP2Byte(quint8 BUCPByte, int byteNumber);

    QMutex BUCP3Mutex;
    struct can_frame BUCP3;
    quint8 getBUCP3Byte(int byteNumber);
    void setBUCP3Byte(quint8 BUCPByte, int byteNumber);

public slots:
    void CAN_init();
    void canTimerTimeoutRecv();
    void canTimerTimeoutSend();
signals:
    void canDataReady(struct can_frame);
    void canError();
    void canPOError();
    void canGOError();
private:
    qint64 lastResetMs;
};

#endif // MYCAN_H
