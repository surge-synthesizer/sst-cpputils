cmake_minimum_required(VERSION 3.10)
project(sst-cpputils VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COLOR_DIAGNOSTICS ON)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE
            /utf-8 /bigobj /permissive-)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        message(STATUS "Skipping __cplusplus etc for clang-cl")
    else ()
        message(STATUS "Including __cplusplus etc for MSVC-not-clang")
        target_compile_options(${PROJECT_NAME} INTERFACE
                /Zc:__cplusplus,enumTypes,templateScope,throwingNew,preprocessor)
    endif ()
endif ()

option(SST_CPPUTILS_BUILD_TESTS "Add targets for building and running sst-cpputils tests" ${PROJECT_IS_TOP_LEVEL})

if (SST_CPPUTILS_BUILD_TESTS)

    # Allow sanitizer builds
    if (${PROJECT_IS_TOP_LEVEL})
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            add_compile_options(
                    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
                    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
            )

            add_link_options(
                    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
                    $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
            )
        endif ()
    endif ()
    add_executable(sst-cpputils-tests)
    target_include_directories(sst-cpputils-tests PRIVATE tests)
    target_link_libraries(sst-cpputils-tests PRIVATE ${PROJECT_NAME})
    target_include_directories(sst-cpputils-tests PRIVATE libs/catch2)
    target_sources(sst-cpputils-tests PRIVATE
            tests/tests.cpp)

    add_custom_command(TARGET sst-cpputils-tests
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E echo "Copying $<TARGET_FILE:sst-cpputils-tests> to test-binary"
            COMMAND ${CMAKE_COMMAND} -E make_directory test-binary
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:sst-cpputils-tests>" test-binary)
endif ()
