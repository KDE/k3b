####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3b
k3b_SOURCES = k3bfileview.cpp k3bglobals.cpp kcutlabel.cpp k3bprojectburndialog.cpp k3bjob.cpp k3bburnprogressdialog.cpp k3boptiondialog.cpp k3bfillstatusdisplay.cpp k3bstickybutton.cpp k3bprogressdialog.cpp k3bripperwidget.cpp k3bcopywidget.cpp k3bdirview.cpp k3bview.cpp k3bdoc.cpp k3b.cpp main.cpp 
k3b_LDADD   = ./data/libdata.a ./cdinfo/libcdinfo.a ./audio/libaudio.a ./device/libdevice.a ./kiotree/libkiotree.a ./audio/input/libinput.a ./option/liboption.a -lid3 -lcdda_interface -lcdda_paranoia $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

SUBDIRS = data cdinfo audio device kiotree option 

EXTRA_DIST = main.cpp k3b.cpp k3b.h k3bdoc.cpp k3bdoc.h k3bview.cpp k3bview.h k3bui.rc k3b.desktop lo32-app-k3b.png lo16-app-k3b.png k3bdirview.cpp k3bdirview.h k3bcopywidget.cpp k3bcopywidget.h k3bripperwidget.cpp k3bripperwidget.h k3bglobals.h k3bprogressdialog.h k3bprogressdialog.cpp k3bstickybutton.cpp k3bstickybutton.h k3bfillstatusdisplay.cpp k3bfillstatusdisplay.h k3boptiondialog.cpp k3boptiondialog.h k3bburnprogressdialog.cpp k3bburnprogressdialog.h k3bjob.cpp k3bjob.h k3bprojectburndialog.cpp k3bprojectburndialog.h kcutlabel.cpp kcutlabel.h k3bglobals.cpp nonsticky.png sticky.png hi16-app-k3b.png hi32-app-k3b.png hi48-app-k3b.png kdiskfreesp.h k3bfileview.cpp k3bfileview.h 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/k3b.desktop $(kde_appsdir)/Applications/k3b.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-k3b.png $(kde_icondir)/locolor/32x32/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-k3b.png $(kde_icondir)/locolor/16x16/apps/k3b.png
	$(mkinstalldirs) $(kde_datadir)/k3b/pics/
	$(INSTALL_DATA) $(srcdir)/nonsticky.png $(kde_datadir)/k3b/pics/nonsticky.png
	$(mkinstalldirs) $(kde_datadir)/k3b/pics/
	$(INSTALL_DATA) $(srcdir)/sticky.png $(kde_datadir)/k3b/pics/sticky.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/hi16-app-k3b.png $(kde_icondir)/hicolor/16x16/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/hi32-app-k3b.png $(kde_icondir)/hicolor/32x32/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/48x48/apps/
	$(INSTALL_DATA) $(srcdir)/hi48-app-k3b.png $(kde_icondir)/hicolor/48x48/apps/k3b.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/k3b.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/k3b.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/k3b.png
	-rm -f $(kde_datadir)/k3b/pics/nonsticky.png
	-rm -f $(kde_datadir)/k3b/pics/sticky.png
	-rm -f $(kde_icondir)/hicolor/16x16/apps/k3b.png
	-rm -f $(kde_icondir)/hicolor/32x32/apps/k3b.png
	-rm -f $(kde_icondir)/hicolor/48x48/apps/k3b.png

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
INCLUDES= $(all_includes) -Iid3lib/include -Iid3lib/include/id3

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

