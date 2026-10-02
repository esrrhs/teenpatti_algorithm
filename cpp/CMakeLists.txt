cmake_minimum_required(VERSION 3.16)
project(teenpatti_algorithm_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8)
else()
  add_compile_options(-Wall -Wextra)
endif()

option(TEENPATTI_EMBED_DATA "Embed teenpatti_data.txt into the library (GNU/Clang)" ON)

add_library(teenpatti src/teenpatti.cpp)
target_include_directories(teenpatti PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(TEENPATTI_EMBED_DATA AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_definitions(teenpatti PRIVATE
    "TEENPATTI_EMBEDDED_DATA_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/teenpatti_data.txt\"")
endif()

add_executable(teenpatti_gen src/gen_main.cpp)
target_link_libraries(teenpatti_gen PRIVATE teenpatti)

enable_testing()
add_executable(teenpatti_test tests/teenpatti_test.cpp)
target_link_libraries(teenpatti_test PRIVATE teenpatti)
target_compile_definitions(teenpatti_test PRIVATE
  "TEENPATTI_TEST_DATA_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/teenpatti_data.txt\"")
add_test(NAME teenpatti_test COMMAND teenpatti_test)

# Copy the data file into the build directory so builds without the embedded
# data still find it from the working directory (ctest runs there).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/teenpatti_data.txt
               ${CMAKE_CURRENT_BINARY_DIR}/teenpatti_data.txt COPYONLY)
