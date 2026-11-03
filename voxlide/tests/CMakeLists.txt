add_executable(voxlide_tests ../../voxlib/tests/test_main.cpp test_mode26.cpp test_mode6.cpp test_shrink.cpp test_moments.cpp test_histogram.cpp test_remap.cpp test_threshold01.cpp test_replace_range.cpp test_zero_grad.cpp test_mag_gradient.cpp test_scale_gradient.cpp test_segment2.cpp test_device_image.cpp test_median_family.cpp test_pore_ops.cpp test_pointwise.cpp test_paint.cpp test_chains.cpp test_median.cpp test_smooth_bilateral.cpp test_bilateral_x.cpp test_bilateral_gauss.cpp)
target_link_libraries(voxlide_tests PRIVATE voxlide OpenMP::OpenMP_CXX)
set_target_properties(voxlide_tests PROPERTIES CXX_STANDARD 23)
add_test(NAME voxlide_tests COMMAND voxlide_tests)

# Throughput table (parallelism_shared.md §9); not part of ctest.
add_executable(voxlide_bench voxlide_bench.cpp)
target_link_libraries(voxlide_bench PRIVATE voxlide OpenMP::OpenMP_CXX)
set_target_properties(voxlide_bench PROPERTIES CXX_STANDARD 23)
