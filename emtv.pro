QT += widgets
TEMPLATE = app
CONFIG += c++17

TARGET = emtv

SOURCES += \
        main.cpp \
        maincontrolwindow.cpp \
        tvdeflector.cpp \
        tvwindow.cpp

HEADERS += \
        maincontrolwindow.h \
        tvdeflector.h \
        tvwindow.h

RESOURCES += \
    emtv.qrc

FORMS += \
    maincontrolwindow.ui
