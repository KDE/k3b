bin_PROGRAMS = k3b

k3b_SOURCES = k3bcore.cpp k3bapplication.cpp k3bdiroperator.cpp kdndfileview.cpp k3btempdirselectionwidget.cpp k3bfiletreeview.cpp k3bcddbmultientriesdialog.cpp k3baudioplayer.cpp k3bprojecttabbar.cpp k3bprojecttabwidget.cpp k3bwriterselectionwidget.cpp k3bsplash.cpp k3bblankingjob.cpp k3bblankingdialog.cpp k3bfileview.cpp kcutlabel.cpp k3bprojectburndialog.cpp k3bfillstatusdisplay.cpp k3bdirview.cpp k3btoolbox.cpp k3bisooptions.cpp k3b.cpp k3bprocess.cpp main.cpp k3bdiskerasinginfodialog.cpp k3bemptydiscwaiter.cpp k3bstdguiitems.cpp k3bstatusbarmanager.cpp k3bfiletreecombobox.cpp k3bburnprogressdialog.cpp k3bjob.cpp \
			k3bdoc.cpp \
			k3bview.cpp \
			k3bcdrecordwriter.cpp \
			k3babstractwriter.cpp \
			k3bcdrdaowriter.cpp \
			k3bthread.cpp \
			k3bthreadjob.cpp \
			k3bjobprogressdialog.cpp k3binteractiondialog.cpp \
			k3bjobprogresssystemtray.cpp k3bsystemproblemdialog.cpp \
			k3bdevicecombobox.cpp \
			k3bprojectmanager.cpp \
			k3binterface.cpp k3binterface.skel \
			k3bprojectinterface.cpp k3bprojectinterface.skel

k3b_LDADD   = ./data/libdata.la ./mixed/libmixed.la ./vcd/libvcd.la ./movix/libmovix.la ./cdinfo/libcdinfo.la ./audio/libaudio.la ./device/libdevice.la ./option/liboption.la ./rip/librip.la ./rip/songdb/libsongdb.la ./tools/libtools.la ./tools/kostore/libkostore.la ./cdcopy/libcdcopy.la ./videoEncoding/libvideoEncoding.la ./plugin/libk3bplugin.la $(OGG_VORBISLIBS) -lartskde  -lkio -lkparts $(RESMGR_LIB)

SUBDIRS = tools device cddb data mixed vcd movix cdinfo audio option rip cdcopy videoEncoding pics icons konqi mimetypes k3bsetup plugin

EXTRA_DIST = main.cpp k3b.cpp k3b.h k3bui.rc k3b.desktop k3bdirview.cpp k3bdirview.h k3bfillstatusdisplay.cpp k3bfillstatusdisplay.h k3bprojectburndialog.cpp k3bprojectburndialog.h kcutlabel.cpp kcutlabel.h k3bfileview.cpp k3bfileview.h k3bblankingdialog.cpp k3bblankingdialog.h k3bblankingjob.cpp k3bblankingjob.h k3bsplash.h k3bsplash.cpp k3bwriterselectionwidget.cpp k3bwriterselectionwidget.h k3bprojecttabwidget.cpp k3bprojecttabwidget.h k3bprojecttabbar.cpp k3bprojecttabbar.h k3baudioplayer.cpp k3baudioplayer.h k3bcddbmultientriesdialog.cpp k3bcddbmultientriesdialog.h k3bfiletreeview.cpp k3bfiletreeview.h k3bcdcontentsview.h k3btempdirselectionwidget.cpp k3btempdirselectionwidget.h kdndfileview.cpp kdndfileview.h k3bdiroperator.cpp k3bdiroperator.h k3btoolbox.h k3btoolbox.cpp

apps_DATA = k3b.desktop
appsdir = $(kde_appsdir)/Multimedia


###################
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to.
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# set the include path for X, qt and KDE
AM_CPPFLAGS= -I$(srcdir)/tools -I$(srcdir)/device $(all_includes)

# K3b does not compile filne with --enable-final
KDE_OPTIONS = nofinal

METASOURCES = AUTO

# the library search path.
k3b_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/k3b
rc_DATA = k3bui.rc eventsrc

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` > rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	  if test -n "$$LIST"; then \
		 $(XGETTEXT) $$LIST -o $(podir)/k3b.pot; \
	fi

