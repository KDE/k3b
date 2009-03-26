include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(CheckStructMember)
include(CheckCXXSourceCompiles)

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

check_include_files(sys/vfs.h    HAVE_SYS_VFS_H)
check_include_files(sys/statvfs.h HAVE_SYS_STATVFS_H)

check_function_exists(stat64    HAVE_STAT64)
