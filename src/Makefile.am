####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3b
k3b_SOURCES = k3bfiletreeview.cpp kstringlistdialog.cpp k3bcddbmultientriesdialog.cpp k3baudioplayer.cpp k3bprojecttabbar.cpp k3bprojecttabwidget.cpp k3bwriterselectionwidget.cpp k3bsplash.cpp k3bblankingjob.cpp k3bblankingdialog.cpp k3bcddb.cpp cddb.cpp k3bfileview.cpp k3bglobals.cpp kcutlabel.cpp k3bprojectburndialog.cpp k3bjob.cpp k3bburnprogressdialog.cpp k3bfillstatusdisplay.cpp k3bcopywidget.cpp k3bdirview.cpp k3bview.cpp k3bdoc.cpp k3b.cpp main.cpp
k3b_LDADD   = ./data/libdata.a ./cdinfo/libcdinfo.a ./audio/libaudio.a ./device/libdevice.a ./audio/input/libinput.a ./option/liboption.a ./rip/librip.a ./libwm/liblibwm.a ./rip/songdb/libsongdb.a ./tools/libtools.a ./cdcopy/libcdcopy.a $(ID3LIB_LIBS) $(CDPARANOIA_LIBS) -lmad -lartskde $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

SUBDIRS = data cdinfo audio device option rip libwm tools cdcopy k3bsetup

EXTRA_DIST = main.cpp k3b.cpp k3b.h k3bdoc.cpp k3bdoc.h k3bview.cpp k3bview.h k3bui.rc k3b.desktop lo32-app-k3b.png lo16-app-k3b.png k3bdirview.cpp k3bdirview.h k3bcopywidget.cpp k3bcopywidget.h k3bglobals.h k3bfillstatusdisplay.cpp k3bfillstatusdisplay.h k3bburnprogressdialog.cpp k3bburnprogressdialog.h k3bjob.cpp k3bjob.h k3bprojectburndialog.cpp k3bprojectburndialog.h kcutlabel.cpp kcutlabel.h k3bglobals.cpp hi16-app-k3b.png hi32-app-k3b.png hi48-app-k3b.png kdiskfreesp.h k3bfileview.cpp k3bfileview.h cddb.h cddb.cpp k3bcddb.cpp k3bcddb.h k3bblankingdialog.cpp k3bblankingdialog.h k3bblankingjob.cpp k3bblankingjob.h k3bsplash.h k3bsplash.cpp k3b-splash-naive.png hi16-tools-cdinfo.png hi22-tools-cdinfo.png hi16-tools-cdrwblank.png hi22-tools-cdrwblank.png k3bwriterselectionwidget.cpp k3bwriterselectionwidget.h k3bprojecttabwidget.cpp k3bprojecttabwidget.h k3bprojecttabbar.cpp k3bprojecttabbar.h k3baudioplayer.cpp k3baudioplayer.h k3bcddbmultientriesdialog.cpp k3bcddbmultientriesdialog.h kstringlistdialog.cpp kstringlistdialog.h klistviewlineedit.h k3bfiletreeview.cpp k3bfiletreeview.h

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/k3b.desktop $(kde_appsdir)/Applications/k3b.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-k3b.png $(kde_icondir)/locolor/32x32/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-k3b.png $(kde_icondir)/locolor/16x16/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/hi16-app-k3b.png $(kde_icondir)/hicolor/16x16/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/hi32-app-k3b.png $(kde_icondir)/hicolor/32x32/apps/k3b.png
	$(mkinstalldirs) $(kde_icondir)/hicolor/48x48/apps/
	$(INSTALL_DATA) $(srcdir)/hi48-app-k3b.png $(kde_icondir)/hicolor/48x48/apps/k3b.png
	$(mkinstalldirs) $(kde_datadir)/k3b/pics/
	$(INSTALL_DATA) $(srcdir)/k3b-splash-naive.png $(kde_datadir)/k3b/pics/k3bsplash.png
	$(mkinstalldirs) $(kde_datadir)/k3b/icons/hicolor/16x16/actions/
	$(INSTALL_DATA) $(srcdir)/hi16-tools-cdinfo.png $(kde_datadir)/k3b/icons/hicolor/16x16/actions/cdinfo.png
	$(mkinstalldirs) $(kde_datadir)/k3b/icons/hicolor/22x22/actions/
	$(INSTALL_DATA) $(srcdir)/hi22-tools-cdinfo.png $(kde_datadir)/k3b/icons/hicolor/22x22/actions/cdinfo.png
	$(mkinstalldirs) $(kde_datadir)/k3b/icons/hicolor/16x16/actions/
	$(INSTALL_DATA) $(srcdir)/hi16-tools-cdrwblank.png $(kde_datadir)/k3b/icons/hicolor/16x16/actions/cdrwblank.png
	$(mkinstalldirs) $(kde_datadir)/k3b/icons/hicolor/22x22/actions/
	$(INSTALL_DATA) $(srcdir)/hi22-tools-cdrwblank.png $(kde_datadir)/k3b/icons/hicolor/22x22/actions/cdrwblank.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/k3b.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/k3b.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/k3b.png
	-rm -f $(kde_icondir)/hicolor/16x16/apps/k3b.png
	-rm -f $(kde_icondir)/hicolor/32x32/apps/k3b.png
	-rm -f $(kde_icondir)/hicolor/48x48/apps/k3b.png
	-rm -f $(kde_datadir)/k3b/pics/k3bsplash.png
	-rm -f $(kde_datadir)/k3b/icons/hicolor/16x16/actions/cdinfo.png
	-rm -f $(kde_datadir)/k3b/icons/hicolor/22x22/actions/cdinfo.png
	-rm -f $(kde_datadir)/k3b/icons/hicolor/16x16/actions/cdrwblank.png
	-rm -f $(kde_datadir)/k3b/icons/hicolor/22x22/actions/cdrwblank.png

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
INCLUDES= $(all_includes) $(CDPARANOIA_INCS) $(ID3LIB_INCS)

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

