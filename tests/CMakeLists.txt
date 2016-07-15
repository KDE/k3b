
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
