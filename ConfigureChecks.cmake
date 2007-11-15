include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(CheckStructMember)
include(CheckCXXSourceCompiles)

macro_bool_to_01(ADD_K3B_DEBUG K3B_DEBUG)
macro_bool_to_01(MUSICBRAINZ_FOUND HAVE_MUSICBRAINZ)

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)


check_function_exists(lrint HAVE_LRINT)
check_function_exists(lrintf HAVE_LRINTF)
