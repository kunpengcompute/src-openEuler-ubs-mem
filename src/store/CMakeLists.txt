aux_source_directory(${CMAKE_CURRENT_LIST_DIR} RECORD_STORE_SRCS)
target_sources(ubs_mem_daemon_object PRIVATE ${RECORD_STORE_SRCS})
target_compile_options(ubs_mem_daemon_object PRIVATE -fPIC)
target_include_directories(ubs_mem_daemon_object PUBLIC ${CMAKE_CURRENT_LIST_DIR})