aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LOGGER_SRCS)
target_sources(ubs_mem_common_object PRIVATE ${LOGGER_SRCS})
target_include_directories(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/src)
