#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDebug>
#include <QtGui>
#include <QTimer>
#include <QFontDatabase>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <QPalette>
#include <QDate>
#include <globalsettings.h>
#include <currentstate.h>
#include <settingsstore.h>
#include <DebouncedInput.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <linux/can.h>
#include <sys/ioctl.h>

#include "camera/cameraplayer.h"
#include "camera/camerawidget.h"
#include "camera/cameraview.h"

//#include "gpio/gpio_class.h"
#include "gpio/gpio_worker.hpp"
#include "gpio/gpio_matrix.hpp"
#include "can/mycan.h"
#include "can/mycanengine.h"
#include "can/mycanj1939.h"

#include "engine.h"

#include "organs/backmagnet.h"
#include "organs/centralbroom.h"
#include "organs/blower.h"
#include "organs/frontrail.h"
#include "organs/organsenums.h"

// диагностика
#include <service/servicemainrightform.h>
#include <service/global/timeConfigure/serviceglobaldatetimeleftform.h>
#include <service/global/password/servicegeneralpasswordleftform.h>
#include <service/devices/hydraulics/servicedeviceshydraulicsleftform.h>
#include <service/devices/dkp/servicedevicesdkpleftform.h>
#include <service/other/engine/serviceotherengineleftform.h>
#include <service/other/light/serviceotherlightleftform.h>
#include <service/other/intervals/servicegpioserviceintervalleftform.h>
#include <Controllers/PhysicalButtonManager.h>
#include <Controllers/cancontroller.h>
#include <Controllers/prerollcontroller.h>
#include <Controllers/startercontroller.h>

#include <settings/settingsmainrightform.h>
#include <settings/gpio/wifi/settingswifileftform.h>
#include <settings/settings/configuration/settingssettingsconfigurationleftform.h>
#include "settings/settingsform.h"

#include "blockform.h"
#include "screenlog.h"

//логгер (черный ящик)
#include "BoolStateWatcher.h"
#include "MedianFilter.h"
#include "logger.h"
#include "maintenancetracker.h"

#include "log/MessageList.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

Q_DECLARE_METATYPE(struct can_frame);

//#define OUT_BACKLIGHT           147
//#define OUT_STARTER             149

//// транспортный режим
//#define IN_AVAR                 79
//// выбрать правый отвал или отвал право
//#define IN_LEFT_UP              58
//// выбрать левый отвыали или отвал лево
//#define IN_LEFT_DOWN            60
//// щетка право или обороты меньше
//#define IN_RIGHT_UP             57
//// щетка лево или обороты больше
//#define IN_RIGHT_DOWN           128
//// стартер двс
//#define IN_START_STOP            78
//// выбор левой потом правой продувки или смена направления обдува
//#define IN_CHANGE_DIRECT        76

#define VEHICLE_OFF_SPEED 15

