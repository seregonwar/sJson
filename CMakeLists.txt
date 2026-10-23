cmake_minimum_required(VERSION 3.16)
project(sJson VERSION 2.0.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

option(SJSON_BUILD_TESTS "Build sJson tests" ON)
option(SJSON_BUILD_BENCHMARKS "Build sJson benchmarks" ON)
option(SJSON_BUILD_FUZZERS "Build libFuzzer-compatible fuzz targets" OFF)
option(SJSON_BUILD_COMPAT "Build optional compatibility benchmark integrations" ON)
option(SJSON_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(SJSON_BUILD_FFI "Build the opaque C ABI for language bindings" ON)
option(SJSON_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries (needed by Python ctypes)" OFF)
if(SJSON_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(sjson INTERFACE)
add_library(sJson::sjson ALIAS sjson)
target_include_directories(sjson INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sjson>)

add_library(sjson_cpp INTERFACE)
target_link_libraries(sjson_cpp INTERFACE sjson)
target_include_directories(sjson_cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings/cpp>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sjson/cpp>)
target_compile_features(sjson_cpp INTERFACE cxx_std_17)
add_library(sJson::sjson_cpp ALIAS sjson_cpp)

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

if(SJSON_BUILD_FFI)
    add_library(sjson_ffi bindings/ffi/sjson_ffi.c)
    target_link_libraries(sjson_ffi PRIVATE sjson)
    target_include_directories(sjson_ffi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings/ffi>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sjson/ffi>)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(sjson_ffi PUBLIC SJSON_FFI_SHARED PRIVATE SJSON_FFI_BUILD)
    endif()
    sjson_apply_warnings(sjson_ffi)
    if(NOT MSVC)
        target_link_libraries(sjson_ffi PRIVATE m)
    endif()
    set_target_properties(sjson_ffi PROPERTIES VERSION 1.0.0 SOVERSION 1)
    add_library(sJson::sjson_ffi ALIAS sjson_ffi)
endif()

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

    add_executable(test_reliability tests/test_reliability.c)
    target_link_libraries(test_reliability PRIVATE sjson)
    sjson_apply_warnings(test_reliability)
    sjson_link_math(test_reliability)
    add_test(NAME sjson_reliability COMMAND test_reliability)

    add_executable(test_limits tests/test_limits.c)
    target_link_libraries(test_limits PRIVATE sjson)
    sjson_apply_warnings(test_limits)
    sjson_link_math(test_limits)
    add_test(NAME sjson_limits COMMAND test_limits)

    add_executable(test_compat tests/test_compat.c)
    target_link_libraries(test_compat PRIVATE sjson)
    target_include_directories(test_compat PRIVATE compat/cjson)
    sjson_apply_warnings(test_compat)
    sjson_link_math(test_compat)
    add_test(NAME sjson_compat COMMAND test_compat)

    include(CheckLanguage)
    check_language(CXX)
    if(CMAKE_CXX_COMPILER)
        enable_language(CXX)
        add_executable(test_cpp tests/test_cpp.cpp)
        target_link_libraries(test_cpp PRIVATE sjson_cpp)
        sjson_apply_warnings(test_cpp)
        sjson_link_math(test_cpp)
        add_test(NAME sjson_cpp COMMAND test_cpp)
    endif()

    if(SJSON_BUILD_FFI)
        add_executable(test_ffi tests/test_ffi.c)
        target_link_libraries(test_ffi PRIVATE sjson_ffi)
        sjson_apply_warnings(test_ffi)
        add_test(NAME sjson_ffi COMMAND test_ffi)
        if(BUILD_SHARED_LIBS)
            find_package(Python3 COMPONENTS Interpreter QUIET)
            if(Python3_Interpreter_FOUND)
                add_test(NAME sjson_python COMMAND ${Python3_EXECUTABLE}
                    -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/bindings/python/tests -v)
                set_tests_properties(sjson_python PROPERTIES ENVIRONMENT
                    "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/bindings/python;SJSON_LIBRARY=$<TARGET_FILE:sjson_ffi>")
            endif()
        endif()
    endif()

    add_executable(json_corpus_runner tests/json_corpus_runner.c)
    target_link_libraries(json_corpus_runner PRIVATE sjson)
    sjson_apply_warnings(json_corpus_runner)
    sjson_link_math(json_corpus_runner)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/JSONTestSuite/test_parsing")
        add_test(NAME sjson_corpus COMMAND json_corpus_runner
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/JSONTestSuite/test_parsing")
    endif()
    if(CMAKE_CXX_COMPILER AND NOT SJSON_SANITIZERS)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(NAME sjson_install COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_install.py
                ${CMAKE_COMMAND} ${CMAKE_CTEST_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR} $<CONFIG> "${CMAKE_GENERATOR}"
                "${CMAKE_C_COMPILER}" "${CMAKE_CXX_COMPILER}")
        endif()
    endif()
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
    if(SJSON_BUILD_TESTS)
        add_test(NAME sjson_fuzz_smoke COMMAND fuzz_parse)
    endif()
endif()

install(TARGETS sjson sjson_cpp EXPORT sJsonTargets)
if(SJSON_BUILD_FFI)
    install(TARGETS sjson_ffi EXPORT sJsonTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES bindings/ffi/sjson_ffi.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sjson/ffi)
endif()
install(FILES LICENSE-MIT LICENSE-GPL DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/sjson)
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
