AM_CPPFLAGS= -I$(srcdir) -I../tools $(all_includes)

METASOURCES = AUTO

lib_LTLIBRARIES = libk3bdevice.la

libk3bdevice_la_LIBADD = $(LIB_KIO) $(RESMGR_LIB)

# FIXME: improve that versioning stuff (info libtool -> versioning)
libk3bdevice_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libk3bdevice_la_SOURCES = k3btrack.cpp k3btoc.cpp k3bdevicemanager.cpp k3bdevice.cpp k3bmsf.cpp k3bdiskinfo.cpp k3bdeviceglobals.cpp k3bscsicommand.cpp

include_HEADERS = k3bdevicemanager.h k3bdevice.h k3btoc.h k3btrack.h k3bdeviceglobals.h k3bdiskinfo.h k3bcdtext.h k3bmsf.h
