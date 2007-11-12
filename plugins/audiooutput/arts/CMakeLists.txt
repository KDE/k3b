
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3bartsoutputplugin_PART_SRCS k3bartsoutputplugin.cpp )

kde4_add_plugin(k3bartsoutputplugin WITH_PREFIX ${k3bartsoutputplugin_PART_SRCS})

target_link_libraries(k3bartsoutputplugin  ${KDE4_KDECORE_LIBS} k3b artsc )

install(TARGETS k3bartsoutputplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bartsoutputplugin.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin $(all_includes)
#
#kde_module_LTLIBRARIES = libk3bartsoutputplugin.la
#
#libk3bartsoutputplugin_la_SOURCES = k3bartsoutputplugin.cpp
#
#libk3bartsoutputplugin_la_LIBADD = ../../../libk3b/libk3b.la -lartsc
#libk3bartsoutputplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bartsoutputplugin.plugin
#
#METASOURCES = AUTO
