QT       += core gui network widgets
CONFIG   += c++17
TARGET    = client
TEMPLATE  = app

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/client.cpp

HEADERS += \
    $$PWD/client.h 

VERSION = 1.0.0 