AM_CPPFLAGS= -I$(srcdir)/libk3bdevice -I$(srcdir) $(all_includes) $(HAL_INCS) $(DBUS_INCS) $(DBUSQT_INCS)

KDE_CXXFLAGS = $(ENABLE_PERMISSIVE_FLAG)

METASOURCES = AUTO

lib_LTLIBRARIES = libk3bdevice.la

libk3bdevice_la_LIBADD = $(LIB_KIO) $(RESMGR_LIB) $(CAM_LIB) $(HAL_DBUS_LIBS)

# lib version 5 for K3b 1.0
libk3bdevice_la_LDFLAGS = $(all_libraries) -version-info 5:0:0 -no-undefined

if include_HAL
libk3bdevice_la_SOURCES = k3bdevice.cpp k3bdevice_mmc.cpp k3bscsicommand.cpp \
k3btrack.cpp k3btoc.cpp k3bdevicemanager.cpp k3bmsf.cpp k3bdiskinfo.cpp \
k3bdeviceglobals.cpp k3bcrc.cpp k3bcdtext.cpp k3bhalconnection.cpp \
k3bdebug.cpp

include_HEADERS = k3bdevicemanager.h k3bdevice.h k3btoc.h k3btrack.h \
k3bdeviceglobals.h k3bdiskinfo.h k3bcdtext.h k3bmsf.h k3bdevicetypes.h \
k3bdevice_export.h k3bhalconnection.h k3bdebug.h
else
libk3bdevice_la_SOURCES = k3bdevice.cpp k3bdevice_mmc.cpp k3bscsicommand.cpp \
k3btrack.cpp k3btoc.cpp k3bdevicemanager.cpp k3bmsf.cpp k3bdiskinfo.cpp \
k3bdeviceglobals.cpp k3bcrc.cpp k3bcdtext.cpp k3bdebug.cpp

include_HEADERS = k3bdevicemanager.h k3bdevice.h k3btoc.h k3btrack.h \
k3bdeviceglobals.h k3bdiskinfo.h k3bcdtext.h k3bmsf.h k3bdevicetypes.h \
k3bdevice_export.h k3bdebug.h
endif

messages: rc.cpp
	$(XGETTEXT) `find -name "*.cpp" -or -name "*.h"` -o $(podir)/libk3bdevice.pot
