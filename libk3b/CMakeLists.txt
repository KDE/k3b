project(libk3b)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/core
  ${CMAKE_CURRENT_BINARY_DIR}/plugin
  ${CMAKE_CURRENT_BINARY_DIR}/tools
  ${CMAKE_CURRENT_BINARY_DIR}/projects
  ${CMAKE_CURRENT_BINARY_DIR}/jobs
  )

set(CMAKE_REQUIRED_LIBRARIES m)
CHECK_FUNCTION_EXISTS(lrint HAVE_LRINT)
CHECK_FUNCTION_EXISTS(lrintf HAVE_LRINTF)

configure_file(config-libk3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libk3b.h )

add_subdirectory( core )
add_subdirectory( plugin )
add_subdirectory( tools )
add_subdirectory( projects )
add_subdirectory( jobs )

set(k3b_jobs_SRCS
  jobs/k3bdatatrackreader.cpp
  jobs/k3breadcdreader.cpp
  jobs/k3bcdcopyjob.cpp
  jobs/k3bclonejob.cpp
  jobs/k3baudiosessionreadingjob.cpp
  jobs/k3bdvdcopyjob.cpp
  jobs/k3baudiofileanalyzerjob.cpp
  jobs/k3baudiocuefilewritingjob.cpp
  jobs/k3bbinimagewritingjob.cpp
  jobs/k3biso9660imagewritingjob.cpp
  jobs/k3bdvdformattingjob.cpp
  jobs/k3bblankingjob.cpp
  jobs/k3bclonetocreader.cpp
  jobs/k3bverificationjob.cpp
  jobs/k3bdvdbooktypejob.cpp
  jobs/k3bmetawriter.cpp
)

########### next target ###############
if(ENABLE_DVD_RIPPING)
  add_subdirectory(videodvd)
  set(k3b_jobs_SRCS
    ${k3b_jobs_SRCS}
    jobs/k3bvideodvdtitletranscodingjob.cpp
    jobs/k3bvideodvdtitledetectclippingjob.cpp
    )
endif(ENABLE_DVD_RIPPING)

set(k3b_core_SRCS
  core/k3bcore.cpp
  core/k3bglobals.cpp
  core/k3bdefaultexternalprograms.cpp
  core/k3bexternalbinmanager.cpp
  core/k3bversion.cpp
  core/k3bjob.cpp
  core/k3bkjobbridge.cpp
  core/k3bthread.cpp
  core/k3bthreadjob.cpp
  core/k3bglobalsettings.cpp
  core/k3bsimplejobhandler.cpp
  core/k3bthreadjobcommunicationevent.cpp)


set(k3b_tools_SRCS
  tools/k3bwavefilewriter.cpp
  tools/k3bbusywidget.cpp
  tools/k3bdeviceselectiondialog.cpp
  tools/k3bmd5job.cpp
  tools/k3btitlelabel.cpp
  tools/k3bstringutils.cpp
  tools/k3bdevicecombobox.cpp
  tools/k3bstdguiitems.cpp
  tools/k3bvalidators.cpp
  tools/k3bthroughputestimator.cpp
  tools/k3biso9660.cpp
  tools/k3bmultichoicedialog.cpp
  tools/k3bdevicehandler.cpp
  tools/k3bcdparanoialib.cpp
  tools/k3bmsfedit.cpp
  tools/k3bcdtextvalidator.cpp
  tools/k3bintvalidator.cpp
  tools/k3bexceptions.cpp
  tools/k3bprogressdialog.cpp
  tools/k3bpushbutton.cpp
  tools/k3bthreadwidget.cpp
  tools/k3bsignalwaiter.cpp
  tools/k3blibdvdcss.cpp
  tools/k3biso9660backend.cpp
  tools/k3bchecksumpipe.cpp
  tools/k3bintmapcombobox.cpp
  tools/k3bdirsizejob.cpp
  tools/k3bactivepipe.cpp
  tools/k3bfilesplitter.cpp
  tools/k3bfilesysteminfo.cpp
  tools/k3bdevicemodel.cpp
  tools/k3bmedium.cpp
  tools/k3bmediacache.cpp
  tools/k3bcddb.cpp
  tools/k3bprocess.cpp
  tools/qprocess/k3bqprocess.cpp
  tools/qprocess/k3bkprocess.cpp
  )

if(WIN32)
	list(APPEND k3b_tools_SRCS
		tools/qprocess/k3bqprocess_win.cpp
		tools/qprocess/private/qwineventnotifier_p.cpp
		tools/qprocess/private/qfsfileengine_win.cpp
		tools/qprocess/private/qfsfileengine.cpp
		tools/qprocess/private/qfsfileengine_iterator.cpp
		tools/qprocess/private/qfsfileengine_iterator_win.cpp
)
else(WIN32)
  	list(APPEND k3b_tools_SRCS
		tools/qprocess/k3bqprocess_unix.cpp
	)
endif(WIN32)

set(k3b_libisofs_SRCS tools/libisofs/isofs.cpp)

set(k3b_videodvd_SRCS
  videodvd/k3bvideodvd.cpp
  videodvd/k3bvideodvdtime.cpp
  videodvd/k3bvideodvdvideostream.cpp)

