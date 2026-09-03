# so
file(GLOB_RECURSE DAEMON_SO_SRCS ock*.cpp)
add_library(ubsmd_shared SHARED)
set_target_properties(ubsmd_shared PROPERTIES OUTPUT_NAME "ubsmd")
target_sources(ubs_mem_daemon_object PRIVATE ${DAEMON_SO_SRCS})
target_link_libraries(ubsmd_shared PUBLIC ubs_mem_daemon_object ubs_mem_comm_daemon_object)
target_link_libraries(ubsmd_shared PUBLIC ubs_mem_common_object)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ASAN" OR CMAKE_BUILD_TYPE STREQUAL "TSAN")
    if (NOT DEBUG_UT STREQUAL "ON")
        target_compile_definitions(ubs_mem_daemon_object PUBLIC _DEBUG)
    endif ()
endif ()

target_include_directories(ubs_mem_daemon_object PUBLIC ../../dlock_utils)
TARGET_LINK_LIBRARIES(ubs_mem_daemon_object PUBLIC pthread)
TARGET_HCOM(ubs_mem_daemon_object)
TARGET_SECUREC_SHARE(ubs_mem_daemon_object)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
target_link_options(ubsmd_shared PRIVATE -Wl,--allow-multiple-definition)
######################################################################
install(TARGETS ubsmd_shared DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PERMISSIONS ${DEAMON_INSTALL_LIB_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)
# exe main.cpp
add_executable(ubsmd_bin main.cpp)
target_link_libraries(ubsmd_bin PUBLIC ubsmd_shared)
target_link_libraries(ubsmd_bin PUBLIC ubs_mem_daemon_object)
set_target_properties(ubsmd_bin PROPERTIES OUTPUT_NAME "ubsmd")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
target_link_options(ubsmd_bin
        PRIVATE
        -pie
        -rdynamic
        -fstack-protector-strong
)
# 符号拆分
split_debug_symbols(ubsmd_shared)
split_debug_symbols(ubsmd_bin)