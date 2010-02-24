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

if( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
    # in cache already
    set(MUSE_FIND_QUIETLY TRUE)
endif( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )

include(CheckIncludeFiles)
check_include_files(mpc/mpcdec.h HAVE_MPC_MPCDEC_H)
check_include_files(mpcdec/mpcdec.h HAVE_MPCDEC_MPCDEC_H)
check_include_files(musepack/musepack.h HAVE_MUSEPACK_MUSEPACK_H)

if( HAVE_MPC_MPCDEC_H )
    find_path( MUSE_INCLUDE_DIR mpc/mpcdec.h )
    find_library( MUSE_LIBRARIES NAMES mpcdec )
    set( MPC_HEADER_FILE "<mpc/mpcdec.h>" )
elseif( HAVE_MPCDEC_MPCDEC_H )
    find_path( MUSE_INCLUDE_DIR mpcdec/mpcdec.h )
    find_library( MUSE_LIBRARIES NAMES mpcdec )
    set( MPC_HEADER_FILE "<mpcdec/mpcdec.h>" )
    set( MPC_OLD_API 1)
elseif( HAVE_MUSEPACK_MUSEPACK_H )
    find_path( MUSE_INCLUDE_DIR musepack/musepack.h )
    find_library( MUSE_LIBRARIES NAMES musepack )
    set( MPC_HEADER_FILE "<musepack/musepack.h>" )
    set( MPC_OLD_API 1 )
endif( HAVE_MPC_MPCDEC_H )

if( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
    set( MUSE_FOUND TRUE )
else( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )
    set( MUSE_FOUND FALSE )
endif( MUSE_INCLUDE_DIR AND MUSE_LIBRARIES )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MUSE DEFAULT_MSG MUSE_INCLUDE_DIR MUSE_LIBRARIES MPC_HEADER_FILE )

# show the MUSE_INCLUDE_DIR and MUSE_LIBRARIES variables only in the advanced view
mark_as_advanced(MUSE_INCLUDE_DIR MUSE_LIBRARIES )
