greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/include

win32:{
    LIBS += -L$$PWD/lib/win -lxlnt -llibpqxx -llibpq -lws2_32 -lwsock32
}
unix:!macx:{
    DEFINES +=unix
    LIBS += -L"$$PWD/lib/" -lxlnt -lpqxx -lpq
}

#QMAKE_LFLAGS += -Wl,-rpath,"'$$ORIGIN'"

SOURCES += \
    3rd/pugixml.cpp \
    doc_io.cpp \
    main.cpp \
    sql_io.cpp \
    ui/ui_aboutDialog.cpp \
    ui/ui_chooseDevsDialog.cpp \
    ui/ui_connectionDialog.cpp \
    ui/ui_mainWindow.cpp \
    xml_io.cpp

HEADERS += \
    doc_io.h \
    sql_io.h \
    ui/ui_aboutDialog.h \
    ui/ui_chooseDevsDialog.h \
    ui/ui_connectionDialog.h \  
    ui/ui_mainWindow.h \
    xml_io.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES +=
