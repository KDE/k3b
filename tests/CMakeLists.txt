project(tests)

include_directories(
    ${k3b_SOURCE_DIR}/src
    ${k3b_SOURCE_DIR}/src/projects
)

kde4_add_unit_test( k3bglobalstest
    k3bglobalstest.cpp )
target_link_libraries( k3bglobalstest
    ${QT_QTTEST_LIBRARY}
    k3b )

kde4_add_unit_test( k3bmetaitemmodeltest
    k3bmetaitemmodeltest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmetaitemmodel.cpp )
target_link_libraries( k3bmetaitemmodeltest
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDEUI_LIBS} )

kde4_add_unit_test( k3bmodelutilstest
    k3bmodelutilstest.cpp
    ${CMAKE_SOURCE_DIR}/src/k3bmodelutils.cpp )
target_link_libraries( k3bmodelutilstest
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS} )
