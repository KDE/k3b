
include(CheckFunctionExists)
set(CMAKE_REQUIRED_LIBRARIES m)
check_function_exists(lrint HAVE_LRINT)
check_function_exists(lrintf HAVE_LRINTF)

configure_file(config-libk3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-libk3b.h )

add_subdirectory( core )
add_subdirectory( plugin )
add_subdirectory( tools )
add_subdirectory( projects )
add_subdirectory( jobs )

if(ENABLE_DVD_RIPPING)
    add_subdirectory(videodvd)
    set(videodvd_sources
        jobs/k3bvideodvdtitletranscodingjob.cpp
        jobs/k3bvideodvdtitledetectclippingjob.cpp
        videodvd/k3bvideodvd.cpp
        videodvd/k3bvideodvdtime.cpp
        videodvd/k3bvideodvdvideostream.cpp
        projects/videodvd/k3bvideodvddoc.cpp
        projects/videodvd/k3bvideodvdjob.cpp
        projects/videodvd/k3bvideodvdimager.cpp
    )
    set(videodvd_libraries dvdread)
endif()

if(WIN32)
    set(platform_sources
        tools/qprocess/k3bqprocess_win.cpp
        tools/qprocess/private/qwineventnotifier_p.cpp
        tools/qprocess/private/qfsfileengine_win.cpp
        tools/qprocess/private/qfsfileengine.cpp
        tools/qprocess/private/qfsfileengine_iterator.cpp
        tools/qprocess/private/qfsfileengine_iterator_win.cpp
    )
elseif(UNIX)
    set(platform_sources
        tools/qprocess/k3bqprocess_unix.cpp
    )
endif()

add_library(k3blib SHARED
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
    core/k3bthreadjobcommunicationevent.cpp
    tools/k3bwavefilewriter.cpp
    tools/k3bbusywidget.cpp
    tools/k3bdeviceselectiondialog.cpp
    tools/k3bmd5job.cpp
    tools/k3btitlelabel.cpp
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
    plugin/k3bplugin.cpp
    plugin/k3bpluginconfigwidget.cpp
    plugin/k3bpluginmanager.cpp
    plugin/k3baudiodecoder.cpp
    plugin/k3baudioencoder.cpp
    plugin/k3bprojectplugin.cpp
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
    tools/libisofs/isofs.cpp
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
    projects/mixedcd/k3bmixeddoc.cpp
    projects/mixedcd/k3bmixedjob.cpp
    projects/movixcd/k3bmovixprogram.cpp
    projects/movixcd/k3bmovixdoc.cpp
    projects/movixcd/k3bmovixjob.cpp
    projects/movixcd/k3bmovixfileitem.cpp
    projects/movixcd/k3bmovixdocpreparer.cpp
    projects/videocd/mpeginfo/k3bmpeginfo.cpp
    projects/videocd/k3bvcddoc.cpp
    projects/videocd/k3bvcdtrack.cpp
    projects/videocd/k3bvcdjob.cpp
    projects/videocd/k3bvcdoptions.cpp
    projects/videocd/k3bvcdxmlview.cpp
    ${videodvd_sources}
    ${platform_sources}
)

target_include_directories(k3blib
    PUBLIC
        .
        core
        plugin
        tools
        projects
        projects/audiocd
        projects/datacd
        projects/videocd
        projects/mixedcd
        projects/movixcd
        projects/videodvd
        tools/qprocess
        jobs
        videodvd
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/core
        ${CMAKE_CURRENT_BINARY_DIR}/plugin
        ${CMAKE_CURRENT_BINARY_DIR}/tools
        ${CMAKE_CURRENT_BINARY_DIR}/projects
        ${CMAKE_CURRENT_BINARY_DIR}/jobs
)

target_link_libraries(k3blib
    PUBLIC
        KF5::Completion
        KF5::ConfigWidgets
        KF5::CoreAddons
        KF5::KIOCore
        KF5::Service
    PRIVATE
        k3bdevice
        KF5::FileMetaData
        KF5::IconThemes
        KF5::I18n
        KF5::KCMUtils
        KF5::Solid
        Qt5::Network
        Qt5::Widgets
        KF5::Cddb
        samplerate
        ${videodvd_libraries}
)

if(WIN32)
    target_link_libraries(k3blib uuid ole32)
endif(WIN32)

set_target_properties(k3blib PROPERTIES
    VERSION ${K3B_LIB_VERSION_MAJOR}.${K3B_LIB_VERSION_MINOR}.${K3B_LIB_VERSION_RELEASE}
    SOVERSION ${K3B_LIB_VERSION_MAJOR}
)

generate_export_header(k3blib
    EXPORT_MACRO_NAME LIBK3B_EXPORT
    DEPRECATED_MACRO_NAME LIBK3B_EXPORT_DEPRECATED
    EXPORT_FILE_NAME k3b_export.h
)

install(TARGETS k3blib ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/k3b_export.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
