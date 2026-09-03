aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MESSAGE_SRCS)
target_sources(ubs_mem_common_object PRIVATE ${MESSAGE_SRCS})
target_include_directories(ubs_mem_common_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
