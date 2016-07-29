# - Try to find DvdRead
# Once done this will define
#
#  DVDREAD_FOUND - system has DvdRead
#  DVDREAD_INCLUDE_DIR - the DvdRead include directory
#  DVDREAD_LIBRARIES - Link these to use DvdRead
#  DVDREAD_DEFINITIONS - Compiler switches required for using DvdRead
#
# Copyright (c) 2007, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( DVDREAD_INCLUDE_DIR AND DVDREAD_LIBRARIES )
   # in cache already
   SET(DvdRead_FIND_QUIETLY TRUE)
endif ( DVDREAD_INCLUDE_DIR AND DVDREAD_LIBRARIES )

FIND_PATH(DVDREAD_INCLUDE_DIR NAMES dvdread/dvd_reader.h
)

FIND_LIBRARY(DVDREAD_LIBRARIES NAMES dvdread
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DvdRead DEFAULT_MSG DVDREAD_INCLUDE_DIR DVDREAD_LIBRARIES )

# show the DVDREAD_INCLUDE_DIR and DVDREAD_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(DVDREAD_INCLUDE_DIR DVDREAD_LIBRARIES )

