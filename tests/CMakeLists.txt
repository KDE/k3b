
add_executable(k3bdataprojectmodeltest
    k3bdataprojectmodeltest.cpp
    k3btestutils.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataprojectmodel.cpp)
target_include_directories(k3bdataprojectmodeltest PRIVATE
    ${CMAKE_BINARY_DIR}/libk3bdevice
    ${CMAKE_SOURCE_DIR}/libk3bdevice
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries(k3bdataprojectmodeltest
    Qt5::Test
    KF5::IconThemes
    KF5::I18n
    k3blib)
add_test(k3bdataprojectmodeltest k3bdataprojectmodeltest)

add_executable(k3bglobalstest k3bglobalstest.cpp)
target_include_directories(k3bglobalstest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3bdevice)
target_link_libraries(k3bglobalstest
    Qt5::Test
    k3blib)
add_test(k3bglobalstest k3bglobalstest)

add_executable(k3bmetaitemmodeltest
    k3bmetaitemmodeltest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmetaitemmodel.cpp)
target_include_directories(k3bmetaitemmodeltest PRIVATE
    ${CMAKE_BINARY_DIR}/libk3b
    ${CMAKE_SOURCE_DIR}/libk3b/core
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries( k3bmetaitemmodeltest
    Qt5::Gui
    Qt5::Test
    KF5::ConfigCore)
add_test(k3bmetaitemmodeltest k3bmetaitemmodeltest)

add_executable(k3bmodelutilstest
    k3bmodelutilstest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmodelutils.cpp)
target_include_directories(k3bmodelutilstest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries(k3bmodelutilstest
    Qt5::Gui
    Qt5::Test)
add_test(k3bmodelutilstest k3bmodelutilstest)

add_executable(k3bdeviceglobalstest k3bdeviceglobalstest.cpp)
target_include_directories(k3bdeviceglobalstest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3bdevice)
target_link_libraries(k3bdeviceglobalstest
    Qt5::Test
    KF5::KIOCore
    k3bdevice)
add_test(k3bdeviceglobalstest k3bdeviceglobalstest)

qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3binterface.h org.k3b.MainWindow.xml )
qt5_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.MainWindow.xml ${CMAKE_SOURCE_DIR}/src/k3binterface.h K3b::Interface k3binterfaceadaptor K3bInterfaceAdaptor)
qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3bjobinterface.h org.k3b.Job.xml)
qt5_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.Job.xml ${CMAKE_SOURCE_DIR}/src/k3bjobinterface.h K3b::JobInterface k3bjobinterfaceadaptor K3bJobInterfaceAdaptor)
qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3bprojectinterface.h org.k3b.Project.xml)
qt5_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.Project.xml ${CMAKE_SOURCE_DIR}/src/k3bprojectinterface.h K3b::ProjectInterface k3bprojectinterfaceadaptor K3bProjectInterfaceAdaptor)
qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3baudioprojectinterface.h org.k3b.AudioProject.xml)
qt5_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.AudioProject.xml ${CMAKE_SOURCE_DIR}/src/k3baudioprojectinterface.h K3b::AudioProjectInterface k3baudioprojectinterfaceadaptor K3bAudioProjectInterfaceAdaptor)
qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3bdataprojectinterface.h org.k3b.DataProject.xml)
qt5_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.DataProject.xml ${CMAKE_SOURCE_DIR}/src/k3bdataprojectinterface.h K3b::DataProjectInterface k3bdataprojectinterfaceadaptor K3bDataProjectInterfaceAdaptor)

if(ENABLE_MUSICBRAINZ)
    set(musicbrainz_sources
        ${CMAKE_SOURCE_DIR}/src/k3btrm.cpp
        ${CMAKE_SOURCE_DIR}/src/k3bmusicbrainz.cpp
        ${CMAKE_SOURCE_DIR}/src/projects/k3bmusicbrainztrackloopupjob.cpp
        ${CMAKE_SOURCE_DIR}/src/projects/k3bmusicbrainzjob.cpp
        ${CMAKE_SOURCE_DIR}/src/projects/k3baudiotracktrmlookupdialog.cpp
    )
