#include "QFile"
#include <QDebug>
#include <QScroller>
#include <QScrollerProperties>

#include "settingsform.h"
#include "password_form.h"
#include "ui_settingsform.h"

#include "mainwindow.h"

SettingsForm::SettingsForm(QSettings *settings_, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::SettingsForm)
{
    ui->setupUi(this);
    settings = settings_;

    parent = parent_;

    elementsScrollArea = new QScrollArea(ui->frame_background);
    QRect scrollRect = ui->gridLayoutWidget->geometry();
    scrollRect.setWidth(qMin(ui->frame_background->width(), scrollRect.width() + 20));
    elementsScrollArea->setGeometry(scrollRect);
    elementsScrollArea->setStyleSheet("QScrollArea {background-color: transparent; border-style:none;outline: none;}");
    elementsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    elementsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    elementsScrollArea->setFrameShape(QFrame::NoFrame);
    elementsScrollArea->viewport()->setAutoFillBackground(false);
    elementsScrollArea->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    elementsScrollArea->viewport()->setStyleSheet("background: transparent;");

    ui->gridLayoutWidget->setParent(elementsScrollArea);
    ui->gridLayoutWidget->setAutoFillBackground(false);
    ui->gridLayoutWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    ui->gridLayoutWidget->setStyleSheet("background: transparent;");
    elementsScrollArea->setWidget(ui->gridLayoutWidget);
    elementsScrollArea->setWidgetResizable(false);

    QScroller::grabGesture(elementsScrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    QScrollerProperties scrollerProps = QScroller::scroller(elementsScrollArea->viewport())->scrollerProperties();
    scrollerProps.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    scrollerProps.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1.0);
    QScroller::scroller(elementsScrollArea->viewport())->setScrollerProperties(scrollerProps);
}

SettingsForm::~SettingsForm()
{
    delete ui;
}

