AM_CPPFLAGS= -I$(srcdir)/../../src \
	-I$(srcdir)/../core \
	-I$(srcdir)/../../libk3bdevice \
	-I$(srcdir)/.. \
	$(all_includes)

noinst_LTLIBRARIES = libk3btools.la

libk3btools_la_LIBADD = libisofs/libisofs.la

libk3btools_la_LDFLAGS = $(all_libraries)

libk3btools_la_SOURCES = k3bwavefilewriter.cpp k3bbusywidget.cpp k3bdeviceselectiondialog.cpp \
	k3bmd5job.cpp k3btitlelabel.cpp k3bcutcombobox.cpp \
	k3bstringutils.cpp k3bdevicecombobox.cpp kcutlabel.cpp \
	k3bstdguiitems.cpp k3bvalidators.cpp k3bthroughputestimator.cpp \
	k3biso9660.cpp k3bmultichoicedialog.cpp k3bdevicehandler.cpp \
	k3bcdparanoialib.cpp k3blistview.cpp k3bmsfedit.cpp \
	k3bcdtextvalidator.cpp k3bintvalidator.cpp k3bexceptions.cpp \
	k3bprogressdialog.cpp k3btoolbox.cpp k3bpushbutton.cpp \
	k3blistviewitemanimator.cpp k3bthreadwidget.cpp k3bradioaction.cpp \
	k3bsignalwaiter.cpp k3blibdvdcss.cpp k3biso9660backend.cpp \
	k3binterferingsystemshandler.cpp k3blsofwrapper.cpp k3bpipe.cpp \
	k3bchecksumpipe.cpp

include_HEADERS = k3bwavefilewriter.h k3bbusywidget.h k3bdeviceselectiondialog.h \
	k3bmd5job.h k3bcutcombobox.h k3bstringutils.h \
	k3bdevicecombobox.h kcutlabel.h k3bstdguiitems.h \
	k3bvalidators.h k3bthroughputestimator.h k3biso9660.h \
	k3bmultichoicedialog.h k3bdevicehandler.h k3bcdparanoialib.h \
	k3blistview.h k3bmsfedit.h k3bcdtextvalidator.h \
	k3bintvalidator.h k3bexceptions.h k3bprogressdialog.h \
	k3btoolbox.h k3bpushbutton.h k3blistviewitemanimator.h \
	k3bthreadwidget.h k3bradioaction.h k3bsignalwaiter.h \
	k3biso9660backend.h k3binterferingsystemshandler.h k3bpipe.h \
	k3bchecksumpipe.h

METASOURCES = AUTO

SUBDIRS = libisofs
