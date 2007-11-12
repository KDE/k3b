
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3bmpcdecoder_PART_SRCS k3bmpcdecoder.cpp k3bmpcwrapper.cpp )

kde4_add_plugin(k3bmpcdecoder WITH_PREFIX ${k3bmpcdecoder_PART_SRCS})

target_link_libraries(k3bmpcdecoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bmpcdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bmpcdecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin \
#	-I$(srcdir)/../../../libk3b/core \
#	-I$(srcdir)/../../../libk3bdevice \
#	$(all_includes)
#
#kde_module_LTLIBRARIES = libk3bmpcdecoder.la
#
#libk3bmpcdecoder_la_SOURCES = k3bmpcdecoder.cpp k3bmpcwrapper.cpp
#
#libk3bmpcdecoder_la_LIBADD = ../../../libk3b/libk3b.la $(MPC_LIBS)
#libk3bmpcdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bmpcdecoder.plugin
#
#METASOURCES = AUTO
