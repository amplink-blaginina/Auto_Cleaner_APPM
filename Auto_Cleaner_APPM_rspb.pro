QT += core gui network quickwidgets quick opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl

CONFIG += c++17 link_pkgconfig

PKGCONFIG += gstreamer-1.0 \
             gstreamer-app-1.0 \
             gstreamer-video-1.0 \
             glib-2.0 \
             poppler-qt5

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/pdf
INCLUDEPATH += $$PWD/gpio
INCLUDEPATH += $$PWD/can
INCLUDEPATH += $$PWD/interface_button
INCLUDEPATH += $$PWD/log
INCLUDEPATH += $$PWD/organs
INCLUDEPATH += $$PWD/service
INCLUDEPATH += $$PWD/settings

LIBS += -lgpiodcxx -lgpiod

# local libgpiod 1.6.3 на машине разработчика (на Pi и при кросс-сборке - системный пакет)
!cross_compile:exists($$(HOME)/libgpiod-1.6/lib) {
    INCLUDEPATH += $$(HOME)/libgpiod-1.6/include
    LIBS += -L$$(HOME)/libgpiod-1.6/lib
    QMAKE_RPATHDIR += $$(HOME)/libgpiod-1.6/lib
}

SOURCES += \
    Controllers/cancontroller.cpp \
    Controllers/gpiocontroller.cpp \
    Controllers/prerollcontroller.cpp \
    Controllers/startercontroller.cpp \
    Controllers/modecontroller.cpp \
    Controllers/organbuttons.cpp \
    Controllers/organpanels.cpp \
    Controllers/viewcontroller.cpp \
    blockform.cpp \
    can/mycanengine.cpp \
    engine.cpp \
    fogotform.cpp \
    gpio/gpio_matrix.cpp \
    gpio/gpio_worker.cpp \
    interface_button/interfacebutton.cpp \
    log/Delegate.cpp \
    log/MessageList.cpp \
    log/screenlog.cpp \
    logger.cpp \
    mainwindow.cpp \
    can/mycan.cpp \
    can/mycanj1939.cpp \
    main.cpp \
    organs/backmagnet.cpp \
    organs/centralbroom.cpp \
    organs/blower.cpp \
    organs/frontrail.cpp \
    organs/organ.cpp \
    organs/organsenums.cpp \
    organs/organsequence.cpp \
    io/caniobus.cpp \
    machine/enginerpmdemand.cpp \
    machine/hydraulicsupply.cpp \
    machine/machineprofile.cpp \
    sim/simiobus.cpp \
    sim/simpanel.cpp \
    password_form.cpp \
    pdf/pdfscroller.cpp \
    service/devices/dkp/servicedevicesdkpleftform.cpp \
    service/devices/hydraulics/servicedeviceshydraulicsleftform.cpp \
    service/global/password/servicegeneralpasswordleftform.cpp \
    service/global/timeConfigure/serviceglobaldatetimeleftform.cpp \
    service/other/engine/serviceotherengineleftform.cpp \
    service/other/light/serviceotherlightleftform.cpp \
    service/safetyinterlock.cpp \
    service/servicemainrightform.cpp \
    settings/currentstate.cpp \
    settings/globalsettings.cpp \
    settings/gpio/superDiag/serviceBUConfigElementform.cpp \
    settings/gpio/superDiag/serviceBUConfigform.cpp \
    settings/gpio/wifi/settingswifileftform.cpp \
    settings/settings/configuration/settingssettingsconfigurationleftform.cpp \
    settings/settingselement.cpp \
    settings/settingsform.cpp \
    settings/settingsmainrightform.cpp \
    settings/settingsreader.cpp \
    settings/settingsstore.cpp

