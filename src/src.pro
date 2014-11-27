QT      += core gui

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    CONFIG += c++11
}

TARGET   = qcli
TEMPLATE = lib

include(../qcli.pri)

DEFINES += QCLI_LIBRARY

HEADERS += qcli_global.h \
    qcli.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