#define OUT_PVI_TEMP        146
#define PVI_TEMP_EDGE_OFF   52
#define PVI_TEMP_EDGE_ON    54


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct WorkMode
    {
        bool backMagnet;
        bool centralBroomLeft;
        bool centralBroomRight;
        bool centralBroomFlow;
        bool centralBroomPress;
        bool frontDumpLeft;
        bool frontDumpRight;
        bool frontDumpFlow;
        bool blowLeft;
        bool blowRight;
        bool frmBroom;
        bool frmMagnet;
        bool frmKung;
        quint8 sweepType;
    };

    struct BtnData{
        QString commonPath;
        QString selectedPath;
        QString normalPath;
        std::function<void()> onPress;
        std::function<void()> onRelease;
    };

    struct CleanConfiguration
    {
        bool frontDumpUse;
        bool magnetUse;
        bool centralBroomUse;
        bool blowUse;
    };

    enum SmetType
    {
        NoneSweep   = 0,
        LeafSweep   = 1,
        LightSweep  = 2,
        MediumSweep = 3,
        HeavySweep  = 4
    };



    MainWindow(int argc, char *argv[], QWidget *parent = nullptr);
    ~MainWindow();

    void removeBadSettings();

    /*PWM*/
    void buttonsLightCheck();
    int buttonsLightLevelEdge;
    int buttonsLightLevel;

    bool stopInProgress;

    //qint8 getFilteredTemp();

    void readValues();
    QVariant readSettingsValue(QString name);
    //QMap<GPIOInput, bool> physBtnsStates;
    //QMap<QString, QVariant> defaultValues;
    QMap<QString, quint32> TOValues;
    QMap<QString, quint32> TOCurValues;
    QMap<QString, QString> TONameValues;
    QMap<QString, bool> TOAlarmValues;
    QMap<QString, quint8> TOSourceValues;
    quint32 engineToday;
    QDate dateToday;

    void addElement(int element_, QString name_, int board_, int channel_, quint8 type_, quint8 median_type_ = 0, quint8 pwm_type_ = 0, quint8 pfm_low_type_ = 0, quint8 pfm_high_type_ = 0, quint8 value_change_speed_ = 0);
    void getSystemConfigure(SystemConfigure* dst);
    void getElements(QMap<int, SystemElement*>* dst);
    void saveSystemConfigure();
    void readSystemConfigure();
    SystemConfigure systemConfigure;
    QMap<int, SystemElement*> systemElements;

    bool isIdleMode();
    bool canStart();
    bool isOrgansTransitioning();
    void applyWorkModeToDeployedOrgans();
    bool organsWereTransitioning;

    bool waitOnStartAlarmed;
    bool cleanWrongSpeedAlarmed;
    int minCleanSpeed, maxCleanSpeed;

    int backGearCounter;

    void showWorkMode();
    void checkAndShowStatus();
    void showStatus(QLabel *label, bool check, QString messageOn = NULL, QString messageOff = NULL);
    void showPultOffIgnition();
    void updateFRM();

    void showStartClean();
    void showModeButton();
    void showMatrixFRMButton();

    bool inHomeState();

    CurrentState *currentState;
    //quint8 startCleanTimeCounter;
    quint8 centralBroomLeftTimeCounter;
    quint8 centralBroomRightTimeCounter;
    quint8 frontDumpLeftTimeCounter;
    quint8 frontDumpRightTimeCounter;
    quint8 blowerTimeCounter;
    //quint8 frmTimeCounter;
    //quint8 leftModeTimeCounter;
    //quint8 rightModeTimeCounter;

    void updateCentralBroomLeft();
    void updateCentralBroomRight();
    void showDumpLeft();
    void showDumpRight();
    void showBlower();
    void showPauseButton();

    struct WorkMode workMode;
    struct CleanConfiguration cleanConfiguration;

    QSettings *settings;


    MedianFilter hydroTempFilter;// Фильтрация температуры гидравлики
    // Антидребезг кнопок матричной клавиатуры
    DebouncedInput startCleanKey;
    DebouncedInput modeLeftKey;
    DebouncedInput modeRightKey;
    DebouncedInput frmKey;
    DebouncedInput pauseKey;

    Engine *engine;
    CentralBroom *broomCentral;
    Blower *blower;
    BackMagnet *backMagnet;
    FrontRail *frontRail;
    GPIOController *_gpio;

    quint8 currentKV;
    bool KVControl;

