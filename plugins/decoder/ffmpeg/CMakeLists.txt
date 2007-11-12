


########### next target ###############

set(k3bffmpegdecoder_PART_SRCS k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp )

kde4_add_plugin(k3bffmpegdecoder WITH_PREFIX ${k3bffmpegdecoder_PART_SRCS})

target_link_libraries(k3bffmpegdecoder  ${KDE4_KDECORE_LIBS} k3b ${KDE4_KDEUI_LIBS} avcodec avformat )

install(TARGETS k3bffmpegdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bffmpegdecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice $(all_includes)
#KDE_CXXFLAGS = -D__STDC_CONSTANT_MACROS
#
#kde_module_LTLIBRARIES = libk3bffmpegdecoder.la
#
#libk3bffmpegdecoder_la_SOURCES = k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp
#
#libk3bffmpegdecoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDEUI) -lavcodec -lavformat
#libk3bffmpegdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bffmpegdecoder.plugin
#
#METASOURCES = AUTO
#
