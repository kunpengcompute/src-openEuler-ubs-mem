set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${UBS_MEMORY_BINARY_DIR}/output/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UBS_MEMORY_BINARY_DIR}/output/bin)
set(CMAKE_CONFIG_OUTPUT_DIRECTORY ${UBS_MEMORY_BINARY_DIR}/output/config)
set(CMAKE_SCRIPT_OUTPUT_DIRECTORY ${UBS_MEMORY_BINARY_DIR}/output/script)
set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${UBS_MEMORY_BINARY_DIR}/output/include)

# 只编译到ubsmd
add_library(ubs_mem_daemon_object OBJECT)
# 同时编译到sdk和ubsmd
add_library(ubs_mem_common_object OBJECT)
# 只编译到ubsmd
add_library(ubs_mem_sdk_object OBJECT)
target_link_libraries(ubs_mem_common_object PUBLIC ubs_mem_common_compile)
target_link_libraries(ubs_mem_daemon_object PUBLIC ubs_mem_common_object)
target_link_libraries(ubs_mem_sdk_object PUBLIC ubs_mem_common_object)
TARGET_SECUREC_SHARE(ubs_mem_common_object)
TARGET_HCOM_HEADER_ONLY(ubs_mem_common_object)

target_include_directories(ubs_mem_common_object PUBLIC ${CMAKE_CURRENT_LIST_DIR})
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} UBS_MEM_SRCS)
target_sources(ubs_mem_common_object PRIVATE ${UBS_MEM_SRCS})
unset(UBS_MEM_SRCS)
TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/common/hcom_adapter
        ${CMAKE_CURRENT_SOURCE_DIR}/include/common
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/communication
        ${CMAKE_CURRENT_SOURCE_DIR}/include/communication/ipc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/communication/rpc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ulog
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/include/uc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ulog
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/app_lib/include
        ${CMAKE_CURRENT_SOURCE_DIR}/app_lib/common
        ${PROJECT_SOURCE_DIR}/src/process/common
        ${PROJECT_SOURCE_DIR}/3rdparty/dlock/
        ${PROJECT_SOURCE_DIR}/3rdparty/ubse/include
)

set(HDAGGER_INSTALL_DIR ${PROJECT_SOURCE_DIR}/src/common/hdagger/)
target_include_directories(ubs_mem_common_object SYSTEM PUBLIC ${HDAGGER_INSTALL_DIR})

target_include_directories(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub)
target_include_directories(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub/linux)

add_subdirectory(under_api)
add_subdirectory(security)
add_subdirectory(logger)
add_subdirectory(ulog)
add_subdirectory(process)
add_subdirectory(communication)
add_subdirectory(mxm_message)
add_subdirectory(mxm_shm)
add_subdirectory(mxm_lease)
add_subdirectory(app_lib)
add_subdirectory(dlock_utils)
add_subdirectory(store)
add_subdirectory(zen_discovery)
add_subdirectory(ptracer)