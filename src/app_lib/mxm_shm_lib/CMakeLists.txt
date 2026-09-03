aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MXM_SHM_SRCS)
target_sources(ubs_mem_sdk_object PRIVATE ${MXM_SHM_SRCS})
target_include_directories(ubs_mem_sdk_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
unset(MXM_SHM_SRCS)

add_subdirectory(shm_meta_data_mgr)
add_subdirectory(ipc_message_sender)