HEADERS += \
    BoolStateWatcher.h \
    Controllers/PhysicalButtonManager.h \
    Controllers/cancontroller.h \
    Controllers/gpiocontroller.h \
    Controllers/prerollcontroller.h \
    Controllers/startercontroller.h \
    Controllers/modecontroller.h \
    Controllers/organbuttons.h \
    Controllers/organpanels.h \
    Controllers/viewcontroller.h \
    DebouncedInput.h \
    MedianFilter.h \
    blockform.h \
    can/mycanengine.h \
    configure.h \
    engine.h \
    fogotform.h \
    gpio/gpio_matrix.hpp \
    gpio/gpio_types.hpp \
    gpio/gpio_worker.hpp \
    interface_button/interfacebutton.h \
    log/Delegate.h \
    log/Delegate_p.h \
    log/MessageList.h \
    log/screenlog.h \
    logger.h \
    mainwindow.h \
    can/mycan.h \
    can/mycanj1939.h \
    organs/backmagnet.h \
    organs/blower.h \
    organs/centralbroom.h \
    organs/frontrail.h \
    organs/organ.h \
    organs/organsenums.h \
    organs/organsequence.h \
    organs/sidetracker.h \
    io/caniobus.h \
    io/iobus.h \
    machine/enginerpmdemand.h \
    machine/hydraulicsupply.h \
    machine/machinecontext.h \
    machine/machineio.h \
    machine/machineprofile.h \
    machine/sweeptype.h \
    sim/simiobus.h \
    sim/simpanel.h \
    password_form.h \
    pdf/pdfscroller.h \
    service/devices/dkp/servicedevicesdkpleftform.h \
    service/devices/hydraulics/servicedeviceshydraulicsleftform.h \
    service/global/password/servicegeneralpasswordleftform.h \
    service/global/timeConfigure/serviceglobaldatetimeleftform.h \
    service/other/engine/serviceotherengineleftform.h \
    service/other/light/serviceotherlightleftform.h \
    service/safetyinterlock.h \
    service/servicemainrightform.h \
    settings/currentstate.h \
    settings/globalsettings.h \
    settings/gpio/superDiag/serviceBUConfigElementform.h \
    settings/gpio/superDiag/serviceBUConfigform.h \
    settings/gpio/wifi/settingswifileftform.h \
    settings/settings/configuration/settingssettingsconfigurationleftform.h \
    settings/settingselement.h \
    settings/settingsform.h \
    settings/settingsmainrightform.h \
    settings/settingsreader.h \
    settings/settingsstore.h

FORMS += \
    blockform.ui \
    fogotform.ui \
    mainwindow.ui \
    password_form.ui \
    service/devices/dkp/servicedevicesdkpleftform.ui \
    service/devices/hydraulics/servicedeviceshydraulicsleftform.ui \
    service/global/password/servicegeneralpasswordleftform.ui \
    service/global/timeConfigure/serviceglobaldatetimeleftform.ui \
    service/other/engine/serviceotherengineleftform.ui \
    service/other/light/serviceotherlightleftform.ui \
    service/servicemainrightform.ui \
    settings/gpio/superDiag/serviceBUConfigElementform.ui \
    settings/gpio/superDiag/serviceBUConfigform.ui \
    settings/gpio/wifi/settingswifileftform.ui \
    settings/settings/configuration/settingssettingsconfigurationleftform.ui \
    settings/settingselement.ui \
    settings/settingsform.ui \
    settings/settingsmainrightform.ui

RESOURCES += \
    images.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# --- Журнал ТО (settings/toJournal) ---
QT += sql
SOURCES += settings/toJournal/serviceTOJournalform.cpp
SOURCES += settings/toJournal/tojournalcalc.cpp
SOURCES += settings/toJournal/tojournalstore.cpp
SOURCES += settings/toJournal/tojournalwidgets.cpp
SOURCES += settings/toJournal/tojournalhooks.cpp
SOURCES += settings/toJournal/tojournalkeyboard.cpp
HEADERS += settings/toJournal/serviceTOJournalform.h
HEADERS += settings/toJournal/tojournalcalc.h
HEADERS += settings/toJournal/tojournalstore.h
HEADERS += settings/toJournal/tojournalwidgets.h
HEADERS += settings/toJournal/tojournalhooks.h
HEADERS += settings/toJournal/tojournalkeyboard.h
HEADERS += settings/toJournal/tojournaldefaults.h
HEADERS += settings/toJournal/tojournaltypes.h
FORMS += settings/toJournal/serviceTOJournalform.ui
# --- конец: Журнал ТО ---

# --- Журнал работы (RPI-RES_260929_66) ---
SOURCES += settings/workJournal/wjcalc.cpp
SOURCES += settings/workJournal/wjstore.cpp
SOURCES += settings/workJournal/wjreport.cpp
SOURCES += settings/workJournal/wjcollector.cpp
SOURCES += settings/workJournal/wjhooks.cpp
SOURCES += settings/workJournal/wjwidgets.cpp
SOURCES += settings/workJournal/wjpages.cpp
SOURCES += settings/workJournal/wjpdf.cpp
SOURCES += settings/workJournal/wjlogo.cpp
SOURCES += settings/workJournal/serviceWorkJournalform.cpp
HEADERS += settings/workJournal/wjtypes.h
HEADERS += settings/workJournal/wjcalc.h
HEADERS += settings/workJournal/wjstore.h
HEADERS += settings/workJournal/wjreport.h
HEADERS += settings/workJournal/wjcollector.h
HEADERS += settings/workJournal/wjhooks.h
HEADERS += settings/workJournal/wjwidgets.h
HEADERS += settings/workJournal/wjpages.h
HEADERS += settings/workJournal/wjpdf.h
HEADERS += settings/workJournal/wjdefaults.h
HEADERS += settings/workJournal/serviceWorkJournalform.h
