AM_CPPFLAGS = -I$(srcdir)/../src -I$(srcdir)/../libk3b/core -I$(srcdir)/../libk3b/tools -I$(srcdir)/../libk3bdevice $(all_includes)

METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_k3bsetup2.la

kcm_k3bsetup2_la_SOURCES = base_k3bsetup2.ui k3bsetup2.cpp
kcm_k3bsetup2_la_LIBADD = $(LIB_KDEUI) ../libk3b/libk3b.la
kcm_k3bsetup2_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined

k3bsetup2_DATA = k3bsetup2.desktop
k3bsetup2dir = $(kde_appsdir)/Settings/System

bin_SCRIPTS = k3bsetup

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/k3bsetup.pot
