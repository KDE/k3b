# - Try to find Polkit-qt
# Once done this will define
#
#  POLKITQT_FOUND - system has Polkit-qt
#  POLKITQT_INCLUDE_DIR - the Polkit-qt include directory
#  POLKITQT_LIBRARIES - Link these to use all Polkit-qt libs
#  POLKITQT_CORE_LIBRARY
#  POLKITQT_GUI_LIBRARY
#  POLKITQT_DEFINITIONS - Compiler switches required for using Polkit-qt

# Copyright (c) 2008, Adrien Bustany, <madcat@mymadcat.com>
# Copyright (c) 2009, Daniel Nicoletti, <dantti85-pk@yahoo.com.br>
#
# Redistribution and use is allowed according to the terms of the GPLv2+ license.

IF (POLKITQT_INCLUDE_DIR AND POLKITQT_LIB)
    SET(POLKITQT_FIND_QUIETLY TRUE)
ENDIF (POLKITQT_INCLUDE_DIR AND POLKITQT_LIB)

# FIND_PATH( POLKITQT_INCLUDE_DIR PolicyKit/policykit-qt/Polkit-qt )
FIND_PATH( POLKITQT_INCLUDE_DIR PolicyKit/polkit-qt/ )

FIND_LIBRARY( POLKITQT_CORE_LIBRARY NAMES polkit-qt-core )
FIND_LIBRARY( POLKITQT_GUI_LIBRARY NAMES polkit-qt-gui )

IF (POLKITQT_INCLUDE_DIR AND POLKITQT_CORE_LIBRARY AND POLKITQT_GUI_LIBRARY)
   SET(POLKITQT_FOUND TRUE)
ELSE (POLKITQT_INCLUDE_DIR AND POLKITQT_CORE_LIBRARY AND POLKITQT_GUI_LIBRARY)
   SET(POLKITQT_FOUND FALSE)
ENDIF (POLKITQT_INCLUDE_DIR AND POLKITQT_CORE_LIBRARY AND POLKITQT_GUI_LIBRARY)

SET(POLKITQT_LIBRARIES ${POLKITQT_CORE_LIBRARY} ${POLKITQT_GUI_LIBRARY})

SET(POLKITQT_INCLUDE_DIR ${POLKITQT_INCLUDE_DIR}/PolicyKit/polkit-qt ${POLKITQT_INCLUDE_DIR}/PolicyKit/)

SET(POLICY_FILES_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/PolicyKit/policy/)

IF (POLKITQT_FOUND)
  IF (NOT POLKITQT_FIND_QUIETLY)
    MESSAGE(STATUS "Found Polkit-qt: ${POLKITQT_LIBRARIES}")
  ENDIF (NOT POLKITQT_FIND_QUIETLY)
ELSE (POLKITQT_FOUND)
  IF (POLKITQT_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could NOT find Polkit-qt")
  ENDIF (POLKITQT_FIND_REQUIRED)
ENDIF (POLKITQT_FOUND)

MARK_AS_ADVANCED(POLKITQT_INCLUDE_DIR POLKITQT_LIB)

#include (PkgConfigGetVar)

macro(dbus_add_activation_system_service _sources)
    #PKGCONFIG_GETVAR(dbus-1 session_bus_services_dir _install_dir)
    foreach (_i ${_sources})
        get_filename_component(_service_file ${_i} ABSOLUTE)
        string(REGEX REPLACE "\\.service.*$" ".service" _output_file ${_i})
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_output_file})
        configure_file(${_service_file} ${_target})
        install(FILES ${_target} DESTINATION ${SHARE_INSTALL_PREFIX}/dbus-1/system-services )
        #install(FILES ${_target} DESTINATION ${_install_dir})
    endforeach (_i ${ARGN})
endmacro(dbus_add_activation_system_service _sources)

