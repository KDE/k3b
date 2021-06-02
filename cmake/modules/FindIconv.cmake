# This file is part of Strigi Desktop Search
#
# SPDX-FileCopyrightText: 2006 Jos van den Oever <jos@vandenoever.info>
#
# SPDX-License-Identifier: LGPL-2.0-or-later

# - Try to find Iconv 
# Once done this will define 
# 
#  ICONV_FOUND - system has Iconv 
#  ICONV_INCLUDE_DIR - the Iconv include directory 
#  ICONV_LIBRARIES - Link these to use Iconv 
#  ICONV_SECOND_ARGUMENT_IS_CONST - the second argument for iconv() is const
# 
include(CheckCXXSourceCompiles)

if(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
  # Already in cache, be silent
  set(ICONV_FIND_QUIETLY TRUE)
endif()

find_path(ICONV_INCLUDE_DIR iconv.h)
 
find_library(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv DEFAULT_MSG ICONV_INCLUDE_DIR ICONV_LIBRARIES)
 
if(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
   set(ICONV_FOUND TRUE)
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 

set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
IF(ICONV_FOUND)
  check_cxx_source_compiles("
  #include <iconv.h>
  int main(){
    iconv_t conv = 0;
    const char* in = 0;
    size_t ilen = 0;
    char* out = 0;
    size_t olen = 0;
    iconv(conv, &in, &ilen, &out, &olen);
    return 0;
  }
" ICONV_SECOND_ARGUMENT_IS_CONST )
ENDIF(ICONV_FOUND)
set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

IF(ICONV_FOUND) 
  IF(NOT ICONV_FIND_QUIETLY) 
    MESSAGE(STATUS "Found Iconv: ${ICONV_LIBRARIES}") 
  ENDIF(NOT ICONV_FIND_QUIETLY) 
ELSE(ICONV_FOUND) 
  IF(Iconv_FIND_REQUIRED) 
    MESSAGE(FATAL_ERROR "Could not find Iconv") 
  ENDIF(Iconv_FIND_REQUIRED) 
ENDIF(ICONV_FOUND) 

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARIES
  ICONV_SECOND_ARGUMENT_IS_CONST
)
