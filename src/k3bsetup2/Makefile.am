kde_module_LTLIBRARIES = libkcm_k3bsetup2.la

AM_CPPFLAGS = -I$(srcdir)/../core -I$(srcdir)/../core/device $(all_includes)

libkcm_k3bsetup2_la_METASOURCES=AUTO

libkcm_k3bsetup2_la_SOURCES = k3bsetup2.cpp 
libkcm_k3bsetup2_la_LIBADD = $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI) -l$(srcdir)/../core/libk3bcore.la
libkcm_k3bsetup2_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined   


messages:
	$(XGETTEXT) $(libkcm_k3bsetup2_la_SOURCES) -o $(podir)/k3bsetup2.pot

settingssound_DATA = k3bsetup2.desktop
settingssounddir = $(kde_appsdir)/Settings/Sound

