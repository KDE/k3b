
####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3bsetup
k3bsetup_SOURCES = k3bsetup2.cpp k3bsetup2page.cpp k3bsetup2finishpage.cpp main.cpp
k3bsetup_LDADD   = ./fstab/libfstab.la ../tools/libtools.la ../device/libdevice.la -lcdda_interface $(LIB_KIO)
k3bsetup_LDFLAGS = $(all_libraries)

SUBDIRS = fstab

EXTRA_DIST = main.cpp k3bsetup2.cpp

KDE_ICON = k3bsetup

apps_DATA = k3bsetup.desktop
appsdir = $(kde_appsdir)/Applications

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -I$(srcdir)/.. -I$(srcdir)/../tools

METASOURCES = AUTO

