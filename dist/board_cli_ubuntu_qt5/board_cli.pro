QT += core network

CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = board_cli

SOURCES += \
    main.cpp \
    fileutils.cpp \
    sender.cpp \
    receiver.cpp

HEADERS += \
    protocol.h \
    fileutils.h \
    sender.h \
    receiver.h
