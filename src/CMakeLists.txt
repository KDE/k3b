
add_subdirectory( helper )

add_subdirectory( icons )
add_subdirectory( pics )
add_subdirectory( services )


########### next target ###############

ki18n_wrap_ui(ui_sources
    option/base_k3bthemeoptiontab.ui
    option/base_k3bmiscoptiontab.ui
    option/base_k3bpluginoptiontab.ui
    projects/base_k3baudiocdtextwidget.ui
    projects/base_k3bbootimagedialog.ui
    projects/base_k3badvanceddataimagesettings.ui
    projects/base_k3baudiocdtextallfieldsdialog.ui
    projects/base_k3bdataimagesettings.ui
    projects/base_k3bdatavolumedescdialog.ui
    projects/base_k3baudiotrackwidget.ui
    projects/base_k3bmovixoptionswidget.ui
    rip/base_k3baudiorippingoptionwidget.ui
    rip/base_k3bcddbpatternwidget.ui
)

if(ENABLE_AUDIO_PLAYER)
    set(audioplayer_sources projects/k3baudiotrackplayer.cpp)
endif()

if(ENABLE_DVD_RIPPING)
    set(videodvd_sources
        rip/videodvd/k3bvideodvdaudiomodel.cpp
        rip/videodvd/k3bvideodvdrippingview.cpp
        rip/videodvd/k3bvideodvdrippingjob.cpp
        rip/videodvd/k3bvideodvdrippingwidget.cpp
        rip/videodvd/k3bvideodvdrippingdialog.cpp
        rip/videodvd/k3bvideodvdrippingpreview.cpp
        rip/videodvd/k3bvideodvdtitledelegate.cpp
        rip/videodvd/k3bvideodvdtitlemodel.cpp
    )

    ki18n_wrap_ui(ui_sources rip/videodvd/base_k3bvideodvdrippingwidget.ui)
endif()

if(ENABLE_MUSICBRAINZ)
    set(musicbrainz_sources
        k3btrm.cpp
        k3bmusicbrainz.cpp
        projects/k3bmusicbrainzjob.cpp
        projects/k3bmusicbrainztrackloopupjob.cpp
        projects/k3baudiotracktrmlookupdialog.cpp
    )
endif()

qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/k3binterface.h org.k3b.MainWindow.xml )
qt5_add_dbus_adaptor( dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.MainWindow.xml k3binterface.h K3b::Interface k3binterfaceadaptor K3bInterfaceAdaptor )
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/k3bjobinterface.h org.k3b.Job.xml )
qt5_add_dbus_adaptor( dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.Job.xml k3bjobinterface.h K3b::JobInterface k3bjobinterfaceadaptor K3bJobInterfaceAdaptor )
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/k3bprojectinterface.h org.k3b.Project.xml )
qt5_add_dbus_adaptor( dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.Project.xml k3bprojectinterface.h K3b::ProjectInterface k3bprojectinterfaceadaptor K3bProjectInterfaceAdaptor )
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/k3baudioprojectinterface.h org.k3b.AudioProject.xml )
qt5_add_dbus_adaptor( dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.AudioProject.xml k3baudioprojectinterface.h K3b::AudioProjectInterface k3baudioprojectinterfaceadaptor K3bAudioProjectInterfaceAdaptor )
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/k3bdataprojectinterface.h org.k3b.DataProject.xml )
qt5_add_dbus_adaptor( dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.DataProject.xml k3bdataprojectinterface.h K3b::DataProjectInterface k3bdataprojectinterfaceadaptor K3bDataProjectInterfaceAdaptor )

