AM_CPPFLAGS= -I$(srcdir)/../core -I$(srcdir)/../core/device -I$(srcdir)/.. $(all_includes)

lib_LTLIBRARIES = libk3btools.la

libk3btools_la_LIBADD = libisofs/libisofs.la ../core/libk3bcore.la

# FIXME: improve that versioning stuff (info libtool -> versioning)
libk3btools_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libk3btools_la_SOURCES = k3bwavefilewriter.cpp k3bbusywidget.cpp k3bdeviceselectiondialog.cpp k3bmd5job.cpp k3btitlelabel.cpp k3bcutcombobox.cpp k3bstringutils.cpp k3bdevicecombobox.cpp kcutlabel.cpp k3binteractiondialog.cpp k3bstdguiitems.cpp k3bjobprogressdialog.cpp k3bburnprogressdialog.cpp k3bjobprogresssystemtray.cpp k3bvalidators.cpp k3bthroughputestimator.cpp k3biso9660.cpp k3bthememanager.cpp

include_HEADERS = k3bwavefilewriter.h k3bbusywidget.h k3bdeviceselectiondialog.h k3bmd5job.h k3bcutcombobox.h k3bstringutils.h k3bdevicecombobox.h kcutlabel.h k3binteractiondialog.h k3bstdguiitems.h k3bjobprogressdialog.h k3bburnprogressdialog.h k3bvalidators.h k3bthroughputestimator.h k3biso9660.h

METASOURCES = AUTO

SUBDIRS = libisofs
