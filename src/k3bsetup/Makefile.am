####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3bsetup
k3bsetup_SOURCES = k3bsetupwizard.cpp k3bsetup.cpp k3bsetuptab.cpp k3bsetupwizardtabs.cpp main.cpp
k3bsetup_LDADD   = ../tools/k3bexternalbinmanager.o ../tools/k3bglobals.o ../device/libdevice.a -lcdda_interface $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

EXTRA_DIST = main.cpp k3bsetup.cpp k3bsetup.h lo32-app-k3bsetup.png lo16-app-k3bsetup.png k3bsetupwizard.cpp k3bsetupwizard.h k3bsetuptab.cpp k3bsetupwizardtabs.cpp k3bsetuptab.h k3bsetupwizardtabs.h

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/k3bsetup.desktop $(kde_appsdir)/Applications/k3bsetup.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-k3bsetup.png $(kde_icondir)/locolor/32x32/apps/k3bsetup.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-k3bsetup.png $(kde_icondir)/locolor/16x16/apps/k3bsetup.png
	$(mkinstalldirs) $(kde_datadir)/k3b/pics/
	$(INSTALL_DATA) $(srcdir)/k3bsetup.png $(kde_datadir)/k3b/pics/k3bsetup.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/k3bsetup.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/k3bsetup.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/k3bsetup.png
	-rm -f $(kde_datadir)/k3b/pics/k3bsetup.png

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
k3bsetup_LDFLAGS = $(all_libraries) $(KDE_RPATH)

#WARNING: if you use a ui.rc file above, use:

# messages: rc.cpp

# instead of 

# messages:

messages: 
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/k3bsetup.pot; \
	fi

