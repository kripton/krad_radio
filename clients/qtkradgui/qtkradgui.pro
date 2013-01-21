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
        mainwindow.cpp \
    kradstation.cpp \
    broadcastthread.cpp

HEADERS  += mainwindow.h \
    kradstation.h \
    broadcastthread.h



unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_mixer"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_sfx"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_ebml"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_radio"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_transmitter"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_system"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_ring"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_container"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_transponder"
unix:INCLUDEPATH += "/usr/include/opus"
unix:INCLUDEPATH += "/home/dsheeler/Development/krad_radio/lib/krad_compositor"
