
####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = k3bsetup
k3bsetup_SOURCES = k3bsetupwizard.cpp k3bsetup.cpp k3bsetuptab.cpp k3bsetupwizardtabs.cpp main.cpp
k3bsetup_LDADD   = ../src/tools/libtools.la ../src/device/libdevice.la -lcdda_interface $(LIB_KIO)
k3bsetup_LDFLAGS = $(all_libraries)

EXTRA_DIST = main.cpp k3bsetup.cpp k3bsetup.h lo32-app-k3bsetup.png lo16-app-k3bsetup.png k3bsetupwizard.cpp k3bsetupwizard.h k3bsetuptab.cpp k3bsetupwizardtabs.cpp k3bsetuptab.h k3bsetupwizardtabs.h

KDE_ICON = k3bsetup

apps_DATA = k3bsetup.desktop
appsdir = $(kde_appsdir)/Multimedia

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

messages: 
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/k3bsetup.pot; \
	fi

