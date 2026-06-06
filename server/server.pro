QT += core gui widgets network

CONFIG += c++11
TEMPLATE = app
TARGET = FileTransferServer

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

include(../shared/common.pri)
