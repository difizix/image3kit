add_executable(voxlib_tests test_main.cpp test_voxelImgUtils.cpp test_otsu.cpp)

target_link_libraries(voxlib_tests PRIVATE voxlib)

target_include_directories(
  voxlib_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
                       ${CMAKE_CURRENT_SOURCE_DIR}/../../common)

add_test(NAME voxlib_tests COMMAND voxlib_tests)
