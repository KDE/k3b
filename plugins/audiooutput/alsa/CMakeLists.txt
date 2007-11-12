
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3balsaoutputplugin_PART_SRCS k3balsaoutputplugin.cpp )

kde4_add_plugin(k3balsaoutputplugin WITH_PREFIX ${k3balsaoutputplugin_PART_SRCS})

target_link_libraries(k3balsaoutputplugin  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3balsaoutputplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3balsaoutputplugin.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin $(all_includes)
#
#kde_module_LTLIBRARIES = libk3balsaoutputplugin.la
#
#libk3balsaoutputplugin_la_SOURCES = k3balsaoutputplugin.cpp
#
#libk3balsaoutputplugin_la_CFLAGS = $(ALSA_CFLAGS)
#libk3balsaoutputplugin_la_LIBADD = ../../../libk3b/libk3b.la $(ALSA_LIBS)
#libk3balsaoutputplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3balsaoutputplugin.plugin
#
#METASOURCES = AUTO
