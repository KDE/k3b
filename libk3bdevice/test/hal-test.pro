LIBS    = -lhal -ldbus-qt-1 -L/usr/kde/3.4/lib -lkdecore
INCLUDEPATH = /usr/include/dbus-1.0 /usr/kde/3.4/include ../
CONFIG  = qt debug
SOURCES = hal-test.cpp ../k3bhalconnection.cpp
HEADERS = hal-test.h
TARGET = hal-test
QMAKE_EXTRA_UNIX_TARGETS = k3bhalconnection.moc
k3bhalconnection.moc.commands = moc -o k3bhalconnection.moc ../k3bhalconnection.h
