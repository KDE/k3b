
add_executable(k3bdataprojectmodeltest
    k3bdataprojectmodeltest.cpp
    k3btestutils.cpp
    ${CMAKE_SOURCE_DIR}/src/projects/k3bdataprojectmodel.cpp)
target_include_directories(k3bdataprojectmodeltest PRIVATE
    ${CMAKE_BINARY_DIR}/libk3bdevice
    ${CMAKE_SOURCE_DIR}/libk3bdevice
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries(k3bdataprojectmodeltest
    Qt6::Test
    KF6::IconThemes
    KF6::I18n
    k3blib)
add_test(NAME k3bdataprojectmodeltest COMMAND k3bdataprojectmodeltest)

add_executable(k3bglobalstest k3bglobalstest.cpp)
target_include_directories(k3bglobalstest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3bdevice)
target_link_libraries(k3bglobalstest
    Qt6::Test
    k3blib)
add_test(NAME k3bglobalstest COMMAND k3bglobalstest)

add_executable(k3bmetaitemmodeltest
    k3bmetaitemmodeltest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmetaitemmodel.cpp)
target_include_directories(k3bmetaitemmodeltest PRIVATE
    ${CMAKE_BINARY_DIR}/libk3b
    ${CMAKE_SOURCE_DIR}/libk3b/core
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries( k3bmetaitemmodeltest
    Qt6::Gui
    Qt6::Test
    KF6::ConfigCore)
add_test(NAME k3bmetaitemmodeltest COMMAND k3bmetaitemmodeltest)

add_executable(k3bmodelutilstest
    k3bmodelutilstest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmodelutils.cpp)
target_include_directories(k3bmodelutilstest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects)
target_link_libraries(k3bmodelutilstest
    Qt6::Gui
    Qt6::Test)
add_test(NAME k3bmodelutilstest COMMAND k3bmodelutilstest)

add_executable(k3bdeviceglobalstest k3bdeviceglobalstest.cpp)
target_include_directories(k3bdeviceglobalstest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3bdevice)
target_link_libraries(k3bdeviceglobalstest
    Qt6::Test
    KF6::KIOCore
    k3bdevice)
add_test(NAME k3bdeviceglobalstest COMMAND k3bdeviceglobalstest)

qt_generate_dbus_interface(${CMAKE_SOURCE_DIR}/src/k3bjobinterface.h org.k3b.Job.xml)
qt_add_dbus_adaptor(dbus_sources ${CMAKE_CURRENT_BINARY_DIR}/org.k3b.Job.xml ${CMAKE_SOURCE_DIR}/src/k3bjobinterface.h K3b::JobInterface k3bjobinterfaceadaptor K3bJobInterfaceAdaptor)

add_executable(k3bexternalbinmanagertest 
    k3bexternalbinmanagertest.cpp
    stub-k3bapplication.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bburnprogressdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bemptydiscwaiter.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bjobinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputcache.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bthememanager.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bthemedlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bdebuggingoutputfile.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bjobprogressdialog.cpp
    ${dbus_sources})
target_include_directories(k3bexternalbinmanagertest PRIVATE
    ${CMAKE_SOURCE_DIR}/libk3b
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/projects
    ${CMAKE_SOURCE_DIR}/src/projects/kostore
    ${CMAKE_SOURCE_DIR}/src/helper
    ${CMAKE_BINARY_DIR}/src)
target_link_libraries(k3bexternalbinmanagertest
    Qt6::Test
    KF6::I18n
    KF6::XmlGui
    KF6::Notifications
    KF6::IconThemes
    k3blib
    k3bdevice)
add_test(NAME k3bexternalbinmanagertest COMMAND k3bexternalbinmanagertest)

if(LIBFUZZER_FOUND)
    find_package(Threads)
    add_executable(k3bfuzzertest 
        k3bfuzzertest.cpp)
    target_link_libraries(k3bfuzzertest
        Qt6::Core
        ${LIBFUZZER_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})
endif()
