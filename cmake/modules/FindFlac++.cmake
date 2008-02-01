# - Try to find Flac, the Free Lossless Audio Codec
# Once done this will define
#
#  FLAC++_FOUND - system has Flac
#  FLAC++_INCLUDE_DIR - the Flac include directory
#  FLAC++_LIBRARIES - Link these to use Flac
#
# No version checking is done - use FLAC_API_VERSION_CURRENT to
# conditionally compile version-dependent code

# Copyright (c) 2008, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(FLAC++_INCLUDE_DIR AND FLAC++_LIBRARIES)
    # Already in cache, be silent
    set(Flac++_FIND_QUIETLY TRUE)	
endif(FLAC++_INCLUDE_DIR AND FLAC++_LIBRARIES)

FIND_PATH(FLAC++_INCLUDE_DIR FLAC++/metadata.h)

FIND_LIBRARY(FLAC++_LIBRARIES NAMES FLAC++ )




IF(FLAC++_INCLUDE_DIR AND FLAC++_LIBRARIES)
   SET(FLAC++_FOUND TRUE)
ENDIF(FLAC++_INCLUDE_DIR AND FLAC++_LIBRARIES)

IF(FLAC++_FOUND)
   IF(NOT Flac++_FIND_QUIETLY)
      MESSAGE(STATUS "Found Flac++: ${FLAC++_LIBRARIES}")
   ENDIF(NOT Flac++_FIND_QUIETLY)
ELSE(FLAC++_FOUND)
   IF(Flac++_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Flac++")
   ENDIF(Flac++_FIND_REQUIRED)
   IF(NOT Flac++_FIND_QUIETLY)
      MESSAGE(STATUS "Could not find Flac++")
   ENDIF(NOT Flac++_FIND_QUIETLY)
ENDIF(FLAC++_FOUND)

MARK_AS_ADVANCED(FLAC++_INCLUDE_DIR FLAC++_LIBRARIES )

