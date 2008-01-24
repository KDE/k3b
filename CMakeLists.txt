project(k3b)

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)
macro_optional_find_package(DvdRead)
macro_optional_find_package(MusicBrainz)
find_package(Kcddb REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT3_SUPPORT)

include(ConfigureChecks.cmake)

include_directories( ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}
${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libk3bdevice
${CMAKE_CURRENT_BINARY_DIR}/libk3bdevice )

SET(K3B_VERSION_STRING "1.95-svn")

configure_file (config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h )
configure_file (k3b.lsm.cmake ${CMAKE_CURRENT_BINARY_DIR}/k3b.lsm )

option(K3BSETUP_BUILD "Build k3bsetup" ON)
option(ADD_K3B_DEBUG "K3b additional debugging support")


add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
# TODO port to strigi
#add_subdirectory( kfile-plugins )
if(K3BSETUP_BUILD)
   add_subdirectory(k3bsetup)
endif(K3BSETUP_BUILD)


# apidox
find_package(Doxygen)

if(DOXYGEN_EXECUTABLE)
  configure_file(${k3b_SOURCE_DIR}/Doxyfile.cmake ${k3b_BINARY_DIR}/Doxyfile)

  if(EXISTS ${QT_DOC_DIR}/html)
    set(QTDOCS "${QT_DOC_DIR}/html")
  else(EXISTS ${QT_DOC_DIR}/html)
    set(QTDOCS "http://doc.trolltech.com/4.3/")
  endif(EXISTS ${QT_DOC_DIR}/html)

  add_custom_target(
    apidox
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND docs/html/installdox -l qt4.tag@${QTDOCS} docs/html/*.html)
endif(DOXYGEN_EXECUTABLE)