add_executable(k3b
    k3bwelcomewidget.cpp
    k3bapplication.cpp
    k3bdevicedelegate.cpp
    k3bmediumdelegate.cpp
    k3bmetaitemmodel.cpp
    k3bplacesmodel.cpp
    k3bdiroperator.cpp
    k3bfiletreeview.cpp
    k3bprojecttabwidget.cpp
    k3bsplash.cpp
    k3bfileview.cpp
    k3bdirview.cpp
    k3b.cpp
    main.cpp
    k3bstatusbarmanager.cpp
    k3burlnavigator.cpp
    k3binterface.cpp
    k3bjobinterface.cpp
    k3baudioprojectinterface.cpp
    k3bdataprojectinterface.cpp
    k3bmixedprojectinterface.cpp
    k3bprojectinterface.cpp
    k3bsystemproblemdialog.cpp
    k3bmediacontentsview.cpp
    k3bcontentsview.cpp
    k3bjobprogressdialog.cpp
    k3bburnprogressdialog.cpp
    k3btempdirselectionwidget.cpp
    k3bdatamodewidget.cpp
    k3bwritingmodewidget.cpp
    k3bwriterselectionwidget.cpp
    k3binteractiondialog.cpp
    k3bthememanager.cpp
    k3bprojectmanager.cpp
    k3bflatbutton.cpp
    k3bemptydiscwaiter.cpp
    k3bdebuggingoutputcache.cpp
    k3bdebuggingoutputdialog.cpp
    k3bdebuggingoutputfile.cpp
    k3bappdevicemanager.cpp
    k3bmediaselectioncombobox.cpp
    k3bwidgetshoweffect.cpp
    k3bmediaselectiondialog.cpp
    k3bdiskinfoview.cpp
    k3btimeoutwidget.cpp
    k3bthemedheader.cpp
    k3bthemedlabel.cpp
    k3blsofwrapper.cpp
    k3blsofwrapperdialog.cpp
    k3baction.cpp
    k3bdevicemenu.cpp
    k3bviewcolumnadjuster.cpp
    k3bmodelutils.cpp
    helper/k3bhelperprogramitem.cpp
    misc/k3bimagewritingdialog.cpp
    misc/k3bmediacopydialog.cpp
    misc/k3bmediaformattingdialog.cpp
    option/k3bmiscoptiontab.cpp
    option/k3bexternalbinmodel.cpp
    option/k3bexternalbinoptiontab.cpp
    option/k3bexternalbinparamsmodel.cpp
    option/k3bexternalbinpermissionmodel.cpp
    option/k3badvancedoptiontab.cpp
    option/k3boptiondialog.cpp
    option/k3bdeviceoptiontab.cpp
    option/k3bnotifyoptiontab.cpp
    option/k3bpluginoptiontab.cpp
    option/k3bthemeoptiontab.cpp
    option/k3bthememodel.cpp
    option/k3bdevicewidget.cpp
    option/k3bexternalbinwidget.cpp
    option/k3bcddboptiontab.cpp
    projects/k3baudioburndialog.cpp
    projects/k3baudiocdtextwidget.cpp
    projects/k3baudioeditorwidget.cpp
    projects/k3baudiotrackdialog.cpp
    projects/k3baudiotrackwidget.cpp
    projects/k3baudiotracksplitdialog.cpp
    projects/k3baudioview.cpp
    projects/k3baudioviewimpl.cpp
    projects/k3baudioprojectdelegate.cpp
    projects/k3baudioprojectmodel.cpp
    projects/k3bmixedburndialog.cpp
    projects/k3bmixedview.cpp
    projects/k3bdataadvancedimagesettingsdialog.cpp
    projects/k3bdatadirtreeview.cpp
    projects/k3bdataimagesettingswidget.cpp
    projects/k3bdatapropertiesdialog.cpp
    projects/k3bdataview.cpp
    projects/k3bdataviewimpl.cpp
    projects/k3bdatavolumedescdialog.cpp
    projects/k3bdataburndialog.cpp
    projects/k3bdataprojectdelegate.cpp
    projects/k3bdataprojectmodel.cpp
    projects/k3bdataprojectsortproxymodel.cpp
    projects/k3bbootimagedialog.cpp
    projects/k3bbootimagemodel.cpp
    projects/k3bmovixburndialog.cpp
    projects/k3bmovixoptionswidget.cpp
    projects/k3bmovixview.cpp
    projects/k3bmovixprojectmodel.cpp
    projects/k3bvideodvdburndialog.cpp
    projects/k3bvideodvdview.cpp
    projects/k3bvcdburndialog.cpp
    projects/k3bvcdtrackdialog.cpp
    projects/k3bvcdtrackkeysdelegate.cpp
    projects/k3bvcdtrackkeysmodel.cpp
    projects/k3bvcdview.cpp
    projects/k3bvcdprojectmodel.cpp
    projects/k3bfillstatusdisplay.cpp
    projects/k3bview.cpp
    projects/k3bdirproxymodel.cpp
    projects/k3bprojectburndialog.cpp
    projects/k3bprojectplugindialog.cpp
    projects/k3bdatamultisessioncombobox.cpp
    projects/k3bdataurladdingdialog.cpp
    projects/k3baudiodatasourceeditwidget.cpp
    projects/k3baudiotrackaddingdialog.cpp
    projects/k3bencodingconverter.cpp
    projects/k3bdatamultisessionimportdialog.cpp
    projects/k3bvolumenamewidget.cpp
    projects/kostore/KoStore.cpp
    projects/kostore/KoZipStore.cpp
    projects/kostore/KoStoreBase.cpp
    rip/k3baudioripjob.cpp
    rip/k3bpatternparser.cpp
    rip/k3baudiorippingdialog.cpp
    rip/k3baudiocdview.cpp
    rip/k3bcddbpatternwidget.cpp
    rip/k3bmassaudioencodingjob.cpp
    rip/k3bvideocdinfo.cpp
    rip/k3bvideocdview.cpp
    rip/k3bvideocdrip.cpp
    rip/k3bvideocdrippingdialog.cpp
    rip/k3bcuefilewriter.cpp
    rip/k3baudioconvertingoptionwidget.cpp
    rip/k3baudioprojectconvertingdialog.cpp
    rip/k3baudioprojectconvertingjob.cpp
    rip/categories.cpp
    rip/k3baudiotrackmodel.cpp
    ${ui_sources}
    ${audioplayer_sources}
    ${musicbrainz_sources}
    ${videodvd_sources}
    ${dbus_sources}
)

