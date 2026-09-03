aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MXM_COMMON_SRC)
target_sources(ubs_mem_sdk_object PRIVATE ${MXM_COMMON_SRC})
target_include_directories(ubs_mem_sdk_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ubs_mem_sdk_object PUBLIC ${PROJECT_SOURCE_DIR}/src/common/hdagger/logger)
target_compile_definitions(ubs_mem_sdk_object PRIVATE CLIENT_LOG)
######################################################################
TARGET_HCOM_HEADER_ONLY(ubs_mem_sdk_object)
TARGET_OBMM_HEADER_ONLY(ubs_mem_sdk_object)
######################################################################

# include gen headers
target_include_directories(ubs_mem_sdk_object SYSTEM PUBLIC ${PROJECT_BINARY_DIR}/include)

add_subdirectory(mxm_lease_lib)
add_subdirectory(mxm_shm_lib)
target_compile_definitions(ubs_mem_sdk_object PRIVATE UBS_MEM_SDK_OBJECT)
add_library(ubsm_sdk_shared SHARED)
target_link_libraries(ubsm_sdk_shared PUBLIC ubs_mem_sdk_object ubs_mem_comm_sdk_object)
target_link_libraries(ubsm_sdk_shared PUBLIC ubs_mem_common_object)
set_target_properties(ubsm_sdk_shared PROPERTIES OUTPUT_NAME "ubsm_sdk")

# 明确指定 libnuma 的库路径
find_library(NUMA_LIB numa PATHS /usr/lib64 /usr/local/lib/)

if(NUMA_LIB)
    target_link_libraries(ubsm_sdk_shared PUBLIC ${NUMA_LIB})
else()
    message(FATAL_ERROR "libnuma.so not found")
endif()

split_debug_symbols(ubsm_sdk_shared)
######################################################################
TARGET_SECUREC_SHARE(ubsm_sdk_shared)
TARGET_HCOM(ubsm_sdk_shared)
#target_link_options(ubsm_sdk_shared PRIVATE -Wl,--allow-multiple-definition)
######################################################################
install(TARGETS ubsm_sdk_shared DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        PERMISSIONS ${DEAMON_INSTALL_LIB_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)