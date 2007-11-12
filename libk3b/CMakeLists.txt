include_directories(
${CMAKE_CURRENT_BINARY_DIR}
${CMAKE_CURRENT_SOURCE_DIR}/core
${CMAKE_CURRENT_SOURCE_DIR}/plugin
${CMAKE_CURRENT_SOURCE_DIR}/tools
${CMAKE_CURRENT_SOURCE_DIR}/projects
${CMAKE_CURRENT_SOURCE_DIR}/cddb
${CMAKE_CURRENT_SOURCE_DIR}/jobs
${CMAKE_CURRENT_BINARY_DIR}/core
${CMAKE_CURRENT_BINARY_DIR}/plugin
${CMAKE_CURRENT_BINARY_DIR}/tools
${CMAKE_CURRENT_BINARY_DIR}/projects
${CMAKE_CURRENT_BINARY_DIR}/cddb
${CMAKE_CURRENT_BINARY_DIR}/jobs
)



add_subdirectory( core )
add_subdirectory( plugin )
add_subdirectory( tools )
add_subdirectory( projects )
add_subdirectory( cddb )
add_subdirectory( jobs )


message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(VIDEODVDDIR)")

########### next target ###############

set(k3b_core_SRCS 
      core/k3bcore.cpp 
      core/k3bglobals.cpp 
      core/k3bdefaultexternalprograms.cpp
      core/k3bexternalbinmanager.cpp 
      core/k3bversion.cpp 
      core/k3bprocess.cpp 
      core/k3bjob.cpp 
      core/k3bthread.cpp 
      core/k3bthreadjob.cpp 
      core/k3bglobalsettings.cpp 
      core/k3bsimplejobhandler.cpp 
      core/k3bthreadjobcommunicationevent.cpp)


set(k3b_cddb_SRCS 
      cddb/k3bcddbquery.cpp 
      cddb/k3bcddb.cpp 
      cddb/k3bcddbresult.cpp 
      cddb/k3bcddbhttpquery.cpp 
      cddb/k3bcddbpquery.cpp 
      cddb/k3bcddblocalquery.cpp 
      cddb/k3bcddbsubmit.cpp 
      cddb/k3bcddblocalsubmit.cpp 
      cddb/k3bcddbmultientriesdialog.cpp)

set(k3b_tools_SRCS
      tools/k3bwavefilewriter.cpp tools/k3bbusywidget.cpp tools/k3bdeviceselectiondialog.cpp 
      tools/k3bmd5job.cpp tools/k3btitlelabel.cpp tools/k3bcutcombobox.cpp 
      tools/k3bstringutils.cpp tools/k3bdevicecombobox.cpp tools/kcutlabel.cpp 
      tools/k3bstdguiitems.cpp tools/k3bvalidators.cpp tools/k3bthroughputestimator.cpp 
      tools/k3biso9660.cpp tools/k3bmultichoicedialog.cpp tools/k3bdevicehandler.cpp 
      tools/k3bcdparanoialib.cpp tools/k3blistview.cpp tools/k3bmsfedit.cpp 
      tools/k3bcdtextvalidator.cpp tools/k3bintvalidator.cpp tools/k3bexceptions.cpp 
      tools/k3bprogressdialog.cpp tools/k3btoolbox.cpp tools/k3bpushbutton.cpp 
      tools/k3blistviewitemanimator.cpp tools/k3bthreadwidget.cpp tools/k3bradioaction.cpp 
      tools/k3bsignalwaiter.cpp tools/k3blibdvdcss.cpp tools/k3biso9660backend.cpp 
      tools/k3bpipe.cpp tools/k3bchecksumpipe.cpp tools/k3btoolbutton.cpp 
      tools/k3bintmapcombobox.cpp tools/k3bdirsizejob.cpp tools/k3brichtextlabel.cpp 
      tools/k3btempfile.cpp tools/k3bactivepipe.cpp tools/k3bfilesplitter.cpp 
      tools/k3bfilesysteminfo.cpp tools/k3bwidgetfactoryaction.cpp
)

set(k3b_plugin_SRCS
    plugin/k3bplugin.cpp 
    plugin/k3bpluginconfigwidget.cpp 
    plugin/k3bpluginmanager.cpp 
    plugin/k3baudiodecoder.cpp 
    plugin/k3baudioencoder.cpp 
    plugin/k3baudioclient.cpp 
    plugin/k3baudioserver.cpp
)

set(k3b_project_SRCS
    projects/k3babstractwriter.cpp 
    projects/k3bgrowisofswriter.cpp 
    projects/k3bgrowisofshandler.cpp 
    projects/k3bdoc.cpp 
    projects/k3bcdrdaowriter.cpp 
    projects/k3bcdrecordwriter.cpp 
    projects/k3binffilewriter.cpp 
    projects/k3btocfilewriter.cpp 
    projects/k3bimagefilereader.cpp 
    projects/k3bcuefileparser.cpp 
    projects/k3bpipebuffer.cpp)

set(k3b_LIB_SRCS ${k3b_core_SRCS} ${k3b_cddb_SRCS} ${k3b_tools_SRCS} ${k3b_plugin_SRCS} ${k3b_project_SRCS})

kde4_add_library(k3b SHARED ${k3b_LIB_SRCS})

target_link_libraries(k3b  ${KDE4_KDECORE_LIBS} jobs k3bdevice )

set_target_properties(k3b PROPERTIES VERSION 3.0.0 SOVERSION 3 )
install(TARGETS k3b DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############




#original Makefile.am contents follow:

#if include_videodvdrip
#VIDEODVDDIR = videodvd
#VIDEODVDLIB = videodvd/libvideodvd.la
#endif
#
#
#lib_LTLIBRARIES = libk3b.la
#
#libk3b_la_SOURCES = dummy.cpp
#
#libk3b_la_LIBADD = core/libk3bcore.la \
#	cddb/libcddb.la \
#	projects/libk3bproject.la \
#	plugin/libk3bplugin.la \
#	tools/libk3btools.la \
#	jobs/libjobs.la \
#	$(VIDEODVDLIB) \
#	../libk3bdevice/libk3bdevice.la
#
#libk3b_la_LDFLAGS = $(all_libraries) -version-info 3:0:0 -no-undefined
#
#SUBDIRS = core plugin tools projects cddb jobs $(VIDEODVDDIR)
#
##pkgconfigdir = $(libdir)/pkgconfig
##pkgconfig_DATA = libk3b.pc
#
#messages:
#	$(XGETTEXT) `find -name "*.cpp" -o -name "*.h"` -o $(podir)/libk3b.pot