target_include_directories(k3b PRIVATE
    ${CMAKE_BINARY_DIR}
    helper
    projects
    projects/kostore
)

target_link_libraries(k3b
    k3bdevice
    k3blib
    KF5::Cddb
    KF5::Archive
    KF5::I18n
    KF5::IconThemes
    KF5::JobWidgets
    KF5::KIOFileWidgets
    KF5::KCMUtils
    KF5::NewStuff
    KF5::Notifications
    KF5::NotifyConfig
    KF5::Solid
    KF5::XmlGui
    KF5::ConfigCore
)

if(Qt5WebKitWidgets_FOUND)
    target_link_libraries(k3b Qt5::WebKitWidgets)
endif()

if(ICONV_FOUND)
    target_include_directories(k3b PRIVATE ${ICONV_INCLUDE_DIR})
    target_link_libraries(k3b ${ICONV_LIBRARIES})
endif()

if(ENABLE_AUDIO_PLAYER)
    target_link_libraries(k3b Qt5::Multimedia)
endif()

if(ENABLE_MUSICBRAINZ)
    target_link_libraries(k3b ${MUSICBRAINZ_LIBRARIES})
endif()

install(TARGETS k3b ${INSTALL_TARGETS_DEFAULT_ARGS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/option/k3btheme.knsrc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/option/k3btheme.knsrc
               @ONLY)


########### install files ###############
install( PROGRAMS  org.kde.k3b.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
#install( FILES  k3b-iso.desktop k3b-cue.desktop DESTINATION ${APPLNK_INSTALL_DIR}/.hidden/ )
install( FILES  k3b.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
install( FILES  k3bui.rc k3bdeviceui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/k3b )
install( FILES  org.kde.k3b.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

if (${ECM_VERSION} STRGREATER "5.58.0")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/option/k3btheme.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
else()
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/option/k3btheme.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
endif()

set( SHARED_MIME_INFO_MINIMUM_VERSION "0.23" )
find_package( SharedMimeInfo REQUIRED )
install( FILES x-k3b.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
