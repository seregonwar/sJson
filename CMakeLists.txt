cmake_minimum_required(VERSION 3.10)
project(sJson VERSION 1.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

option(SJSON_BUILD_TESTS "Build sJson tests" ON)
option(SJSON_BUILD_BENCHMARKS "Build sJson benchmarks" ON)
option(SJSON_BUILD_FUZZERS "Build libFuzzer-compatible fuzz targets" OFF)
option(SJSON_BUILD_COMPAT "Build optional compatibility benchmark integrations" ON)
option(SJSON_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_library(sjson INTERFACE)
target_include_directories(sjson INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sjson>)

function(sjson_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(SJSON_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
        if(SJSON_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(sjson_link_math target)
    if(NOT MSVC)
        target_link_libraries(${target} PRIVATE m)
    endif()
endfunction()

if(SJSON_BUILD_TESTS)
    enable_testing()

    add_executable(test_json src/test_json.c)
    target_link_libraries(test_json PRIVATE sjson)
    sjson_apply_warnings(test_json)
    sjson_link_math(test_json)
    add_test(NAME sjson_regression COMMAND test_json)

    add_executable(json_corpus_runner tests/json_corpus_runner.c)
    target_link_libraries(json_corpus_runner PRIVATE sjson)
    sjson_apply_warnings(json_corpus_runner)
    sjson_link_math(json_corpus_runner)
endif()

if(SJSON_BUILD_BENCHMARKS)
    add_executable(bench_sjson benchmarks/bench_sjson.c)
    target_link_libraries(bench_sjson PRIVATE sjson)
    target_compile_definitions(bench_sjson PRIVATE NDEBUG)
    sjson_apply_warnings(bench_sjson)
    sjson_link_math(bench_sjson)
endif()

if(SJSON_BUILD_COMPAT AND SJSON_BUILD_BENCHMARKS)
    find_path(CJSON_INCLUDE_DIR cjson/cJSON.h)
    find_library(CJSON_LIBRARY cjson)
    if(CJSON_INCLUDE_DIR AND CJSON_LIBRARY)
        add_executable(bench_compare benchmarks/bench_compare.c)
        target_link_libraries(bench_compare PRIVATE sjson ${CJSON_LIBRARY})
        target_include_directories(bench_compare PRIVATE ${CJSON_INCLUDE_DIR})
        target_compile_definitions(bench_compare PRIVATE NDEBUG SJSON_BENCH_WITH_CJSON)
        sjson_apply_warnings(bench_compare)
        sjson_link_math(bench_compare)

        add_executable(bench_assets benchmarks/bench_assets.c)
        target_link_libraries(bench_assets PRIVATE sjson ${CJSON_LIBRARY})
        target_include_directories(bench_assets PRIVATE ${CJSON_INCLUDE_DIR})
        target_compile_definitions(bench_assets PRIVATE NDEBUG SJSON_BENCH_WITH_CJSON)
        sjson_apply_warnings(bench_assets)
        sjson_link_math(bench_assets)
    endif()
endif()

if(SJSON_BUILD_FUZZERS)
    add_executable(fuzz_parse fuzz/fuzz_parse.c)
    target_link_libraries(fuzz_parse PRIVATE sjson)
    sjson_apply_warnings(fuzz_parse)
    sjson_link_math(fuzz_parse)
endif()

install(TARGETS sjson EXPORT sJsonTargets)
install(FILES src/json_pal.h src/sJson.c DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sjson)
install(DIRECTORY bindings/cpp/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sjson/cpp FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY compat/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sjson/compat FILES_MATCHING PATTERN "*.h")

install(EXPORT sJsonTargets
    FILE sJsonTargets.cmake
    NAMESPACE sJson::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sJson)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sJsonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sJsonConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sJson)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sJsonConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sJsonConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sJsonConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sJson)
