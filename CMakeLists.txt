cmake_minimum_required(VERSION 3.25)
project(cross_platform_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR
    "This project is verified with clang only (got '${CMAKE_CXX_COMPILER_ID}'). "
    "Set CXX=clang++ (or use the Visual Studio ClangCL toolset on Windows).")
endif()

add_executable(demo src/main.cpp)

# clang-cl uses MSVC-style flags, clang/clang++ use GNU-style flags.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  target_compile_options(demo PRIVATE /W4 /WX)
else()
  target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME demo COMMAND demo)