set(k3b_plugin_SRCS
  plugin/k3bplugin.cpp
  plugin/k3bpluginconfigwidget.cpp
  plugin/k3bpluginmanager.cpp
  plugin/k3baudiodecoder.cpp
  plugin/k3baudioencoder.cpp
  plugin/k3bprojectplugin.cpp
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
  projects/k3bcuefileparser.cpp )

set(k3b_project_audiocd_SRCS
  projects/audiocd/k3baudiojob.cpp
  projects/audiocd/k3baudiotrack.cpp
  projects/audiocd/k3baudiotrackreader.cpp
  projects/audiocd/k3baudiodoc.cpp
  projects/audiocd/k3baudiodocreader.cpp
  projects/audiocd/k3baudiofile.cpp
  projects/audiocd/k3baudiofilereader.cpp
  projects/audiocd/k3baudiozerodata.cpp
  projects/audiocd/k3baudiozerodatareader.cpp
  projects/audiocd/k3baudiodatasource.cpp
  projects/audiocd/k3brawaudiodatareader.cpp
  projects/audiocd/k3brawaudiodatasource.cpp
  projects/audiocd/k3baudionormalizejob.cpp
  projects/audiocd/k3baudiojobtempdata.cpp
  projects/audiocd/k3baudioimager.cpp
  projects/audiocd/k3baudiomaxspeedjob.cpp
  projects/audiocd/k3baudiocdtrackreader.cpp
  projects/audiocd/k3baudiocdtracksource.cpp
  projects/audiocd/k3baudiocdtrackdrag.cpp
  projects/audiocd/k3baudiodatasourceiterator.cpp
  )

set(k3b_project_datacd
  projects/datacd/k3bdatajob.cpp
  projects/datacd/k3bdatadoc.cpp
  projects/datacd/k3bdataitem.cpp
  projects/datacd/k3bdiritem.cpp
  projects/datacd/k3bfileitem.cpp
  projects/datacd/k3bisoimager.cpp
  projects/datacd/k3bbootitem.cpp
  projects/datacd/k3bisooptions.cpp
  projects/datacd/k3bfilecompilationsizehandler.cpp
  projects/datacd/k3bsessionimportitem.cpp
  projects/datacd/k3bmkisofshandler.cpp
  projects/datacd/k3bdatapreparationjob.cpp
  projects/datacd/k3bmsinfofetcher.cpp
  projects/datacd/k3bdatamultisessionparameterjob.cpp
  )

set(k3b_project_mixedcd
  projects/mixedcd/k3bmixeddoc.cpp
  projects/mixedcd/k3bmixedjob.cpp
  )

set(k3b_project_movixcd
  projects/movixcd/k3bmovixprogram.cpp
  projects/movixcd/k3bmovixdoc.cpp
  projects/movixcd/k3bmovixjob.cpp
  projects/movixcd/k3bmovixfileitem.cpp
  projects/movixcd/k3bmovixdocpreparer.cpp
  )

set(k3b_project_videocd
  projects/videocd/k3bvcddoc.cpp
  projects/videocd/k3bvcdtrack.cpp
  projects/videocd/k3bvcdjob.cpp
  projects/videocd/k3bvcdoptions.cpp
  projects/videocd/k3bvcdxmlview.cpp
  )

set(k3b_project_mpeg_info
  projects/videocd/mpeginfo/k3bmpeginfo.cpp
  )

set(k3b_project_videodvd
  projects/videodvd/k3bvideodvddoc.cpp
  projects/videodvd/k3bvideodvdjob.cpp
  projects/videodvd/k3bvideodvdimager.cpp
  )


set(k3b_LIB_SRCS
  ${k3b_core_SRCS}
  ${k3b_tools_SRCS}
  ${k3b_plugin_SRCS}
  ${k3b_project_SRCS}
  ${k3b_jobs_SRCS}
  ${k3b_libisofs_SRCS}
  ${k3b_project_audiocd_SRCS}
  ${k3b_project_datacd}
  ${k3b_project_mixedcd}
  ${k3b_project_movixcd}
  ${k3b_project_mpeg_info}
  ${k3b_project_videocd}
  ${k3b_project_videodvd}
  )

if (ENABLE_DVD_RIPPING)
  set(k3b_LIB_SRCS ${k3b_LIB_SRCS}
    ${k3b_videodvd_SRCS}
    )
endif (ENABLE_DVD_RIPPING)

kde4_add_library(k3b SHARED ${k3b_LIB_SRCS})

target_link_libraries(k3b
  ${KCDDB_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDE4_KUTILS_LIBS}
  ${KDE4_KPTY_LIBS}
  ${KDE4_SOLID_LIBS}
  ${SAMPLERATE_LIBRARIES}
  k3bdevice
)
if(WIN32)
	target_link_libraries(k3b uuid ole32)
endif(WIN32)

if(ENABLE_DVD_RIPPING)
  target_link_libraries(k3b ${DVDREAD_LIBRARIES})
endif(ENABLE_DVD_RIPPING)

set_target_properties(k3b PROPERTIES OUTPUT_NAME k3blib)

set_target_properties(k3b PROPERTIES VERSION ${K3B_LIB_VERSION_MAJOR}.${K3B_LIB_VERSION_MINOR}.${K3B_LIB_VERSION_RELEASE} SOVERSION ${K3B_LIB_VERSION_MAJOR})
install(TARGETS k3b ${INSTALL_TARGETS_DEFAULT_ARGS} )
