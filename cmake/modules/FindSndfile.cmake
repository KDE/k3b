#
# Try to find libsndfile
# Once done this will define
#
#  SNDFILE_FOUND - libsndfile was found
#  SNDFILE_INCLUDE_DIR - the libsndfile include directory
#  SNDFILE_LIBRARIES - libsndfile libraries to link to
#
# Copyright (c) 2008, Sebastian Trueg, <trueg@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARIES )
   # in cache already
   SET(Sndfile_FIND_QUIETLY TRUE)
endif ( SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARIES )

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)

  PKGCONFIG(sndfile _SndfileIncDir _SndfileLinkDir _SndfileLinkFlags _SndfileCflags)
ENDIF (NOT WIN32)


FIND_PATH(SNDFILE_INCLUDE_DIR
  NAMES sndfile.h
  PATHS ${_SndfileIncDir}
  NO_DEFAULT_PATH
)

FIND_LIBRARY(SNDFILE_LIBRARIES 
  NAMES sndfile
  PATHS ${_SndfileLinkDir}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sndfile DEFAULT_MSG SNDFILE_INCLUDE_DIR SNDFILE_LIBRARIES )

# show the SNDFILE_INCLUDE_DIR and SNDFILE_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARIES )

