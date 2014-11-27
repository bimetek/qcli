QT       += testlib

QT       -= gui

TARGET    = qclitest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE  = app

include(../qcli.pri)

INCLUDEPATH += $$PWD/../src

DEFINES += SRCDIR=\\\"$$PWD/../src\\\"

SOURCES += \
    test_main.cpp \
    simpletest.cpp \
    qclitest.cpp

HEADERS += \
    simpletest.h \
    qclitest.h
