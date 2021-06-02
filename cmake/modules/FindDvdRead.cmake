# - Try to find DvdRead
# Once done this will define
#
#  DVDREAD_FOUND - system has DvdRead
#  DVDREAD_INCLUDE_DIR - the DvdRead include directory
#  DVDREAD_LIBRARIES - Link these to use DvdRead
#  DVDREAD_DEFINITIONS - Compiler switches required for using DvdRead
#
# SPDX-FileCopyrightText: 2007 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

if( DVDREAD_INCLUDE_DIR AND DVDREAD_LIBRARIES )
   # in cache already
   set(DvdRead_FIND_QUIETLY TRUE)
endif()

find_path(DVDREAD_INCLUDE_DIR NAMES dvdread/dvd_reader.h)

find_library(DVDREAD_LIBRARIES NAMES dvdread)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DvdRead DEFAULT_MSG DVDREAD_INCLUDE_DIR DVDREAD_LIBRARIES)

if(DVDREAD_FOUND)
    add_library(dvdread SHARED IMPORTED)
    set_target_properties(dvdread PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DVDREAD_INCLUDE_DIR}"
        IMPORTED_LOCATION "${DVDREAD_LIBRARIES}"
    )
endif()

# show the DVDREAD_INCLUDE_DIR and DVDREAD_LIBRARIES variables only in the advanced view
mark_as_advanced(DVDREAD_INCLUDE_DIR DVDREAD_LIBRARIES)
