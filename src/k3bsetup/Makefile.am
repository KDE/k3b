bin_PROGRAMS = k3bsetup
k3bsetup_SOURCES = k3bsetupwizard.cpp k3bsetup.cpp k3bsetuptab.cpp k3bsetupwizardtabs.cpp main.cpp
k3bsetup_LDADD   = -L$(srcdir)/../core -lk3bcore $(LIB_KIO) $(RESMGR_LIB)
k3bsetup_LDFLAGS = $(all_libraries)

EXTRA_DIST = main.cpp k3bsetup.cpp k3bsetup.h lo32-app-k3bsetup.png lo16-app-k3bsetup.png k3bsetupwizard.cpp k3bsetupwizard.h k3bsetuptab.cpp k3bsetupwizardtabs.cpp k3bsetuptab.h k3bsetupwizardtabs.h

KDE_ICON = k3bsetup

apps_DATA = k3bsetup.desktop
appsdir = $(kde_appsdir)/Multimedia

# set the include path for X, qt and KDE
AM_CPPFLAGS = -I$(srcdir)/../core $(all_includes)

METASOURCES = AUTO

