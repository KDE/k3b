
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(kio_videodvd_PART_SRCS videodvd.cpp )

kde4_add_plugin(kio_videodvd ${kio_videodvd_PART_SRCS})

target_link_libraries(kio_videodvd  ${KDE4_KDECORE_LIBS} kio k3b )

install(TARGETS kio_videodvd DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  videodvd.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES  videodvd.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services )



#original Makefile.am contents follow:

#INCLUDES    = 	-I$(srcdir)/../../libk3b/tools \
#		-I$(srcdir)/../../libk3b/core \
#		-I$(srcdir)/../../libk3bdevice \
#		$(all_includes)
#
#kde_module_LTLIBRARIES = kio_videodvd.la
#
#kio_videodvd_la_SOURCES = videodvd.cpp
#kio_videodvd_la_LIBADD = -lkio ../../libk3b/libk3b.la
#kio_videodvd_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)
#
#protocol_DATA = videodvd.protocol
#protocoldir = $(kde_servicesdir)
#
#konq_sidebartree_init_services_data_DATA = videodvd.desktop
#konq_sidebartree_init_services_datadir = $(kde_datadir)/konqsidebartng/virtual_folders/services
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_videodvd.pot
