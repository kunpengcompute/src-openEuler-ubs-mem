aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
target_sources(ubs_mem_daemon_object PRIVATE ${SRC_LIST})
target_include_directories(ubs_mem_daemon_object PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/zen_discovery
)
target_include_directories(ubs_mem_daemon_object SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/3rdparty/spdlog/include
        ${PROJECT_SOURCE_DIR}/3rdparty/dlock
)