void SettingsForm::fillElements()
{
    auto reader = ((MainWindow*)parent)->getReader();
    for (int i = 0; i < elementsGlobal.count(); i++)
        elementsGlobal.at(i)->deleteLater();
    elementsGlobal.clear();
    for (int i = 0; i < elementsMediumSweep.count(); i++)
        elementsMediumSweep.at(i)->deleteLater();
    elementsMediumSweep.clear();
    for (int i = 0; i < elementsLightSweep.count(); i++)
        elementsLightSweep.at(i)->deleteLater();
    elementsLightSweep.clear();
    for (int i = 0; i < elementsHeavySweep.count(); i++)
        elementsHeavySweep.at(i)->deleteLater();
    elementsHeavySweep.clear();
    for (int i = 0; i < elementsFrontTimings.count(); i++)
        elementsFrontTimings.at(i)->deleteLater();
    elementsFrontTimings.clear();
    for (int i = 0; i < elementsMiddleTimings.count(); i++)
        elementsMiddleTimings.at(i)->deleteLater();
    elementsMiddleTimings.clear();
    for (int i = 0; i < elementsBackTimings.count(); i++)
        elementsBackTimings.at(i)->deleteLater();
    elementsBackTimings.clear();

    elementsGlobal.append(new SettingsElement("Порог включения уборки(км/ч)", "Global", "enableCleanSpeed", settings->value("Global/enableCleanSpeed").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Задержка включения зажигания(С)", "Global", "restartIgnitionDelay", settings->value("Global/restartIgnitionDelay").toInt(), 0, 10));
    elementsGlobal.append(new SettingsElement("Порог отключения уборки(км/ч)", "Global", "disableCleanSpeed", settings->value("Global/disableCleanSpeed").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Рабочая температура двигателя(C)", "Global", "engineTempGood", settings->value("Global/engineTempGood").toInt(), 0, 120));
    elementsGlobal.append(new SettingsElement("Опасная температура двигателя(C)", "Global", "engineTempWarn", settings->value("Global/engineTempWarn").toInt(), 0, 120));
    elementsGlobal.append(new SettingsElement("Критическая температура двигателя(C)", "Global", "engineTempCrit", settings->value("Global/engineTempCrit").toInt(), 0, 120));
    elementsGlobal.append(new SettingsElement("Время ожидания охлаждения двигателя(м)", "Global", "engineTempWarnTime", settings->value("Global/engineTempWarnTime").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Рабочая температура гидросистемы(C)", "Global", "hydroTempGood", settings->value("Global/hydroTempGood").toInt(), -40, 100));
    elementsGlobal.append(new SettingsElement("Опасная температура гидросистемы(C)", "Global", "hydroTempWarn", settings->value("Global/hydroTempWarn").toInt(), -40, 100));
    elementsGlobal.append(new SettingsElement("Критическая температура гидросистемы(C)", "Global", "hydroTempCrit", settings->value("Global/hydroTempCrit").toInt(), -40, 100));
    elementsGlobal.append(new SettingsElement("Пересчет температуры гидросистемы коэф k (kx+b)", "Global", "hydroTempK", settings->value("Global/hydroTempK").toFloat(), -200, 200, 0.1));
    elementsGlobal.append(new SettingsElement("Пересчет температуры гидросистемы коэф b (kx+b)", "Global", "hydroTempB", settings->value("Global/hydroTempB").toInt(), -500, 500));
    elementsGlobal.append(new SettingsElement("Давление ТИ1 коэф b (kx+b)", "Global", "hydraulicPressure1B", settings->value("Global/hydraulicPressure1B").toFloat(), -500, 500, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ1 коэф k (kx+b)", "Global", "hydraulicPressure1K", settings->value("Global/hydraulicPressure1K").toFloat(), -200, 200, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ2 коэф k (kx+b)", "Global", "hydraulicPressure2K", settings->value("Global/hydraulicPressure2K").toFloat(), -200, 200, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ2 коэф b (kx+b)", "Global", "hydraulicPressure2B", settings->value("Global/hydraulicPressure2B").toFloat(), -500, 500, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ3 коэф k (kx+b)", "Global", "hydraulicPressure3K", settings->value("Global/hydraulicPressure3K").toFloat(), -200, 200, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ3 коэф b (kx+b)", "Global", "hydraulicPressure3B", settings->value("Global/hydraulicPressure3B").toFloat(), -500, 500, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ4 коэф k (kx+b)", "Global", "hydraulicPressure4K", settings->value("Global/hydraulicPressure4K").toFloat(), -200, 200, 0.1));
    elementsGlobal.append(new SettingsElement("Давление ТИ4 коэф b (kx+b)", "Global", "hydraulicPressure4B", settings->value("Global/hydraulicPressure4B").toFloat(), -500, 500, 0.1));
    elementsGlobal.append(new SettingsElement("Показывать checkEngine (1 = Да)", "Global", "showCheckEngine", settings->value("Global/showCheckEngine").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Инвертированность ДКП (1 = Да)", "Global", "DKPInversion", settings->value("Global/DKPInversion").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Холостые(об)", "Engine", "rpm.None", settings->value("Engine/rpm.None").toInt(), 0, 3000, 100));
    elementsGlobal.append(new SettingsElement("Включение охлаждения(C)", "Engine", "rpm.VentEdge", settings->value("Engine/rpm.VentEdge").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Адрес()", "Engine", "addr", settings->value("Engine/addr").toInt(), 0, 255, 1));
    elementsGlobal.append(new SettingsElement("Требуемые дни для предпускового прогрева", "Engine", "startRollRequiredDays", settings->value("Engine/startRollRequiredDays").toInt(), 0, 30));
    elementsGlobal.append(new SettingsElement("Порог низкой температуры запуска(C)", "Engine", "startLowTemperatureEdge", settings->value("Engine/startLowTemperatureEdge").toInt(), -40, 10));
    elementsGlobal.append(new SettingsElement("Макс. время работы стартера(с)", "Engine", "starterMaxWorkSec", settings->value("Engine/starterMaxWorkSec").toInt(), 0, 60));
    elementsGlobal.append(new SettingsElement("Макс. попыток стартера", "Engine", "starterMaxAttempts", settings->value("Engine/starterMaxAttempts").toInt(), 0, 10));
    elementsGlobal.append(new SettingsElement("Пауза стартера(с)", "Engine", "starterPauseSec", settings->value("Engine/starterPauseSec").toInt(), 0, 300));
    elementsGlobal.append(new SettingsElement("Макс. время предпускового прогрева(с)", "Engine", "rollMaxWorkSec", settings->value("Engine/rollMaxWorkSec").toInt(), 0, 60));
    elementsGlobal.append(new SettingsElement("Пауза предпускового прогрева(с)", "Engine", "rollPauseSec", settings->value("Engine/rollPauseSec").toInt(), 0, 300));
    elementsGlobal.append(new SettingsElement("Макс. попыток предпускового прогрева", "Engine", "rollMaxAttempts", settings->value("Engine/rollMaxAttempts").toInt(), 0, 10));
    elementsGlobal.append(new SettingsElement("Часы до предупреждения датчика воды", "Engine", "waterSensorRedHours", settings->value("Engine/waterSensorRedHours").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Часы до предупреждения воздушного фильтра", "Engine", "airFilterRedHours", settings->value("Engine/airFilterRedHours").toInt(), 0, 100));
    elementsGlobal.append(new SettingsElement("Аварийный режим датчика воды (1=Да)", "Engine", "waterSensorEmergencyMode", settings->value("Engine/waterSensorEmergencyMode").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Аварийный режим воздушного фильтра (1=Да)", "Engine", "airFilterEmergencyMode", settings->value("Engine/airFilterEmergencyMode").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Игнорировать все аварийные блокировки (1=Да)", "Engine", "ignoreAllEmergency", settings->value("Engine/ignoreAllEmergency").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Отключить требование прокрутки (1=Да)", "Engine", "disableRollRequirement", settings->value("Engine/disableRollRequirement").toBool(), 0, 1));
    elementsGlobal.append(new SettingsElement("Отключить блокировку ДВС по температуре (1=Да)", "Engine", "disableTemperatureBlock", settings->value("Engine/disableTemperatureBlock").toBool(), 0, 1));

    elementsLightSweep.append(new SettingsElement("Скорость вращения щетки листья(%)", "CentralBroom", "speeds.LeafSweep", settings->value("CentralBroom/speeds.LeafSweep").toInt(), 0, 100));
    elementsLightSweep.append(new SettingsElement("Скорость выдува листья(%)", "Blower", "speeds.LeafSweep", settings->value("Blower/speeds.LeafSweep").toInt(), 0, 100));
    elementsLightSweep.append(new SettingsElement("Обороты двигателя листья(об)", "Engine", "rpm.LeafSweep", settings->value("Engine/rpm.LeafSweep").toInt(), 0, 3000, 100));
    elementsLightSweep.append(new SettingsElement("Скорость вращения щетки легкий(%)", "CentralBroom", "speeds.LightSweep", settings->value("CentralBroom/speeds.LightSweep").toInt(), 0, 100));
    elementsLightSweep.append(new SettingsElement("Скорость выдува легкий(%)", "Blower", "speeds.LightSweep", settings->value("Blower/speeds.LightSweep").toInt(), 0, 100));
    elementsLightSweep.append(new SettingsElement("Обороты двигателя легкий(об)", "Engine", "rpm.LightSweep", settings->value("Engine/rpm.LightSweep").toInt(), 0, 3000, 100));

    elementsMediumSweep.append(new SettingsElement("Скорость вращения щетки средний(%)", "CentralBroom", "speeds.MediumSweep", settings->value("CentralBroom/speeds.MediumSweep").toInt(), 0, 100));
    elementsMediumSweep.append(new SettingsElement("Скорость выдува средний(%)", "Blower", "speeds.MediumSweep", settings->value("Blower/speeds.MediumSweep").toInt(), 0, 100));
    elementsMediumSweep.append(new SettingsElement("Обороты двигателя средний(об)", "Engine", "rpm.MediumSweep", settings->value("Engine/rpm.MediumSweep").toInt(), 0, 3000, 100));

    elementsHeavySweep.append(new SettingsElement("Скорость вращения щетки тяжелый(%)", "CentralBroom", "speeds.HeavySweep", settings->value("CentralBroom/speeds.HeavySweep").toInt(), 0, 100));
    elementsHeavySweep.append(new SettingsElement("Скорость выдува тяжелый(%)", "Blower", "speeds.HeavySweep", settings->value("Blower/speeds.HeavySweep").toInt(), 0, 100));
    elementsHeavySweep.append(new SettingsElement("Обороты двигателя тяжелый(об)", "Engine", "rpm.HeavySweep", settings->value("Engine/rpm.HeavySweep").toInt(), 0, 3000, 100));

    elementsFrontTimings.append(new SettingsElement("Отвал опускание(с)", "Dump", "timeouts.DumpDownOut", settings->value("Dump/timeouts.DumpDownOut").toInt(), 0, 60));
    elementsFrontTimings.append(new SettingsElement("Отвал поднимание(с)", "Dump", "timeouts.DumpDownIn", settings->value("Dump/timeouts.DumpDownIn").toInt(), 0, 60));
    elementsFrontTimings.append(new SettingsElement("Отвал разворачивание(с)", "Dump", "timeouts.DumpSlideOut", settings->value("Dump/timeouts.DumpSlideOut").toInt(), 0, 60));
    elementsFrontTimings.append(new SettingsElement("Отвал сворачивание(с)", "Dump", "timeouts.DumpSlideIn", settings->value("Dump/timeouts.DumpSlideIn").toInt(), 0, 60));
    elementsFrontTimings.append(new SettingsElement("Отвал плавающий(с)", "Dump", "timeouts.DumpFlowOut", settings->value("Dump/timeouts.DumpFlowOut").toInt(), 0, 60));
    elementsFrontTimings.append(new SettingsElement("Отвал отскок(с)", "Dump", "timeouts.DumpBounceOut", settings->value("Dump/timeouts.DumpBounceOut").toFloat(), 0, 60, 0.1));
    // время хода: по нему программа оценивает положение отвала (датчика положения нет)
    elementsFrontTimings.append(new SettingsElement("Отвал поворот от упора до упора(с)", "Dump", "slideTimeSec", reader->readSettingsValue("Dump/slideTimeSec").toFloat(), 0.5, 60, 0.1));

    elementsMiddleTimings.append(new SettingsElement("Щетка поднимание(с)", "CentralBroom", "timeouts.BroomDownIn", reader->readSettingsValue("CentralBroom/timeouts.BroomDownIn").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка опускание(с)", "CentralBroom", "timeouts.BroomDownOut", reader->readSettingsValue("CentralBroom/timeouts.BroomDownOut").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка поворачивание назад(с)", "CentralBroom", "timeouts.BroomSlideIn", reader->readSettingsValue("CentralBroom/timeouts.BroomSlideIn").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка поворачивание(с)", "CentralBroom", "timeouts.BroomSlideOut", reader->readSettingsValue("CentralBroom/timeouts.BroomSlideOut").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка раскручивание(с)", "CentralBroom", "timeouts.BroomRotateOut", settings->value("CentralBroom/timeouts.BroomRotateOut").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка остановка(с)", "CentralBroom", "timeouts.BroomRotateIn", settings->value("CentralBroom/timeouts.BroomRotateIn").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка плавающая(с)", "CentralBroom", "timeouts.BroomFlowOut", settings->value("CentralBroom/timeouts.BroomFlowOut").toInt(), 0, 60));
    elementsMiddleTimings.append(new SettingsElement("Щетка отскок(с)", "CentralBroom", "timeouts.BroomBounceOut", settings->value("CentralBroom/timeouts.BroomBounceOut").toFloat(), 0, 60, 0.1));
    // время хода: по нему программа оценивает положение щётки (датчиков высоты и середины нет).
    // Лучше ставить чуть меньше реального - щётка раскрутится немного раньше касания и остановится раньше при подъёме
    elementsMiddleTimings.append(new SettingsElement("Щетка ход портала вниз(с)", "CentralBroom", "lowerTimeSec", reader->readSettingsValue("CentralBroom/lowerTimeSec").toFloat(), 0.5, 60, 0.1));
    elementsMiddleTimings.append(new SettingsElement("Щетка ход портала вверх(с)", "CentralBroom", "raiseTimeSec", reader->readSettingsValue("CentralBroom/raiseTimeSec").toFloat(), 0.5, 60, 0.1));
    elementsMiddleTimings.append(new SettingsElement("Щетка падение портала в плавании(с)", "CentralBroom", "flowDropTimeSec", reader->readSettingsValue("CentralBroom/flowDropTimeSec").toFloat(), 0.5, 60, 0.1));
    elementsMiddleTimings.append(new SettingsElement("Щетка порог вращения(% хода портала)", "CentralBroom", "spinHeightPercent", reader->readSettingsValue("CentralBroom/spinHeightPercent").toInt(), 0, 100, 5));
    elementsMiddleTimings.append(new SettingsElement("Щетка поворот от упора до упора(с)", "CentralBroom", "slideTimeSec", reader->readSettingsValue("CentralBroom/slideTimeSec").toFloat(), 0.5, 60, 0.1));

    elementsBackTimings.append(new SettingsElement("Продувка вниз(с)", "Blower", "timeouts.BlowerDownOut", settings->value("Blower/timeouts.BlowerDownOut").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка вверх(с)", "Blower", "timeouts.BlowerDownIn", settings->value("Blower/timeouts.BlowerDownIn").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка включение направления(с)", "Blower", "timeouts.BlowerSlideOut", settings->value("Blower/timeouts.BlowerSlideOut").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка выключение направления(с)", "Blower", "timeouts.BlowerSlideIn", settings->value("Blower/timeouts.BlowerSlideIn").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка раскручивание(с)", "Blower", "timeouts.BlowerRotateOut", settings->value("Blower/timeouts.BlowerRotateOut").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка остановка(с)", "Blower", "timeouts.BlowerRotateIn", settings->value("Blower/timeouts.BlowerRotateIn").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Продувка темп разгона/остановки(%/c)", "Blower", "fanAccelRate", settings->value("Blower/fanAccelRate").toInt(), 0, 100));

    elementsBackTimings.append(new SettingsElement("Магнит опускание(с)", "BackMagnet", "timeouts.BackMagnetDownOut", settings->value("BackMagnet/timeouts.BackMagnetDownOut").toInt(), 0, 60));
    elementsBackTimings.append(new SettingsElement("Магнит поднимание(с)", "BackMagnet", "timeouts.BackMagnetDownIn", settings->value("BackMagnet/timeouts.BackMagnetDownIn").toInt(), 0, 60));

//    elementsCleanConfigure.append(new SettingsElement("Передняя щетка(вкл=1,выкл=0)", "CleanConfiguration", "frontBroomUse", settings->value("CleanConfiguration/frontBroomUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Магнитная плита(вкл=1,выкл=0)", "CleanConfiguration", "magnetUse", settings->value("CleanConfiguration/magnetUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Вода(вкл=1,выкл=0)", "CleanConfiguration", "waterUse", settings->value("CleanConfiguration/waterUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Боковые щетки(вкл=1,выкл=0)", "CleanConfiguration", "sideBroomsUse", settings->value("CleanConfiguration/sideBroomsUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Продувка(вкл=1,выкл=0)", "CleanConfiguration", "blowUse", settings->value("CleanConfiguration/blowUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Подборщик и задняя щетка(вкл=1,выкл=0)", "CleanConfiguration", "pickupAndBackBroomUse", settings->value("CleanConfiguration/pickupAndBackBroomUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Задний подборщик(вкл=1,выкл=0)", "CleanConfiguration", "backPickupUse", settings->value("CleanConfiguration/backPickupUse").toInt(), 0, 1));
//    elementsCleanConfigure.append(new SettingsElement("Керхер(вкл=1,выкл=0)", "CleanConfiguration", "kerherUse", settings->value("CleanConfiguration/kerherUse").toInt(), 0, 1));
}

void SettingsForm::pressGrid()
{
    ui->gridLayout->setContentsMargins(4, 4, 4, 4);

    // поджатие всех строк
    ui->gridLayout->setRowStretch(5, 1);
    ui->gridLayout->setColumnStretch(0, 0);
    ui->gridLayout->setColumnStretch(4, 0);

    // минималки по размерам чтобы не расползались строки
    ui->gridLayout->setColumnMinimumWidth(0, 160);
    ui->gridLayout->setColumnMinimumWidth(1, 160);
    ui->gridLayout->setColumnMinimumWidth(2, 160);
    ui->gridLayout->setColumnMinimumWidth(3, 160);
    ui->gridLayout->setColumnMinimumWidth(4, 160);

    ui->gridLayout->setRowMinimumHeight(0, 105);
    ui->gridLayout->setRowMinimumHeight(1, 105);
    ui->gridLayout->setRowMinimumHeight(2, 105);
    ui->gridLayout->setRowMinimumHeight(3, 105);
    ui->gridLayout->setRowMinimumHeight(4, 105);

    // шаг между элементами
    ui->gridLayout->setHorizontalSpacing(8);
    // выравнивание всех элементов сетки чтобы отступы были одинаковыми
    ui->gridLayout->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
}

void SettingsForm::clearGrid()
{
    QLayoutItem* item;
    while ( ( item = ui->gridLayout->takeAt( 0 ) ) != NULL )
    {
        item->widget()->setParent(NULL);
        delete item;
    }
}

void SettingsForm::showElements(int col_cnt, QList<SettingsElement*> & elements)
{
    clearGrid();

    int current_col = 0;
    for (int i = 0; i < elements.size(); i++)
    {
        ui->gridLayout->addWidget(elements.at(i), i / col_cnt, current_col);
        current_col++;
        if (current_col == col_cnt)
            current_col = 0;
    }

    const int rows = (elements.size() + col_cnt - 1) / col_cnt;
    const int colWidth = 160;
    const int rowHeight = 105;
    const int spacing = 8;
    const int margins = 4;

    // Ширину фиксируем по viewport, чтобы отключить горизонтальную прокрутку/свайп.
    const int contentWidth = elementsScrollArea->viewport()->width();
    const int viewportHeight = elementsScrollArea->viewport()->height();
    const int calculatedHeight = rows > 0 ? (rows * rowHeight + (rows - 1) * spacing + margins * 2) : viewportHeight;
    const int contentHeight = qMax(viewportHeight, calculatedHeight);

    ui->gridLayoutWidget->setFixedSize(contentWidth, contentHeight);
}

void SettingsForm::callGlobal()
{
    showElements(5, elementsGlobal);
    pressGrid();
}

void SettingsForm::callLightSweep()
{
    showElements(5, elementsLightSweep);
    pressGrid();
}

void SettingsForm::callMediumSweep()
{
    showElements(5, elementsMediumSweep);
    pressGrid();
}

void SettingsForm::callHeavySweep()
{
    showElements(5, elementsHeavySweep);
    pressGrid();
}

void SettingsForm::callFrontTimings()
{
    showElements(5, elementsFrontTimings);
    pressGrid();
}

void SettingsForm::callMiddleTimings()
{
    showElements(5, elementsMiddleTimings);
    pressGrid();
}

void SettingsForm::callBackTimings()
{
    showElements(5, elementsBackTimings);
    pressGrid();
}

void SettingsForm::on_pushButton_save_clicked()
{
    // сохраняем настройки
    // перед этим удаляем lock файл - были случаи что lock файл блокировал запись настроек
    if (QFile::exists(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock"))
        QFile::remove(QCoreApplication::applicationDirPath() + "/settingsAutoCleaner.ini.lock");

    qDebug() << "SAVE SETTINGS";
    for (int i = 0; i < elementsGlobal.count(); i++)
        settings->setValue(elementsGlobal.at(i)->settingsGroup + "/" + elementsGlobal.at(i)->settingsName, elementsGlobal.at(i)->value);
    for (int i = 0; i < elementsLightSweep.count(); i++)
        settings->setValue(elementsLightSweep.at(i)->settingsGroup + "/" + elementsLightSweep.at(i)->settingsName, elementsLightSweep.at(i)->value);
    for (int i = 0; i < elementsMediumSweep.count(); i++)
        settings->setValue(elementsMediumSweep.at(i)->settingsGroup + "/" + elementsMediumSweep.at(i)->settingsName, elementsMediumSweep.at(i)->value);
    for (int i = 0; i < elementsHeavySweep.count(); i++)
        settings->setValue(elementsHeavySweep.at(i)->settingsGroup + "/" + elementsHeavySweep.at(i)->settingsName, elementsHeavySweep.at(i)->value);
    for (int i = 0; i < elementsFrontTimings.count(); i++)
        settings->setValue(elementsFrontTimings.at(i)->settingsGroup + "/" + elementsFrontTimings.at(i)->settingsName, elementsFrontTimings.at(i)->value);
    for (int i = 0; i < elementsMiddleTimings.count(); i++)
        settings->setValue(elementsMiddleTimings.at(i)->settingsGroup + "/" + elementsMiddleTimings.at(i)->settingsName, elementsMiddleTimings.at(i)->value);
    for (int i = 0; i < elementsBackTimings.count(); i++)
        settings->setValue(elementsBackTimings.at(i)->settingsGroup + "/" + elementsBackTimings.at(i)->settingsName, elementsBackTimings.at(i)->value);

    settings->sync();
    system("sync");
    // надо удалить все файлы настроек вида settingsAutoCleaner.ini.Zht231
    ((MainWindow*)parent)->removeBadSettings();

    emit closedAndSave();
}
