AM_CPPFLAGS = 	-I$(srcdir)/../core \
		-I$(srcdir)/../../libk3bdevice \
		-I$(srcdir)/../../src \
		-I$(srcdir)/../tools \
		-I$(srcdir)/../cddb \
		-I$(srcdir)/../plugin \
		-I$(srcdir)/../projects \
		-I$(srcdir)/../projects/audiocd \
		-I$(srcdir)/../projects/datacd \
		-I$(srcdir)/../kostore \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libimages.la

libimages_la_SOURCES = k3bimagereaderbase.cpp k3biso9660imagereader.cpp \
		k3bnrgisoimagereader.cpp k3bcuefilereader.cpp k3bcdrecordcloneimagereader.cpp \
		k3bvideodvdfolderreader.cpp k3bsimpleimagesource.cpp k3bimagesink.cpp \
		k3bvideodvdfolderimagesource.cpp k3bimagesource.cpp \
		k3baudiocueimagesource.cpp k3bimagesourcestreamer.cpp k3bplainimagesource.cpp \
		k3bimageburner.cpp k3bcdimageburner.cpp k3bimage.cpp k3bimagecreator.cpp \
		k3bsimpletar.cpp

include_HEADERS = k3bimagereaderbase.h k3biso9660imagereader.h \
	k3bnrgisoimagereader.h k3bcuefilereader.h k3bcdrecordcloneimagereader.h \
	k3bvideodvdfolderreader.h k3bimagesource.h k3bimagesink.h \
	k3bimageburner.h k3bcdimageburner.h k3bimage.h \
	k3bimagecreator.h

noinst_HEADERS = k3baudiocueimagesource.h k3bsimpletar.h
