bin_PROGRAMS = k3b

k3b_SOURCES = k3bwelcomewidget.cpp k3bapplication.cpp k3bdiroperator.cpp kdndfileview.cpp k3bfiletreeview.cpp k3bprojecttabbar.cpp k3bprojecttabwidget.cpp k3bsplash.cpp k3bblankingdialog.cpp k3bfileview.cpp k3bdirview.cpp k3b.cpp main.cpp k3bstatusbarmanager.cpp k3bfiletreecombobox.cpp k3breadcdreader.cpp k3binterface.cpp k3binterface.skel k3bprojectinterface.cpp k3bprojectinterface.skel k3bsystemproblemdialog.cpp k3bcdcontentsview.cpp k3bwriterspeedverificationdialog.cpp k3bdatatrackreader.cpp k3bsidepanel.cpp

k3b_LDADD   = ./cdinfo/libcdinfo.la ./option/liboption.la ./rip/librip.la ./cdcopy/libcdcopy.la ./dvdcopy/libdvdcopy.la ./videoEncoding/libvideoEncoding.la ./images/libimagewriting.la ../libk3bdevice/libk3bdevice.la ../libk3b/libk3b.la -lkio -lkparts $(CAM_LIB)

SUBDIRS = cdinfo option rip cdcopy images dvdcopy videoEncoding pics icons konqi mimetypes sounds

xdg_apps_DATA = k3b.desktop

silent_DATA = k3b-iso.desktop k3b-cue.desktop
silentdir = $(kde_appsdir)/.hidden/

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
AM_CPPFLAGS = 	-I$(srcdir)/../libk3b/tools \
		-I$(srcdir)/../libk3b/core \
		-I$(srcdir)/../libk3b/cddb \
		-I$(srcdir)/../libk3bdevice \
		-I$(srcdir)/../libk3b/projects \
		-I$(srcdir)/../libk3b/projects/datacd \
		-I$(srcdir)/../libk3b/projects/datadvd \
		-I$(srcdir)/../libk3b/projects/audiocd \
		-I$(srcdir)/../libk3b/projects/videocd \
		-I$(srcdir)/../libk3b/projects/mixedcd \
		-I$(srcdir)/../libk3b/projects/movixcd \
		-I$(srcdir)/../libk3b/plugin \
		$(all_includes)

METASOURCES = AUTO

# the library search path.
k3b_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/k3b
rc_DATA = k3bui.rc eventsrc tips

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.rc -o -name \*.ui` > rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) `find . -name \*.cpp -o -name \*.h` `find ../plugins -name \*.cpp -o -name \*.h` -o $(podir)/k3b.pot
	rm -f tips.cpp 


### FIXME: Workaround because --enable-final triggers multiple-inclusion
### of kcmdlineargs.h and *one* of those inclusions (the one in
### k3bsmartinstancereuser.cpp) does the silly #define private public
### trick to munge the internals of KCmdLineArgs; unfortunately,
### kcmdlineargs.h has already been included by then and it's protected
### against multiple-inclusion and the trick fails. This suggests that
### there should be _some_ way to save command-line arguments to a
### stream in a public API.
KDE_OPTIONS = nofinal

