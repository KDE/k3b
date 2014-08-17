# - Try to find Samplerate
# Once done this will define
#
#  SAMPLERATE_FOUND - system has Samplerate
#  SAMPLERATE_INCLUDE_DIR - the Samplerate include directory
#  SAMPLERATE_LIBRARIES - Link these to use Samplerate
#  SAMPLERATE_DEFINITIONS - Compiler switches required for using Samplerate
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Copyright (c) 2007, Laurent Montel, <montel@kde.org>

if(SAMPLERATE_INCLUDE_DIR AND SAMPLERATE_LIBRARIES)
    # in cache already
    set(Samplerate_FIND_QUIETLY TRUE)
endif()

find_path(SAMPLERATE_INCLUDE_DIR NAMES samplerate.h)

find_library(SAMPLERATE_LIBRARIES NAMES samplerate samplerate-0 libsamplerate libsamplerate-0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Samplerate DEFAULT_MSG SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARIES)

add_library(samplerate SHARED IMPORTED)
set_target_properties(samplerate PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SAMPLERATE_INCLUDE_DIR}"
    IMPORTED_LOCATION "${SAMPLERATE_LIBRARIES}"
)

# show the SAMPLERATE_INCLUDE_DIR and SAMPLERATE_LIBRARIES variables only in the advanced view
mark_as_advanced(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARIES)
