aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} UBS_MEM_SRCS)
target_sources(ubs_mem_daemon_object PRIVATE ${UBS_MEM_SRCS})
target_include_directories(ubs_mem_daemon_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
unset(UBS_MEM_SRCS)