QT += widgets serialport sql

QTPLUGIN += QSQLMYSQL

TARGET = terminal
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    console.cpp \
    settingsdialog.cpp \
    browsedbdialog.cpp \
    adduserdialog.cpp \
    adddevicedialog.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    browsedbdialog.h \
    adduserdialog.h \
    user.h \
    adddevicedialog.h

FORMS += \
    mainwindow.ui \
    dialog.ui \
    settingsdialog.ui \
    browsedbdialog.ui \
    adduserdialog.ui \
    adddevicedialog.ui

RESOURCES += \
    terminal.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/serialport/terminal
INSTALLS += target