//    quint8 menuMode;
    GlobalSettings *globals;
    // переменные интерфейса
    bool startClean;
    //bool starter; // нажата ли кнопка стартера
    bool chooseBackCamera;
    bool chooseFrm;

    bool engineTempCrit;
    bool engineTempWarn;
    int engineTempWarnTimer;
    int engineTempWarnEdge;
    int engineTempGoodValue;
    int engineTempWarnValue;
    int engineTempCritValue;
    int hydroTempGoodValue;
    int hydroTempWarnValue;
    int hydroTempCritValue;
    bool hydroTempCrit;
    bool hydroTempWarn;
    quint8 fanAccelRate;
    //QList<qint8> hydroTempMedianBuffer;
    //QList<qint8> hydroTempBuffer;
    //int hydroTempCounterToShow;
    quint32 beamTimeCounter;
    //quint32 cleanOffTimeCounter;
    bool backControl;
    quint32 backIdleCounter;
    bool backLight;
    quint32 backBlockCounter;
    quint32 backLightTimeCounter;
    //quint32 ignitionOffTimer;
    qint32 chooseGabaritCount;

    bool Password_accepted;

    //int pauseCleanTimeCounter;
    bool pauseActive;

    StarterController *starter;
    PrerollController *preroll;
    CanController *can;

    // int starterMaxWorkSec;
    // int starterPauseSec;
    // int starterMaxAttempts;
    // int rollMaxWorkSec;
    // int rollPauseSec;
    // int rollMaxAttempts;
    //int requireRollAfterDays;
    //int lowTempRequireWarm;
    //int waterSensorRedHours;
    //int airFilterRedHours;

    bool waterSensorEmergencyMode;
    bool airFilterEmergencyMode;

    bool disableTemperatureBlock;
    bool ignoreAllEmergency;

    bool engineRunStatePrev;

    bool serviceIgnitionAutoRestoreBlocked;
    bool logNeedWarmShown;
    // bool waterSensorActivePrev;
    // bool airFilterActivePrev;
    // bool oilFilterActivePrev;
    // bool heatRelayActivePrev;
    // bool waterSensorTimeStarted;
    // bool airFilterTimeStarted;
    quint32 waterSensorStartedAt;
    quint32 airFilterStartedAt;

    float hydroTempK;
    float hydroTempB;
    float hydraulicPressureK[4];
    float hydraulicPressureB[4];

    float hydraulicPressureValue(int index) const;
    void toggleAllFrm();


    int oldCentralOffset;

    QLabel* serviceSetingsName;

    ServiceMainRightForm* serviceMainRightForm;
    ServiceGlobalDateTimeLeftForm* serviceGlobalDateTimeLeftForm;
    ServiceGeneralPasswordLeftForm* serviceGeneralPasswordLeftForm;
    ServiceDevicesHydraulicsLeftForm* serviceDevicesHydraulicsLeftForm;
    ServiceDevicesDKPLeftForm* serviceDevicesDKPLeftForm;
    ServiceOtherEngineLeftForm* serviceOtherEngineLeftForm;
    ServiceOtherLightLeftForm* serviceOtherLightLeftForm;
    ServiceGPIOServiceIntervalLeftForm* serviceGPIOServiceIntervalLeftForm;

    SettingsMainRightForm* settingsMainRightForm;
    SettingsWifiLeftForm* settingsWifiLeftForm;
    SettingsForm* settingsForm;
    SettingsSettingsConfigurationLeftForm* settingsSettingsConfigurationLeftForm;

    Logger * logger;
    //gpio_class *gp;
    GPIOWorker *gpio;
    GPIOMatrix *gpioMatirx;

    BlockForm *blockScreen;

    int speedCounter;
    int voltageCounter;
    int frontRPMCounter;
    int frontRPM;

    bool showCheckEngine;

    MyCanEngine * canForEngine;
    MyCanJ1939 *canj1939;
    MyCanJ1939 *canj1939Main;

