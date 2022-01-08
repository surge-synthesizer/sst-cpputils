cmake_minimum_required(VERSION 3.10)
project(sst-cpputils VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

get_directory_property(parent_dir PARENT_DIRECTORY)
if("${parent_dir}" STREQUAL "")
    set(is_toplevel 1)
else()
    set(is_toplevel 0)
endif()
option(SST_CPPUTILS_BUILD_TESTS "Add targets for building and running sst-cpputils tests" ${is_toplevel})

if(SST_CPPUTILS_BUILD_TESTS)
    add_executable(sst-cpputils-tests)
    target_include_directories(sst-cpputils-tests PRIVATE tests)
    target_link_libraries(sst-cpputils-tests PRIVATE ${PROJECT_NAME})
    target_sources(sst-cpputils-tests PRIVATE
            tests/tests.cpp)
endif()
