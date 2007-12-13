# - Try to find Lame
# Once done this will define
#
#  MUSE_FOUND - system has Muse
#  MUSE_INCLUDE_DIR - the Muse include directory
#  MUSE_LIBRARIES - Link these to use Muse
#  MUSE_DEFINITIONS - Compiler switches required for using Muse
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
   # in cache already
   SET(Muse_FIND_QUIETLY TRUE)
endif ( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )


FIND_PATH(MUSE_INCLUDE_DIR NAMES mpcdec/mpcdec.h
)

if(MUSE_INCLUDE_DIR)
  FIND_LIBRARY(MUSE_LIBRARIES NAMES mpcdec)
  set(MPC_HEADER_FILE "<mpcdec/mpcdec.h>")
else(MUSE_INCLUDE_DIR)
  FIND_PATH(MUSE_INCLUDE_DIR NAMES musepack/musepack.h)
  set(MPC_HEADER_FILE "<musepack/musepack.h>")
  FIND_LIBRARY(MUSE_LIBRARIES NAMES musepack )
endif(MUSE_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Muse DEFAULT_MSG MUSE_INCLUDE_DIR MUSE_LIBRARIES )

# show the MUSE_INCLUDE_DIR and MUSE_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(MUSE_INCLUDE_DIR MUSE_LIBRARIES )

