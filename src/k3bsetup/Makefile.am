AM_CPPFLAGS = -I../core -I../core/device $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = kcm_k3bsetup2.la

kcm_k3bsetup2_la_SOURCES = base_k3bsetup2.ui k3bsetup2.cpp
kcm_k3bsetup2_la_LIBADD = $(LIB_KDEUI) ../core/libk3bcore.la
kcm_k3bsetup2_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined

k3bsetup2_DATA = k3bsetup2.desktop
k3bsetup2dir = $(kde_appsdir)/Settings/System
