aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MXM_LEASE_SRCS)
target_sources(ubs_mem_sdk_object PRIVATE ${MXM_LEASE_SRCS})
target_include_directories(ubs_mem_sdk_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
unset(MXM_LEASE_SRCS)
