aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} RPC_SERVER_SRC)
target_sources(ubs_mem_daemon_object PRIVATE ${RPC_SERVER_SRC})
target_include_directories(ubs_mem_daemon_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/communication/adapter)