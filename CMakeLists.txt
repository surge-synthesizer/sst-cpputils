cmake_minimum_required(VERSION 3.10)
project(sst-cpputils VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE
            /utf-8 /bigobj /diagnostics:color /permissive-
            /Zc:__cplusplus,enumTypes,templateScope,throwingNew,preprocessor)
endif ()

get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else ()
    set(is_toplevel 0)
endif ()
option(SST_CPPUTILS_BUILD_TESTS "Add targets for building and running sst-cpputils tests" ${is_toplevel})

if (SST_CPPUTILS_BUILD_TESTS)
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
