AM_CPPFLAGS= 	-I$(srcdir)/.. \
		-I$(srcdir)/../../core \
		-I$(srcdir)/../../plugin \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../src \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/../.. \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libdata.la

libdata_la_SOURCES =    k3bdatajob.cpp \
			k3bdatadoc.cpp \
			k3bdataitem.cpp \
			k3bdiritem.cpp \
			k3bfileitem.cpp \
			k3bisoimager.cpp \
			k3bmsinfofetcher.cpp \
			k3bbootitem.cpp \
			k3bisooptions.cpp \
			k3bfilecompilationsizehandler.cpp \
			k3bdataverifyingjob.cpp \
			k3bsessionimportitem.cpp \
			k3bmkisofshandler.cpp

include_HEADERS = 	k3bdatadoc.h \
			k3bdatajob.h \
			k3bdataitem.h \
			k3bdiritem.h \
			k3bfileitem.h \
			k3bbootitem.h \
			k3bisooptions.h
