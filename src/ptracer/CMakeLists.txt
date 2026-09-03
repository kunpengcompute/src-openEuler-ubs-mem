target_include_directories(ubs_mem_common_object PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tracers
)

aux_source_directory(. PTRACER_SRCS)
aux_source_directory(tracers PTRACER_SRCS)
target_sources(ubs_mem_common_object PRIVATE ${PTRACER_SRCS})
if (ENABLE_PTRACER STREQUAL "ON")
    target_compile_definitions(ubs_mem_common_object PUBLIC ENABLE_PTRACER)
endif ()