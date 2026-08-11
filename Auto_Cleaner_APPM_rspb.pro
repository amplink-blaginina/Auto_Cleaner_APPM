QT += core gui network quickwidgets quick opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl

CONFIG += c++17 link_pkgconfig

PKGCONFIG += gstreamer-1.0 \
             gstreamer-app-1.0 \
             gstreamer-video-1.0 \
             glib-2.0

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/pdf
INCLUDEPATH += $$PWD/gpio
INCLUDEPATH += $$PWD/can
INCLUDEPATH += $$PWD/interface_button
INCLUDEPATH += $$PWD/log
INCLUDEPATH += $$PWD/organs
INCLUDEPATH += $$PWD/service
INCLUDEPATH += $$PWD/settings

# local libgpiod 1.6.3 — put BEFORE /usr/include
INCLUDEPATH += $$(HOME)/libgpiod-1.6/include
LIBS += -L$$(HOME)/libgpiod-1.6/lib -lgpiodcxx -lgpiod

# Poppler Qt5
INCLUDEPATH += /usr/include/poppler/qt5
INCLUDEPATH += /usr/include
LIBS += -lpoppler -lpoppler-qt5

# Optional runtime search path for local libgpiod
QMAKE_RPATHDIR += $$(HOME)/libgpiod-1.6/lib

SOURCES += \
    Controllers/cancontroller.cpp \
    Controllers/gpiocontroller.cpp \
    Controllers/prerollcontroller.cpp \
    Controllers/startercontroller.cpp \
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
    organs/organsenums.cpp \
    password_form.cpp \
    pdf/pdfscroller.cpp \
    service/devices/dkp/servicedevicesdkpleftform.cpp \
    service/devices/hydraulics/servicedeviceshydraulicsleftform.cpp \
    service/global/password/servicegeneralpasswordleftform.cpp \
    service/global/timeConfigure/serviceglobaldatetimeleftform.cpp \
    service/other/intervals/servicegpioserviceintervalleftform.cpp \
    service/other/intervals/servicetoelement.cpp \
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
    Controllers/cancontroller.h \
    Controllers/gpiocontroller.h \
    Controllers/prerollcontroller.h \
    Controllers/startercontroller.h \
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
    organs/organsenums.h \
    password_form.h \
    pdf/pdfscroller.h \
    service/devices/dkp/servicedevicesdkpleftform.h \
    service/devices/hydraulics/servicedeviceshydraulicsleftform.h \
    service/global/password/servicegeneralpasswordleftform.h \
    service/global/timeConfigure/serviceglobaldatetimeleftform.h \
    service/other/intervals/servicegpioserviceintervalleftform.h \
    service/other/intervals/servicetoelement.h \
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
    service/other/intervals/servicegpioserviceintervalleftform.ui \
    service/other/intervals/servicetoelement.ui \
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
