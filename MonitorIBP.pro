QT += widgets core serialport gui network svg quickwidgets

CONFIG += c++17
TARGET  = MonitorIBP
VERSION = 1.1.0
# ---- Версия задаётся ТОЛЬКО строкой VERSION выше ----
# qmake записывает её в resources/version.h, а оттуда версию берут
# MonitorIBP.rc (свойства exe) и main.cpp (QApplication::applicationVersion).
VER_PARTS = $$split(VERSION, .)
VERSION_H = \
    "// Создаётся qmake из VERSION в MonitorIBP.pro — не правьте вручную" \
    "$${LITERAL_HASH}define APP_VERSION_STR    \"$${VERSION}\"" \
    "$${LITERAL_HASH}define APP_VERSION_RC     $$member(VER_PARTS, 0),$$member(VER_PARTS, 1),$$member(VER_PARTS, 2),0" \
    "$${LITERAL_HASH}define APP_VERSION_RC_STR \"$${VERSION}.0\\0\""
write_file($$PWD/resources/version.h, VERSION_H)|error("Не удалось записать resources/version.h")
INCLUDEPATH += ui core

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    core/autostart.cpp \
    core/comportavailables.cpp \
    core/dailylogfile.cpp \
    core/measurementlog.cpp \
    core/snmpcardregistry.cpp \
    core/snmpcardv1.cpp \
    core/snmptrapevents.cpp \
    core/snmptraplistener.cpp \
    core/upsmibalarms.cpp \
    core/upsshutdownscenario.cpp \
    ui/comidentwidget.cpp \
    ui/comratingswidget.cpp \
    ui/connectionsettingswindow.cpp \
    core/connecttocomport.cpp \
    ui/logsettingswindow.cpp \
    ui/logwindow.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    core/megatecparser.cpp \
    core/megatecupsmonitor.cpp \
    core/snmpupsmonitor.cpp \
    ui/ptrtstatuswidget.cpp \
    ui/snmpcardtypewindow.cpp \
    ui/snmpidentwidget.cpp \
    ui/snmpratingswidget.cpp \
    ui/thememanager.cpp \
    ui/trapsettingswindow.cpp \
    ui/trapwindow.cpp \
    ui/upscontrolpanel.cpp \
    ui/upsparamswindow.cpp \
    ui/upsschematicmodel.cpp


HEADERS += \
    core/autostart.h \
    core/comportavailables.h \
    core/dailylogfile.h \
    core/logsettings.h \
    core/measurementlog.h \
    core/snmpcarddriver.h \
    core/snmpcardregistry.h \
    core/snmpcardv1.h \
    core/snmpconnectionparams.h \
    core/snmptrapdata.h \
    core/snmptrapevents.h \
    core/snmptraplistener.h \
    core/upscommand.h \
    core/upsmibalarms.h \
    core/upsshutdownscenario.h \
    ui/comidentwidget.h \
    ui/comratingswidget.h \
    ui/connectionsettingswindow.h \
    core/connecttocomport.h \
    ui/logsettingswindow.h \
    ui/logwindow.h \
    ui/mainwindow.h \
    core/megatecparser.h \
    core/megatecupsmonitor.h \
    core/snmpupsmonitor.h \
    core/upsdatastruct.h \
    ui/ptrtstatuswidget.h \
    ui/snmpcardtypewindow.h \
    ui/snmpidentwidget.h \
    ui/snmpratingswidget.h \
    ui/thememanager.h \
    ui/trapsettingswindow.h \
    ui/trapwindow.h \
    ui/upscontrolpanel.h \
    ui/upsidentwidget.h \
    ui/upsparamswindow.h \
    ui/upsratingswidget.h \
    ui/upsschematicmodel.h
FORMS += \
    ui/comidentwidget.ui \
    ui/comratingswidget.ui \
    ui/connectionsettingswindow.ui \
    ui/logsettingswindow.ui \
    ui/logwindow.ui \
    ui/mainwindow.ui \
    ui/ptrtstatuswidget.ui \
    ui/snmpcardtypewindow.ui \
    ui/snmpidentwidget.ui \
    ui/snmpratingswidget.ui \
    ui/trapsettingswindow.ui \
    ui/trapwindow.ui \
    ui/upscontrolpanel.ui \
    ui/upsparamswindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
    INCLUDEPATH += C:/msys64/mingw64/include

    QMAKE_CXXFLAGS += -DNETSNMP_ENABLE_IPV6 -fno-strict-aliasing -DNETSNMP_REMOVE_U64 \
                       -Umingw32 -Dmingw32=mingw32 -D_WIN32_WINNT=0xffff \
                       -DWIN32 -DVC_EXTRALEAN -DWIN32_LEAN_AND_MEAN

    LIBS += C:/msys64/mingw64/lib/libnetsnmp.a \
            C:/msys64/mingw64/lib/libssl.a \
            C:/msys64/mingw64/lib/libcrypto.a \
            C:/msys64/mingw64/lib/libregex.a \
            C:/msys64/mingw64/lib/libtre.a \
            C:/msys64/mingw64/lib/libsnmpapi.a \
            -lws2_32 -liphlpapi -lcrypt32 -lintl\

}

CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

RESOURCES += \
    resources.qrc

win32: RC_FILE = resources/MonitorIBP.rc
macx:  ICON     = resources/icons/app.icns

unix:!macx {
    LIBS += -lnetsnmp
}
