cmake_minimum_required(VERSION 3.10)
project(json_pal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

# Header-only — test executable is the only build artifact
add_executable(test_json src/test_json.c)
target_link_libraries(test_json m)

enable_testing()
add_test(NAME json_pal_suite COMMAND test_json)
