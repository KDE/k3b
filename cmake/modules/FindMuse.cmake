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


IF( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
     # in cache already
     SET(MUSE_FIND_QUIETLY TRUE)
ENDIF( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )

UNSET( MUSE_INCLUDE_DIR CACHE )

FIND_PATH( MUSE_INCLUDE_DIR mpc/mpcdec.h )
if( MUSE_INCLUDE_DIR )
    FIND_LIBRARY( MUSE_LIBRARIES NAMES mpcdec )
    SET( MPC_HEADER_FILE "<mpc/mpcdec.h>" )
else( MUSE_INCLUDE_DIR )
    FIND_PATH( MUSE_INCLUDE_DIR mpcdec/mpcdec.h )
    IF( MUSE_INCLUDE_DIR )
        FIND_LIBRARY( MUSE_LIBRARIES NAMES mpcdec )
        SET( MPC_HEADER_FILE "<mpcdec/mpcdec.h>" )
        SET( MPC_OLD_API 1)
    ELSE( MUSE_INCLUDE_DIR )
        FIND_PATH( MUSE_INCLUDE_DIR musepack/musepack.h )
        FIND_LIBRARY( MUSE_LIBRARIES NAMES musepack )
        SET( MPC_HEADER_FILE "<musepack/musepack.h>" )
        SET( MPC_OLD_API 1 )
    ENDIF( MUSE_INCLUDE_DIR )
ENDIF( MUSE_INCLUDE_DIR )

IF( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
    SET( MUSE_FOUND TRUE )
ELSE( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
    SET( MUSE_FOUND FALSE )
ENDIF( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MUSE DEFAULT_MSG MUSE_INCLUDE_DIR MUSE_LIBRARIES MPC_HEADER_FILE )

# show the MUSE_INCLUDE_DIR and MUSE_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(MUSE_INCLUDE_DIR MUSE_LIBRARIES )

