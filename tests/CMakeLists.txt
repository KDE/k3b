
include_directories(
    ${CMAKE_SOURCE_DIR}/libk3b/core
)

kde4_add_unit_test( k3btest k3bglobalstest.cpp )
target_link_libraries( k3btest ${QT_QTTEST_LIBRARY} k3b )
