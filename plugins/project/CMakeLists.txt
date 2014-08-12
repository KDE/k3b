include_directories(
    ../../libk3b/projects/audiocd/
    ../../libk3b/projects/datacd/
    ../../libk3b/projects/
    ../../libk3b/tools/
)

add_subdirectory( audioprojectcddb )

if(ENABLE_TAGLIB)
    add_subdirectory(audiometainforenamer)
endif()

