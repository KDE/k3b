find_library(LIBFUZZER_LIBRARIES NAMES libFuzzer.a)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibFuzzer DEFAULT_MSG LIBFUZZER_LIBRARIES)
 
if(LIBFUZZER_LIBRARIES)
    set(LIBFUZZER_FOUND TRUE)
endif()

MARK_AS_ADVANCED(
    LIBFUZZER_LIBRARIES
)
