TEMPLATE = app
TARGET = exSCA

QT = core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += \
    main.cpp \
    window.cpp \
    soundchanges.cpp \
    highlighter.cpp \
    affixerdialog.cpp

HEADERS += \
    window.h \
    soundchanges.h \
    highlighter.h \
    affixerdialog.h

RC_ICONS = Icon.ico
