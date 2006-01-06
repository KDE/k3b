AM_CPPFLAGS = -I$(srcdir)/../../libk3b/core \
	-I$(srcdir)/../../libk3b/core \
	-I$(srcdir)/../../libk3b/plugin \
	-I$(srcdir)/../../libk3b/tools \
	-I$(srcdir)/../../libk3b/projects \
	-I$(srcdir)/../../libk3b/projects/datacd \
	-I$(srcdir)/../../libk3bdevice \
	-I$(srcdir)/../../src/projects \
	$(all_includes)

# these are the headers for your project
noinst_HEADERS   = kfile_k3bprojectfileplugin.h

kde_module_LTLIBRARIES = kfile_k3b.la

kfile_k3b_la_SOURCES = kfile_k3bprojectfileplugin.cpp
kfile_k3b_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
kfile_k3b_la_LIBADD = $(LIB_KIO) ../../libk3b/libk3b.la ../../src/projects/kostore/libkostore.la

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

services_DATA = kfile_k3b.desktop
servicesdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kfile_k3b.pot
