
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3blameencoder_PART_SRCS k3blameencoder.cpp )

kde4_add_ui3_files(k3blameencoder_PART_SRCS base_k3blameencodersettingswidget.ui base_k3bmanualbitratesettingswidget.ui )

kde4_add_plugin(k3blameencoder WITH_PREFIX ${k3blameencoder_PART_SRCS})

target_link_libraries(k3blameencoder  ${KDE4_KDECORE_LIBS} k3b mp3lame )

install(TARGETS k3blameencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3blameencoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3blameencoder.la
#
#libk3blameencoder_la_SOURCES = base_k3blameencodersettingswidget.ui \
#	base_k3bmanualbitratesettingswidget.ui \
#	k3blameencoder.cpp
#
#libk3blameencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE) -lmp3lame
#libk3blameencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3blameencoder.plugin
#
#METASOURCES = AUTO