endif()
add_executable(k3bexternalbinmanagertest 
    k3bexternalbinmanagertest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bburnprogressdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3blsofwrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bemptydiscwaiter.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bjobinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputcache.cpp
    ${CMAKE_SOURCE_DIR}/src/k3blsofwrapperdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiotrackwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bpatternparser.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudioprojectconvertingjob.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdatamultisessioncombobox.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bcuefilewriter.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bmassaudioencodingjob.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudioripjob.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bcddbpatternwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudioconvertingoptionwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdtrackkeysdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudioprojectconvertingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiotrackdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bbootimagemodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioeditorwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiodatasourceeditwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiotracksplitdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiotrackaddingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioprojectdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioprojectmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bbootimagedialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bencodingconverter.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataurladdingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataprojectdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataprojectsortproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataprojectmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudiocdtextwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmixedburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdirproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataviewimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioviewimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bprojectplugindialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bfillstatusdisplay.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmovixoptionswidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdtrackkeysmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdtrackdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdprojectmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmovixburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdatavolumedescdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataadvancedimagesettingsdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/helper/k3bhelperprogramitem.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bexternalbinpermissionmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bexternalbinparamsmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bexternalbinmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataimagesettingswidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bprojectburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdatapropertiesdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmovixprojectmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvolumenamewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmovixprojectmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvideodvdburndialog.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bpluginoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bcddboptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bdevicewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bexternalbinwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bthememodel.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3badvancedoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bthemeoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bnotifyoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bexternalbinoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bdeviceoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3bmiscoptiontab.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdatamodewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmediaselectiondialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bsystemproblemdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/misc/k3bimagewritingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/option/k3boptiondialog.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvideodvdview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmovixview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bvcdview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bmixedview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3baudioview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdiroperator.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudiorippingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bviewcolumnadjuster.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudiotrackmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bvideocdrip.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bvideocdrippingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bvideocdinfo.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdrippingjob.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdaudiomodel.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdrippingwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdrippingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmodelutils.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdrippingpreview.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdtitlemodel.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdtitledelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmediacontentsview.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/videodvd/k3bvideodvdrippingview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdiskinfoview.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3bvideocdview.cpp
    ${CMAKE_SOURCE_DIR}/src/rip/k3baudiocdview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bcontentsview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bfileview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3burlnavigator.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdirview.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdatamultisessionimportdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/misc/k3bmediaformattingdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3btempdirselectionwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bwritingmodewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bwriterselectionwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmediumdelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmediaselectioncombobox.cpp
    ${CMAKE_SOURCE_DIR}/src/k3binteractiondialog.cpp
    ${CMAKE_SOURCE_DIR}/src/misc/k3bmediacopydialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3baction.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bappdevicemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdevicemenu.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmetaitemmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bplacesmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdevicedelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bfiletreeview.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bthemedheader.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bstatusbarmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmixedprojectinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdataprojectinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bprojectinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3baudioprojectinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/kostore/KoStoreBase.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/kostore/KoZipStore.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/kostore/KoStore.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bprojectmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/k3binterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bprojecttabwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bflatbutton.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bwelcomewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/k3b.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bsplash.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bapplication.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bthememanager.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bthemedlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputfile.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bjobprogressdialog.cpp
    ${dbus_sources}
    ${musicbrainz_sources})
target_include_directories(k3bexternalbinmanagertest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3b
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects
    ${CMAKE_SOURCE_DIR}/src/projects/kostore
    ${CMAKE_SOURCE_DIR}/src/helper
    ${CMAKE_BINARY_DIR}/src)
target_link_libraries(k3bexternalbinmanagertest
    Qt5::Test
    Qt5::WebKitWidgets
    KF5::KIOCore
    KF5::ConfigCore
    KF5::I18n
    KF5::XmlGui
    KF5::Notifications
    KF5::Solid
    KF5::IconThemes
    KF5::JobWidgets
    KF5::Archive
    KF5::KIOFileWidgets
    KF5::Cddb
    KF5::NewStuff
    KF5::NotifyConfig
    KF5::KCMUtils
    k3blib
    k3bdevice)
if(ENABLE_MUSICBRAINZ)
    target_link_libraries(k3bexternalbinmanagertest ${MUSICBRAINZ_LIBRARIES})
endif()
add_test(k3bexternalbinmanagertest k3bexternalbinmanagertest)

if(LIBFUZZER_FOUND)
    add_executable(k3bfuzzertest 
        k3bfuzzertest.cpp
        ${CMAKE_SOURCE_DIR}/libk3b/tools/libisofs/isofs.cpp)
    target_include_directories(k3bfuzzertest PRIVATE
        ${CMAKE_SOURCE_DIR}/libk3b)
    target_link_libraries(k3bfuzzertest
        Qt5::Core
        ${LIBFUZZER_LIBRARIES})
endif()
