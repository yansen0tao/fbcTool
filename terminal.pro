QT += widgets serialport

TARGET = fbcUpg
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    fbcupg.cpp \
    passwordialog.cpp \
    qpasswordlineedit.cpp \
    upgradesetting.cpp \
    upgradeinfo.cpp \
    assistantobject.cpp \
    helpdialog.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    fbcupg.h \
    passwordialog.h \
    qpasswordlineedit.h \
    upgradesetting.h \
    upgradeinfo.h \
    assistantobject.h \
    helpdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui \
    passwordialog.ui \
    upgradesetting.ui \
    helpdialog.ui

RESOURCES += \
    terminal.qrc

RC_FILE = appIcon.rc
