AM_CPPFLAGS = -I$(srcdir)/../../libk3b/core \
	-I$(srcdir)/../../libk3bdevice \
	-I$(srcdir)/../../libk3b/plugin \
	-I$(srcdir)/../../libk3b/tools \
	-I$(srcdir)/.. \
	$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = liboption.la

liboption_la_SOURCES = base_k3bcddboptiontab.ui base_k3bmiscoptiontab.ui \
	base_k3bpluginoptiontab.ui \
	base_k3bthemeoptiontab.ui k3bmiscoptiontab.cpp \
	k3bexternalbinoptiontab.cpp k3bcddboptiontab.cpp \
	k3bburningoptiontab.cpp k3boptiondialog.cpp \
	k3bdeviceoptiontab.cpp k3bnotifyoptiontab.cpp \
	k3bpluginoptiontab.cpp \
	k3bthemeoptiontab.cpp k3bdevicewidget.cpp \
	k3bexternalbinwidget.cpp
