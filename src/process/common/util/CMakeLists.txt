
target_include_directories(ubs_mem_common_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ubs_mem_common_object PUBLIC ${CMAKE_SOURCE_DIR}/src/common/hdagger/lock)

file(GLOB UTILS_SRCS *.cc *.cpp *.h)
target_sources(ubs_mem_common_object PRIVATE ${UTILS_SRCS})
find_library(NUMA_LIBRARY numa)
target_link_libraries(ubs_mem_common_object PUBLIC systemd
                      ${NUMA_LIBRARY})


TARGET_LINK_LIBRARIES(ubs_mem_daemon_object PUBLIC
        -Wl,--start-group
        pthread dl
        -L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        -Wl,--end-group
)