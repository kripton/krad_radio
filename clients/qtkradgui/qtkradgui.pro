#-------------------------------------------------
#
# Project created by QtCreator 2013-01-20T09:38:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtkradgui
TEMPLATE = app

LIBS += -L/usr/local/lib/ -lkradradio_client

SOURCES += main.cpp\
    kradstation.cpp \
    broadcastthread.cpp \
    labelledslider.cpp \
    crossfade.cpp \
    stationwidget.cpp \
    mainwindow.cpp \
    resizableframe.cpp \
    eqslider.cpp \
    eq.cpp \
    tapetubewidget.cpp

HEADERS  += \
    kradstation.h \
    broadcastthread.h \
    labelledslider.h \
    crossfade.h \
    stationwidget.h \
    mainwindow.h \
    resizableframe.h \
    eqslider.h \
    eq.h \
    tapetubewidget.h



unix:INCLUDEPATH += "../../lib/krad_mixer"
unix:INCLUDEPATH += "../../lib/krad_sfx"
unix:INCLUDEPATH += "../../lib/krad_ebml"
unix:INCLUDEPATH += "../../lib/krad_radio"
unix:INCLUDEPATH += "../../lib/krad_transmitter"
unix:INCLUDEPATH += "../../lib/krad_system"
unix:INCLUDEPATH += "../../lib/krad_ring"
unix:INCLUDEPATH += "../../lib/krad_container"
unix:INCLUDEPATH += "../../lib/krad_transponder"
unix:INCLUDEPATH += "/usr/include/opus"
unix:INCLUDEPATH += "../../lib/krad_compositor"

FORMS +=
