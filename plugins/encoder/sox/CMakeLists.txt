


########### next target ###############

set(k3bsoxencoder_PART_SRCS k3bsoxencoder.cpp )

kde4_add_ui3_files(k3bsoxencoder_PART_SRCS base_k3bsoxencoderconfigwidget.ui )

kde4_add_plugin(k3bsoxencoder WITH_PREFIX ${k3bsoxencoder_PART_SRCS})

target_link_libraries(k3bsoxencoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bsoxencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bsoxencoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3bsoxencoder.la
#
#libk3bsoxencoder_la_SOURCES = base_k3bsoxencoderconfigwidget.ui k3bsoxencoder.cpp
#
#libk3bsoxencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE)
#libk3bsoxencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bsoxencoder.plugin
#
#METASOURCES = AUTO
