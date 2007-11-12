
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(k3bexternalencoder_PART_SRCS k3bexternalencoder.cpp k3bexternalencoderconfigwidget.cpp k3bexternalencodercommand.cpp )

kde4_add_ui3_files(k3bexternalencoder_PART_SRCS base_k3bexternalencoderconfigwidget.ui base_k3bexternalencodereditwidget.ui )

kde4_add_plugin(k3bexternalencoder WITH_PREFIX ${k3bexternalencoder_PART_SRCS})

target_link_libraries(k3bexternalencoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bexternalencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bexternalencoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)
#
#kde_module_LTLIBRARIES = libk3bexternalencoder.la
#
#libk3bexternalencoder_la_SOURCES = base_k3bexternalencoderconfigwidget.ui \
#	base_k3bexternalencodereditwidget.ui k3bexternalencoder.cpp \
#	k3bexternalencoderconfigwidget.cpp k3bexternalencodercommand.cpp
#
#libk3bexternalencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE)
#libk3bexternalencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/k3b/plugins
#plugins_DATA = k3bexternalencoder.plugin
#
#METASOURCES = AUTO
