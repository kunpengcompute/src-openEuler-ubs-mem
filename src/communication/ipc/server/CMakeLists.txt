aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} IPC_SERVER_SRC)
target_sources(ubs_mem_daemon_object PRIVATE ${IPC_SERVER_SRC})
target_include_directories(ubs_mem_daemon_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
unset(IPC_SERVER_SRC)