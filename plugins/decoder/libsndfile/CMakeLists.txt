


########### next target ###############

set(k3blibsndfiledecoder_PART_SRCS k3blibsndfiledecoder.cpp )

kde4_add_plugin(k3blibsndfiledecoder WITH_PREFIX ${k3blibsndfiledecoder_PART_SRCS})

target_link_libraries(k3blibsndfiledecoder  ${KDE4_KDECORE_LIBS} k3b ${KDE4_KDEUI_LIBS} sndfile )

install(TARGETS k3blibsndfiledecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3blibsndfiledecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3blibsndfiledecoder.la
#
#libk3blibsndfiledecoder_la_SOURCES = k3blibsndfiledecoder.cpp
#
#libk3blibsndfiledecoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDEUI) -lsndfile
#libk3blibsndfiledecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3blibsndfiledecoder.plugin
#
#METASOURCES = AUTO
#
