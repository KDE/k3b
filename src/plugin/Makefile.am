if compile_libsamplerate
USED_LIBSAMPLERATE=./libsamplerate/libsamplerate.la
SUBDIRS = libsamplerate
else
USED_LIBSAMPLERATE=$(LIBSAMPLERATE)
endif


AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../core/ -I$(srcdir)/../core/device $(all_includes)

METASOURCES = AUTO

lib_LTLIBRARIES = libk3bplugin.la

libk3bplugin_la_LIBADD = ../core/libk3bcore.la $(USED_LIBSAMPLERATE)

# FIXME: improve that versioning stuff (info libtool -> versioning)
libk3bplugin_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libk3bplugin_la_SOURCES = 	k3bplugin.cpp \
				k3bpluginfactory.cpp \
				k3bpluginconfigwidget.cpp \
				k3bpluginmanager.cpp \
				k3baudiodecoder.cpp \
				k3baudioencoder.cpp

include_HEADERS = k3bplugin.h k3bpluginfactory.h k3bpluginmanager.h k3baudiodecoder.h k3baudioencoder.h
