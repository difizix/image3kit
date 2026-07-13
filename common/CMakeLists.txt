add_library(sirun_utils STATIC SiRun.cpp)
target_include_directories(sirun_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# clangd/vscode mess:
target_link_libraries(sirun_utils PRIVATE pybind11::headers Python::Module)
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../voxlib")
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/stb")
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/svplot")
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim")
target_include_directories(sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/zlib")
target_compile_features(sirun_utils PUBLIC cxx_std_23)

target_include_directories(
  sirun_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim/primitives")
target_compile_definitions(sirun_utils PRIVATE VMMLIB)

add_executable(check_sirun test_sirun.cpp)
target_link_libraries(check_sirun PRIVATE sirun_utils)

add_executable(check_input test_input.cpp)
target_link_libraries(check_input PRIVATE sirun_utils)
