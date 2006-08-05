if compile_libsamplerate
USED_LIBSAMPLERATE=./libsamplerate/libsamplerate.la
SUBDIRS = libsamplerate
else
USED_LIBSAMPLERATE=$(LIBSAMPLERATE)
endif


AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../core/ -I$(srcdir)/../../src -I$(srcdir)/../../libk3bdevice $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libk3bplugin.la

libk3bplugin_la_LIBADD = $(USED_LIBSAMPLERATE)

libk3bplugin_la_LDFLAGS = $(all_libraries)

libk3bplugin_la_SOURCES = 	k3bplugin.cpp \
				k3bpluginconfigwidget.cpp \
				k3bpluginmanager.cpp \
				k3baudiodecoder.cpp \
				k3baudioencoder.cpp \
				k3baudioclient.cpp \
				k3baudioserver.cpp


include_HEADERS = k3bplugin.h k3bpluginfactory.h k3bpluginmanager.h k3baudiodecoder.h k3baudioencoder.h k3bpluginconfigwidget.h k3baudiooutputplugin.h k3bprojectplugin.h
