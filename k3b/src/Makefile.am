bin_PROGRAMS = k3b

k3b_SOURCES = k3bwelcomewidget.cpp k3bapplication.cpp k3bdiroperator.cpp \
	k3bfiletreeview.cpp \
	k3bprojecttabwidget.cpp k3bsplash.cpp \
	k3bfileview.cpp k3bdirview.cpp k3b.cpp main.cpp \
	k3bstatusbarmanager.cpp k3bfiletreecombobox.cpp \
	k3binterface.cpp k3binterface.skel \
	k3bjobinterface.cpp k3bjobinterface.skel \
	k3bprojectinterface.cpp k3bprojectinterface.skel \
	k3bdataprojectinterface.cpp k3bdataprojectinterface.skel \
	k3bsystemproblemdialog.cpp k3bmediacontentsview.cpp \
	k3bsidepanel.cpp k3bcontentsview.cpp \
	k3bjobprogressdialog.cpp k3bburnprogressdialog.cpp k3btempdirselectionwidget.cpp \
	k3bdatamodewidget.cpp k3bwritingmodewidget.cpp k3bwriterselectionwidget.cpp \
	k3binteractiondialog.cpp k3bthememanager.cpp \
	k3bprojectmanager.cpp k3btrm.cpp k3bmusicbrainz.cpp \
	k3baudioprojectinterface.cpp k3bmixedprojectinterface.cpp \
	k3baudioprojectinterface.skel k3bmixedprojectinterface.skel k3bflatbutton.cpp \
	k3bemptydiscwaiter.cpp k3bjobprogressosd.cpp \
	k3bdebuggingoutputdialog.cpp k3bdebuggingoutputfile.cpp k3bappdevicemanager.cpp \
	k3bmediacache.cpp k3bmedium.cpp k3bmediaselectioncombobox.cpp k3btooltip.cpp \
	k3bwidgetshoweffect.cpp k3bmediaselectiondialog.cpp k3bdiskinfoview.cpp \
	k3bpassivepopup.cpp k3btimeoutwidget.cpp k3bminibutton.cpp \
	k3bthemedheader.cpp k3bthemedlabel.cpp k3blsofwrapper.cpp \
	k3blsofwrapperdialog.cpp k3bservicemenuinstaller.cpp k3bfirstrun.cpp

k3b_LDADD = ./option/liboption.la ./rip/librip.la \
	./projects/libprojects.la ../libk3bdevice/libk3bdevice.la \
	../libk3b/libk3b.la ./misc/libmisc.la ./fastscale/libfastscale.la \
	-lkio -lkparts $(CAM_LIB) $(MUSICBRAINZ_LIBS)

SUBDIRS = option misc rip projects fastscale pics icons konqi mimetypes sounds

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
		-I$(srcdir)/../libk3b/videodvd \
		-I$(srcdir)/../libk3b/jobs \
		-I$(srcdir)/../libk3bdevice \
		-I$(srcdir)/../libk3b/projects \
		-I$(srcdir)/../libk3b/projects/datacd \
		-I$(srcdir)/../libk3b/projects/datadvd \
		-I$(srcdir)/../libk3b/projects/audiocd \
		-I$(srcdir)/../libk3b/projects/videocd \
		-I$(srcdir)/../libk3b/projects/mixedcd \
		-I$(srcdir)/../libk3b/projects/movixcd \
		-I$(srcdir)/../libk3b/projects/videodvd \
		-I$(srcdir)/../libk3b/projects/movixdvd \
		-I$(srcdir)/../libk3b/plugin \
		-I$(srcdir)/projects \
		$(all_includes)

METASOURCES = AUTO

# the library search path.
k3b_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/k3b
rc_DATA = k3bui.rc eventsrc tips

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.rc -o -name \*.ui` `find ../plugins -name \*.rc -o -name \*.ui`> rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) `find . -name \*.cpp -o -name \*.h` `find ../plugins -name \*.cpp -o -name \*.h` -o $(podir)/k3b.pot
	rm -f tips.cpp 
