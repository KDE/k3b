macro_optional_find_package(OggVorbis)

include_directories(../libk3b/plugin/ ../libk3b/core/)
add_subdirectory( decoder )
add_subdirectory( encoder )
add_subdirectory( project )
