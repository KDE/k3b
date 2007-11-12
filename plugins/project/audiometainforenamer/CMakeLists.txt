
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3baudiometainforenamerplugin_PART_SRCS k3baudiometainforenamerplugin.cpp )

kde4_add_plugin(k3baudiometainforenamerplugin WITH_PREFIX ${k3baudiometainforenamerplugin_PART_SRCS})

target_link_libraries(k3baudiometainforenamerplugin  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3baudiometainforenamerplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3baudiometainforenamerplugin.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core \
#	-I$(srcdir)/../../../libk3b/core \
#	-I$(srcdir)/../../../libk3b/plugin \
#	-I$(srcdir)/../../../libk3b/tools \
#	-I$(srcdir)/../../../libk3b/projects \
#	-I$(srcdir)/../../../libk3b/projects/datacd \
#	-I$(srcdir)/../../../libk3bdevice \
#	$(all_includes)
#
#kde_module_LTLIBRARIES = libk3baudiometainforenamerplugin.la
#
#libk3baudiometainforenamerplugin_la_SOURCES = k3baudiometainforenamerplugin.cpp
#libk3baudiometainforenamerplugin_la_LIBADD = ../../../libk3b/libk3b.la
#libk3baudiometainforenamerplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3baudiometainforenamerplugin.plugin
#
#METASOURCES = AUTO
