if with_k3bsetup1
K3BSETUP1_BUILD = k3bsetup
endif

bin_PROGRAMS = k3b

k3b_SOURCES = k3bwelcomewidget.cpp k3bapplication.cpp k3bdiroperator.cpp kdndfileview.cpp k3bfiletreeview.cpp k3bcddbmultientriesdialog.cpp k3baudioplayer.cpp k3bprojecttabbar.cpp k3bprojecttabwidget.cpp k3bsplash.cpp k3bblankingdialog.cpp k3bfileview.cpp k3bdirview.cpp k3btoolbox.cpp k3b.cpp main.cpp k3bstatusbarmanager.cpp k3bfiletreecombobox.cpp k3breadcdreader.cpp k3binterface.cpp k3binterface.skel k3bprojectinterface.cpp k3bprojectinterface.skel k3bsystemproblemdialog.cpp

k3b_LDADD   = ./cdinfo/libcdinfo.la ./option/liboption.la ./rip/librip.la ./cdcopy/libcdcopy.la ./dvdcopy/libdvdcopy.la ./cdclone/libcdclone.la ./videoEncoding/libvideoEncoding.la ./plugin/libk3bplugin.la $(ARTS_LIBS) -lkio -lkparts ./core/libk3bcore.la ./tools/libk3btools.la ./projects/libk3bproject.la ./images/libimagewriting.la

SUBDIRS = core tools plugin projects cddb cdinfo option rip cdcopy cdclone images dvdcopy videoEncoding pics icons konqi mimetypes tests audiodecoding audioencoding sounds $(K3BSETUP1_BUILD)

# TODO: enable when we go KDE 3.2
#xdg_apps_DATA = k3b.desktop
# remove the following then
appsdir = $(kde_appsdir)/Multimedia
apps_DATA = k3b.desktop

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
AM_CPPFLAGS= -I$(srcdir)/tools -I$(srcdir)/core -I$(srcdir)/core/device -I$(srcdir)/projects -I$(srcdir)/projects/datacd -I$(srcdir)/projects/datadvd -I$(srcdir)/projects/audiocd -I$(srcdir)/projects/videocd -I$(srcdir)/projects/mixedcd -I$(srcdir)/projects/movixcd -I$(srcdir)/device -I$(srcdir)/plugin $(all_includes)

METASOURCES = AUTO

# the library search path.
k3b_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/k3b
rc_DATA = k3bui.rc eventsrc tips

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` > rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	  if test -n "$$LIST"; then \
		 $(XGETTEXT) $$LIST -o $(podir)/k3b.pot; \
	fi

