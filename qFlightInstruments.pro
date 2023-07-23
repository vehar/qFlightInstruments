#-------------------------------------------------
#
# Project created by QtCreator 2011-06-14T09:29:57
#
#-------------------------------------------------

QT += core gui widgets


TARGET = qFlightInstruments
TEMPLATE = app

QMAKE_CXXFLAGS += /std:c++17
QMAKE_LFLAGS += /std:c++17


SOURCES += main.cpp \
        TestWin.cpp \
        qFlightInstruments.cpp \


HEADERS  += qFlightInstruments.h \
            TestWin.h

