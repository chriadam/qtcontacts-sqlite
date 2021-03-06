include(../../config.pri)

TEMPLATE = lib
TARGET = qtcontacts_sqlite

QT += sql dbus

CONFIG += plugin hide_symbols
PLUGIN_TYPE=contacts

# we hardcode this for Qt4 as there's no GenericDataLocation offered by QDesktopServices
DEFINES += 'QTCONTACTS_SQLITE_CENTRAL_DATA_DIR=\'\"/home/nemo/.local/share/system/\"\''
DEFINES += 'QTCONTACTS_SQLITE_PRIVILEGED_DIR=\'\"privileged\"\''
DEFINES += 'QTCONTACTS_SQLITE_DATABASE_DIR=\'\"Contacts/qtcontacts-sqlite/\"\''
DEFINES += 'QTCONTACTS_SQLITE_DATABASE_NAME=\'\"contacts.db\"\''
# we build a path like: /home/nemo/.local/share/system/Contacts/qtcontacts-sqlite/contacts.db

# Use the option to sort presence state by availability
DEFINES += SORT_PRESENCE_BY_AVAILABILITY

INCLUDEPATH += \
        ../extensions

HEADERS += \
        memorytable_p.h \
        semaphore_p.h \
        trace_p.h \
        conversion_p.h \
        contactid_p.h \
        contactsdatabase.h \
        contactsengine.h \
        contactstransientstore.h \
        contactnotifier.h \
        contactreader.h \
        contactwriter.h \
        ../extensions/contactmanagerengine.h

SOURCES += \
        memorytable.cpp \
        semaphore_p.cpp \
        conversion.cpp \
        contactid.cpp \
        contactsdatabase.cpp \
        contactsengine.cpp \
        contactstransientstore.cpp \
        contactsplugin.cpp \
        contactnotifier.cpp \
        contactreader.cpp \
        contactwriter.cpp

target.path = $$[QT_INSTALL_PLUGINS]/contacts
INSTALLS += target

PACKAGENAME=qtcontacts-sqlite-qt5-extensions

headers.path = $${PREFIX}/include/$${PACKAGENAME}
headers.files = ../extensions/*
headers.depends = ../extensions/*
INSTALLS += headers

pkgconfig.path = $${PREFIX}/lib/pkgconfig
pkgconfig.files = ../$${PACKAGENAME}.pc
INSTALLS += pkgconfig

OTHER_FILES += plugin.json