private:
    MyCan *can0;
    QString blowerVertPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_lift_";
    QString blowerHorPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_purgeUnit_turn_";
    QString dumpVertPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_lift_";
    QString dumpHorPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_";
    QString broomVertPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsFront_lift_";
    QString broomHorPath = "background-image: url(:/Images/Images/main/buttons/configuration_button_rotatingBroomsBelow_";
    //"background-image: url(:/Images/Images/main/buttons/configuration_button_dozerBlade_turn_off.png);";

    Ui::MainWindow *ui;
    QTimer mainProgressTimer;
    QTimer repaintTimer;
    QTimer oneSecondTimer;
    QTimer goHomeTimer;
    QFont font;

    PhysicalButtonManager m_buttonManager;
    //std::unordered_map<std::pair<CleanConfiguration, Direction>, double> dict;

    //void addLog(QString text, LogStatus logStatus);

    void checkEngineAndRollLocks();


    void updateSensorAndWarningIndicators();
    void updateIndicatorPixmap(QLabel* label, const QString& colorName, const QString& baseName);

    // void updatePrerollButtonsVisual();
    // void cachePrerollButtons();
    void restoreIgnitionAfterRoll();

    void setBlowerState();
    void changeSweepMode(quint8 mode);
    void setDumpState();

    void setBroomState();

    //void setStyle(QPushButton *btn, QString path);
    void tryMoveBroomDown();
    void tryMoveBroomUp();
    void tryMoveBroomLeft();
    void tryMoveBroomRight();
    void broomSingleMovement(organsEnums::Direction dir);
    bool isDisabled();
    void createTimers();

    void createButtons();
    bool isBroomDownPressed();
    bool isBroomUpPressed();
    void onBroomReleased();
    void onRailReleased();
    void onBlowerReleased();
    void updateButtonsIcons();
    void updateButtonsActiveState();
    void updateOrgansStates();
    void setBtnState(QWidget *widget, QString path, std::function<void ()> handler);
    //void setBtnState(QWidget *widget, QPushButton *btn, QString path, std::function<void ()> handler);
    void selectBtnState(bool gpioPressed, QLabel *lbl, QPushButton *btn, QString onPath, QString offPath, std::function<void ()> onPress, std::function<void ()> onRelease);
    void selectBtnState(bool gpioPressed, QPushButton *btn, QString onPath, QString offPath, std::function<void ()> onPressHandler, std::function<void ()> onReleaseHandler);
    void loadAndSetFonts();
    void setDefaultWorkMode();
    void setDefaultSettings();
    QString getOrganText(organsEnums::Organ organ);
    void printOrganStatus(organsEnums::Organ organ, organsEnums::Direction direction, bool state);

    void updateBroomBtnsView();
    void setRandomPassword(int pass, QString passwordName);
    void setDefaultValues();
    void createFormsAndHide();
    void updateDumpBtnsView();

    ViewController *view;
    SettingsReader *_settingsReader;
    SettingsStore *settingsStore = nullptr;
    MaintenanceTracker *maintenanceTracker = nullptr;
    void configureChannelTypes();
    void insertValues();
    bool isSpeedTooHigh();
    void stopCleaningForSafety(const QString &reason);
    void checkEngineOverheat();
    void setBroomPressed(bool);
    void registerPhysButtons();
    void updatePhysButtons();
    void configureFilters();
    BoolStateWatcher m_oilFilterWatcher;
    BoolStateWatcher m_waterSensorWatcher;
    BoolStateWatcher m_airFilterWatcher;
    BoolStateWatcher m_heatRelayWatcher;
    void configureButtons();

    BoolStateWatcher m_broomUpWatcher;
    BoolStateWatcher m_broomDownWatcher;
    BoolStateWatcher m_broomLeftWatcher;
    BoolStateWatcher m_broomRightWatcher;
    BoolStateWatcher m_dumpUpWatcher;
    BoolStateWatcher m_dumpDownWatcher;
    BoolStateWatcher m_dumpLeftWatcher;
    BoolStateWatcher m_dumpRightWatcher;
    BoolStateWatcher m_blowUpWatcher;
    BoolStateWatcher m_blowDownWatcher;
    BoolStateWatcher m_blowLeftWatcher;
    BoolStateWatcher m_blowRightWatcher;
    BoolStateWatcher m_dumpFlowWatcher;
    BoolStateWatcher m_broomFlowWatcher;

    void updateButtonsUniversal();
    //void configureButtons();
    void setBtnView(bool isPressed, QLabel *lbl, QPushButton *btn, QString onPath, QString offPath);
    QString getBlowerDefaultIcon();
    QString getDumpDefaultIcon();
    QString getBroomDefaultIcon();
    void setVertButtonsView(bool state);
    BoolStateWatcher m_broomPressWatcher;
    void setBroomFlow(bool state);
    void setDumpFlow(bool state);
    void configureMovementStart(QPushButton *btn, QLabel *lbl, QString path, std::function<bool ()> isBusy, std::function<void ()> handler);
    void configureMovementStop(QPushButton *btn, QLabel *lbl, QString path, std::function<bool ()> isBusy, std::function<void ()> handler);
    void startCleaning(bool state);
    void updateBroomFlowPressIcon();
    void setButtonVisualState(QPushButton *button, QLabel *iconLabel, const QString &style, bool wasDown);
