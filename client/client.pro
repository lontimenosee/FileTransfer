QT += core gui widgets network

CONFIG += c++11
TEMPLATE = app
TARGET = FileTransferClient

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

include(../shared/common.pri)
