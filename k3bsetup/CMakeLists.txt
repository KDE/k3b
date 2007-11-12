########### next target ###############

set(kcm_k3bsetup2_PART_SRCS k3bsetup2.cpp )

kde4_add_ui3_files(kcm_k3bsetup2_PART_SRCS base_k3bsetup2.ui )

kde4_add_plugin(kcm_k3bsetup2 ${kcm_k3bsetup2_PART_SRCS})

target_link_libraries(kcm_k3bsetup2  ${KDE4_KDEUI_LIBS} k3b )

install(TARGETS kcm_k3bsetup2 DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES  k3bsetup DESTINATION ${BIN_INSTALL_DIR} )
install( FILES  k3bsetup2.desktop DESTINATION ${APPLNK_INSTALL_DIR}/Settings/System )



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/../src -I$(srcdir)/../libk3b/core -I$(srcdir)/../libk3b/tools -I$(srcdir)/../libk3bdevice $(all_includes)
#
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = kcm_k3bsetup2.la
#
#kcm_k3bsetup2_la_SOURCES = base_k3bsetup2.ui k3bsetup2.cpp
#kcm_k3bsetup2_la_LIBADD = $(LIB_KDEUI) ../libk3b/libk3b.la
#kcm_k3bsetup2_la_LDFLAGS = -module -avoid-version $(all_libraries) -no-undefined
#
#k3bsetup2_DATA = k3bsetup2.desktop
#k3bsetup2dir = $(kde_appsdir)/Settings/System
#
#bin_SCRIPTS = k3bsetup
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/k3bsetup.pot
