if (DEBUG_UT STREQUAL "ON" OR DEBUG_FUZZ STREQUAL "ON")
    target_include_directories(ubs_mem_daemon_object PUBLIC ${PROJECT_SOURCE_DIR}/test/mxm-unit-tests/rack_com_stub)
endif ()

target_include_directories(ubs_mem_daemon_object PUBLIC ${CMAKE_CURRENT_LIST_DIR})
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} UBS_MEM_SRCS)
target_sources(ubs_mem_daemon_object PRIVATE ${UBS_MEM_SRCS})
unset(UBS_MEM_SRCS)