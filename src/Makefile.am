####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3b
k3b_SOURCES = k3bprojectburndialog.cpp k3bjob.cpp k3bburnprogressdialog.cpp k3boptiondialog.cpp k3bfillstatusdisplay.cpp k3bstickybutton.cpp k3bprogressdialog.cpp k3bdevicemanager.cpp k3bripperwidget.cpp k3bcopywidget.cpp k3bdirview.cpp k3bview.cpp k3bdoc.cpp k3b.cpp main.cpp 
k3b_LDADD   = ./data/libdata.a ./cdinfo/libcdinfo.a ./audio/libaudio.a -lkfile -lkdeui -lkdecore $(LIB_QT) $(LIBSOCKET)

SUBDIRS = data cdinfo audio 

EXTRA_DIST = main.cpp k3b.cpp k3b.h k3bdoc.cpp k3bdoc.h k3bview.cpp k3bview.h k3bui.rc k3b.desktop lo32-app-k3b.png lo16-app-k3b.png k3bdirview.cpp k3bdirview.h k3bcopywidget.cpp k3bcopywidget.h k3bripperwidget.cpp k3bripperwidget.h k3bdevicemanager.cpp k3bdevicemanager.h k3bglobals.h k3bprogressdialog.h k3bprogressdialog.cpp k3bstickybutton.cpp k3bstickybutton.h k3bfillstatusdisplay.cpp k3bfillstatusdisplay.h k3boptiondialog.cpp k3boptiondialog.h k3bburnprogressdialog.cpp k3bburnprogressdialog.h k3bjob.cpp k3bjob.h k3bprojectburndialog.cpp k3bprojectburndialog.h 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/k3b.desktop $(kde_appsdir)/Applications/k3b.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-k3b.png $(kde_icondir)/locolor/32x32/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-k3b.png $(kde_icondir)/locolor/16x16/apps/k3b.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/k3b.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/k3b.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/k3b.png

####### kdevelop will overwrite this part!!! (end)############
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
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
k3b_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/k3b
rc_DATA = k3bui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/k3b.pot; \
	fi

