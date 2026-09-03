IF (NOT DEFINED CMAKE_BUILD_TYPE OR x${CMAKE_BUILD_TYPE} STREQUAL "x")
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF ()

EXECUTE_PROCESS(COMMAND g++ -dumpversion
        TIMEOUT 5
        OUTPUT_VARIABLE cPlusPlusVerString
        OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[(0-9)]*" cPlusPlusVerNum ${cPlusPlusVerString})

EXECUTE_PROCESS(COMMAND cmake --version
        TIMEOUT 5
        OUTPUT_VARIABLE cMakeVerString
        OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "cmake version " "" cMakeVerNumString ${cMakeVerString})
string(REGEX MATCH "[(0-9).]*" cMakeVerNum ${cMakeVerNumString})

set(MATRIX_MEM_CXX_STANDARD "-std=c++17")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(HUAWEI_SECURE_BUILD_FLAGS "-fstack-protector-strong -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now  -fPIE -fPIC -rdynamic -g -ggdb3")
else ()
    SET(HUAWEI_SECURE_BUILD_FLAGS "-fstack-protector-strong -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now  -fPIE -fPIC -rdynamic -ftrapv")
endif ()
SET(C_COMMON_FLAGS "${C_COMMON_FLAGS}  -fno-omit-frame-pointer ${HUAWEI_SECURE_BUILD_FLAGS}")
SET(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} ${MATRIX_MEM_CXX_STANDARD}  -fno-omit-frame-pointer ${HUAWEI_SECURE_BUILD_FLAGS}")

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
SET(CMAKE_C_FLAGS_DEBUG "${C_COMMON_FLAGS} -O0 -ggdb3")
SET(CMAKE_C_FLAGS_RELEASE "${C_COMMON_FLAGS} -O2 -D_FORTIFY_SOURCE=2 -DNDEBUG=1 -s")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${C_COMMON_FLAGS} -O2 -g -D_FORTIFY_SOURCE=2 -DNDEBUG=1")

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "${CXX_COMMON_FLAGS} -O0 -ggdb3")
SET(CMAKE_CXX_FLAGS_RELEASE "${CXX_COMMON_FLAGS} -O2 -D_FORTIFY_SOURCE=2 -DNDEBUG=1 -s")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CXX_COMMON_FLAGS} -O2 -g -D_FORTIFY_SOURCE=2 -DNDEBUG=1")

if (DEBUG_UT STREQUAL "ON" OR DEBUG_FUZZ STREQUAL "ON")
    set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage ")
    set(CMAKE_C_FLAGS " ${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage ")
    add_compile_definitions(DEBUG_MEM_UT)
    add_link_options(-lgcov --coverage)
    set(CMAKE_EXE_LINKER_FLAGS " -lgcov --coverage ")
    set(CMAKE_SHARED_LINKER_FLAGS " -lgcov --coverage ")
endif ()

if (ASAN_BUILD STREQUAL "ON")
    if (NOT DEBUG_UT STREQUAL "ON" AND NOT DEBUG_FUZZ STREQUAL "ON")
        message(WARNING "ASAN_BUILD is ON.")
        set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -fsanitize=address ")
        set(CMAKE_C_FLAGS " ${CMAKE_CXX_FLAGS} -fsanitize=address ")
    endif ()
    add_link_options(-fsanitize=address)
    set(CMAKE_EXE_LINKER_FLAGS " -fsanitize=address ")
    set(CMAKE_SHARED_LINKER_FLAGS " -fsanitize=address  ")
endif ()

set(BUILD_MODE ${CMAKE_BUILD_TYPE})
if (${BUILD_MODE} MATCHES "Release")
    set(CXX_FLAGS
            -O3
            -fPIE
            -g
            -w
            -fms-extensions
            -fno-omit-frame-pointer
            -fstack-protector-all
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
    )
else ()
    set(CXX_FLAGS
            -g
            -w
            -fPIE
            -fms-extensions
    )
endif (${BUILD_MODE} MATCHES "Release")
add_library(ubs_mem_common_compile INTERFACE)
IF (NOT (DEBUG_UT STREQUAL "ON"))
    # -Wpedantic 过于严格
    target_compile_options(ubs_mem_common_compile INTERFACE
            $<$<CONFIG:Debug,Release,RelWithDebInfo>:
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-sign-compare
            -Wno-missing-field-initializers
            -Wno-cast-qual
            -Winvalid-pch
            -Wcast-align
            -Wwrite-strings
            -Wfloat-equal
            -Wno-error=cast-qual
            >
    )
    # 灵活数组
    target_compile_options(ubs_mem_common_compile INTERFACE -Wno-array-bounds -Wno-error=array-bounds)
    target_compile_options(ubs_mem_common_compile INTERFACE -Wno-c++20-extensions)
    target_compile_options(ubs_mem_common_compile INTERFACE -Wno-pmf-conversions)
endif ()

IF (UBSE_SDK)
    message(WARNING "Compile the code with UBSE_SDK")
    add_compile_definitions(UBSE_SDK)
ENDIF ()

if(NOT DEFINED ARCH_LIB_DIR)
    set(ARCH_LIB_DIR "/usr/lib64")
endif()
if(NOT DEFINED SYS_LIB_DIR)
    set(SYS_LIB_DIR "/usr/lib64")
endif()
add_compile_definitions(ARCH_LIB_DIR="${ARCH_LIB_DIR}")
add_compile_definitions(SYS_LIB_DIR="${SYS_LIB_DIR}")