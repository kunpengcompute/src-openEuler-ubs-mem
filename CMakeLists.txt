cmake_minimum_required(VERSION 3.13)
project(UBS_MEMORY VERSION 1.0 LANGUAGES CXX C)

SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/output)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_EXE_LINKER_FLAGS "-pie")
SET(CMAKE_SKIP_RPATH TRUE)

option(ENABLE_PTRACER "build ptracer" ON)
message(STATUS "ENABLE_PTRACER = ${ENABLE_PTRACER}")
# 编译general
if (NOT DEFINED BUILD_GENERAL_COMPUTING)
    # 针对跑DT时通过环境变量传递出包类型的方式，读取变量值
    set(BUILD_GENERAL_COMPUTING $ENV{BUILD_GENERAL_COMPUTING})
    if (NOT BUILD_GENERAL_COMPUTING)
        set(BUILD_GENERAL_COMPUTING OFF)
    else ()
        set(BUILD_GENERAL_COMPUTING ON)
    endif ()
endif ()

#################################
# include common cmake modules
#################################
include("cmake/building.cmake")
include("cmake/utils.cmake")
include("cmake/deps.cmake")
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

if (NOT DEFINED CI_BUILDING)
    set(CI_BUILDING OFF)
endif ()

#################################
# build 3rd party dependencies
#################################
add_subdirectory(3rdparty)
#################################
# build src
#################################
add_subdirectory(src)
#################################
# build test
#################################
set(MATRIX_MEM_TEST_BINARY_DIR ${UBS_MEMORY_BINARY_DIR}/output_test/bin)
set(MATRIX_MEM_TEST_LIBRARY_DIR ${UBS_MEMORY_BINARY_DIR}/output_test/lib)
set(MATRIX_MEM_TEST_SCRIPT_DIR ${UBS_MEMORY_BINARY_DIR}/output_test/script)
add_subdirectory(test)