signals:
    void resetComplete();
    void Send_Pass_2_pass_form(int);
    void Send_SecretPass_2_pass_form(int);
    void Pass_close();
public :
    bool isBroomTransitioning();
    bool isDumpTransitioning();
    bool isMagnetTransitioning();
    bool isBlowTransitioning();
    QString getMovementText(organsEnums::Direction direction);
    void setBroomFlowView(bool state);
    void setBroomPressView(bool state);
    void setDumpFlowView(bool state);

    void resetPassword();
    ViewController *getView();
    SettingsReader * getReader();
    //void invertIgnition();
    void checkIgnition();
    void tryToDisableBroomFlow();
    void tryToDisableDumpFlow();
    void setServiceFormName(QWidget *form, QString name);
    void changeBlowDirection(bool isRight);
    void clearBlowDirection();
    bool getGPIOInput(GPIOInput id);
public slots:
    //void messageListPressed();
    void settingsAskPassword();
    void diagAskPassword();
    void passwordSettingsOk(int);
    void passwordDiagOk(int);
    void resetDevices();
    //void startIgnition();
    void canPOError();
    void canJ1939Error();
    void canJ1939MainError();
    void incomeData(struct can_frame frame);
    void incomeDataJ1939(quint32 pgn, quint8 sa, QByteArray data);
    void incomeDataJ1939Main(quint32 pgn, quint8 sa, QByteArray data);
    void serviceClosed();
    void settingsClosed();

private slots:
    void repaintProgress();
    void mainProgress();
    void on_pushButton_startstop_clicked();
    void oneSecond();
    void on_pushButton_service_clicked();
    void on_pushButton_lightSweep_clicked();
    void on_pushButton_leafSweep_clicked();
    void on_pushButton_mediumSweep_clicked();
    void on_pushButton_heavySweep_clicked();
    void on_pushButton_settings_clicked();

    void on_pushButton_centralBroomUp_clicked();
    void on_pushButton_centralBroomDown_clicked();
    void on_pushButton_centralBroomLeft_clicked();
    void on_pushButton_centralBroomRight_clicked();

    void on_pushButton_dumpUp_clicked();
    void on_pushButton_dumpDown_clicked();
    void on_pushButton_dumpLeft_clicked();
    void on_pushButton_dumpRight_clicked();

    void on_pushButton_blowerUp_clicked();
    void on_pushButton_blowerDown_clicked();
    void on_pushButton_blowerLeft_clicked();
    void on_pushButton_blowerRight_clicked();

    void on_pushButton_dumpFlow_clicked();
    void on_pushButton_centralBroomFlow_clicked();
    void on_pushButton_centralBroomPress_clicked();

    void on_pushButton_backMagnet_clicked();
    void on_pushButton_frmKung_clicked();
    void on_pushButton_frmBroom_clicked();
    void on_pushButton_frmMagnet_clicked();
    void on_pushButton_homeState_clicked();

};
#endif // MAINWINDOW_H
