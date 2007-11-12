
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3bwavedecoder_PART_SRCS k3bwavedecoder.cpp )

kde4_add_plugin(k3bwavedecoder WITH_PREFIX ${k3bwavedecoder_PART_SRCS})

target_link_libraries(k3bwavedecoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bwavedecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bwavedecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3bwavedecoder.la
#
#libk3bwavedecoder_la_SOURCES = k3bwavedecoder.cpp
#
#libk3bwavedecoder_la_LIBADD = $(LIB_KDECORE) ../../../libk3b/libk3b.la
#libk3bwavedecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bwavedecoder.plugin
#
#METASOURCES = AUTO
