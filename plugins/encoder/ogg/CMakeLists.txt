


########### next target ###############

set(k3boggvorbisencoder_PART_SRCS k3boggvorbisencoder.cpp )

kde4_add_ui3_files(k3boggvorbisencoder_PART_SRCS base_k3boggvorbisencodersettingswidget.ui )

kde4_add_plugin(k3boggvorbisencoder WITH_PREFIX ${k3boggvorbisencoder_PART_SRCS})

target_link_libraries(k3boggvorbisencoder  ${KDE4_KDECORE_LIBS} k3b ogg vorbis vorbisenc )

install(TARGETS k3boggvorbisencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3boggvorbisencoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3boggvorbisencoder.la
#
#libk3boggvorbisencoder_la_SOURCES = base_k3boggvorbisencodersettingswidget.ui k3boggvorbisencoder.cpp
#
#libk3boggvorbisencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE) -logg -lvorbis -lvorbisenc
#libk3boggvorbisencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3boggvorbisencoder.plugin
#
#METASOURCES = AUTO
