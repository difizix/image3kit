add_executable(voxlide_tests ../../voxlib/tests/test_main.cpp test_mode26.cpp test_median.cpp test_smooth_bilateral.cpp)
target_link_libraries(voxlide_tests PRIVATE voxlide OpenMP::OpenMP_CXX)
set_target_properties(voxlide_tests PROPERTIES CXX_STANDARD 23)
add_test(NAME voxlide_tests COMMAND voxlide_tests)

# Throughput table (parallelism_shared.md §9); not part of ctest.
add_executable(voxlide_bench voxlide_bench.cpp)
target_link_libraries(voxlide_bench PRIVATE voxlide OpenMP::OpenMP_CXX)
set_target_properties(voxlide_bench PROPERTIES CXX_STANDARD 23)
