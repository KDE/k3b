


########### next target ###############

set(k3bmaddecoder_PART_SRCS k3bmad.cpp k3bmaddecoder.cpp )

kde4_add_plugin(k3bmaddecoder WITH_PREFIX ${k3bmaddecoder_PART_SRCS})

target_link_libraries(k3bmaddecoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bmaddecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bmaddecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice $(taglib_includes) $(all_includes)
#
#kde_module_LTLIBRARIES = libk3bmaddecoder.la
#
#libk3bmaddecoder_la_SOURCES = k3bmad.cpp k3bmaddecoder.cpp
#
#libk3bmaddecoder_la_LIBADD = $(LIB_KDECORE) $(MAD_LIB) $(taglib_libs) ../../../libk3b/libk3b.la
#libk3bmaddecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bmaddecoder.plugin
#
#METASOURCES = AUTO
