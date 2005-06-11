INCLUDES    = 	-I$(srcdir)/../../libk3b/tools \
		-I$(srcdir)/../../libk3b/core \
		-I$(srcdir)/../../libk3bdevice \
		$(all_includes)

kde_module_LTLIBRARIES = kio_videodvd.la

kio_videodvd_la_SOURCES = videodvd.cpp
kio_videodvd_la_LIBADD = -lkio ../../libk3b/libk3b.la
kio_videodvd_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = videodvd.protocol
protocoldir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_videodvd.pot
