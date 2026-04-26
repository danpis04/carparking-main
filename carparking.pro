QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/enviroment.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/q_learning.cpp \
    src/utils.cpp

HEADERS += \
    src/enviroment.h \
    src/mainwindow.h \
    src/q_learning.h \
    src/utils.h

FORMS += \
    src/mainwindow.ui

OBJECTS_DIR = build

DESTDIR = bin

TARGET = carparking



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
