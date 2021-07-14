QT += core
QT -= gui

CONFIG += c++11
QMAKE_CXXFLAGS = -std=c++0x
TARGET = NetServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    threadpool.cpp \
    channel.cpp \
    poller.cpp \
    eventloop.cpp \
    eventloopthread.cpp \
    eventloopthreadpool.cpp \
    timer.cpp \
    timermanager.cpp \
    socket.cpp \
    tcpconnection.cpp \
    tcpserver.cpp \
    httpsession.cpp \
    httpserver.cpp \
    echoserver.cpp \
    log/logger.cpp \
    log/logtest.cpp \
    coroutine/coroutine.cpp \
    coroutine/coroutine_test.cpp

HEADERS += \
    threadpool.h \
    channel.h \
    poller.h \
    eventloop.h \
    eventloopthread.h \
    eventloopthreadpool.h \
    timer.h \
    timermanager.h \
    socket.h \
    tcpconnection.h \
    tcpserver.h \
    httpsession.h \
    httpserver.h \
    echoserver.h \
    log/logger.h \
    coroutine/coroutine.h
