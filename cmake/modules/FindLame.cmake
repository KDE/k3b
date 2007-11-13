# - Try to find Lame
# Once done this will define
#
#  LAME_FOUND - system has Lame
#  LAME_INCLUDE_DIR - the Lame include directory
#  LAME_LIBRARIES - Link these to use Lame
#  LAME_DEFINITIONS - Compiler switches required for using Lame
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( LAME_INCLUDE_DIR AND LAME_LIBRARIES )
   # in cache already
   SET(Lame_FIND_QUIETLY TRUE)
endif ( LAME_INCLUDE_DIR AND LAME_LIBRARIES )

FIND_PATH(LAME_INCLUDE_DIR NAMES lame/lame.h
)

FIND_LIBRARY(LAME_LIBRARIES NAMES mp3lame
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lame DEFAULT_MSG LAME_INCLUDE_DIR LAME_LIBRARIES )

# show the LAME_INCLUDE_DIR and LAME_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(LAME_INCLUDE_DIR LAME_LIBRARIES )

