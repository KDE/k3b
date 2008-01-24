# - Try to find Kcddb
# Once done this will define
#
#  KCDDB_FOUND - system has Kcddb
#  KCDDB_INCLUDE_DIR - the Kcddb include directory
#  KCDDB_LIBRARIES - Link these to use Kcddb
#  KCDDB_DEFINITIONS - Compiler switches required for using Kcddb
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( KCDDB_INCLUDE_DIR AND KCDDB_LIBRARIES )
   # in cache already
   SET(Kcddb_FIND_QUIETLY TRUE)
endif ( KCDDB_INCLUDE_DIR AND KCDDB_LIBRARIES )

FIND_PATH(KCDDB_INCLUDE_DIR NAMES client.h
  PATH_SUFFIXES libkcddb
  PATHS
  ${KDE4_INCLUDE_DIR}
  ${INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(KCDDB_LIBRARIES NAMES kcddb
    PATHS
    ${KDE4_LIB_DIR}
    ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Kcddb "kcddb was not found. Need to install from kdemultimedia" KCDDB_INCLUDE_DIR KCDDB_LIBRARIES )

# show the KCDDB_INCLUDE_DIR and KCDDB_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KCDDB_INCLUDE_DIR KCDDB_LIBRARIES )

