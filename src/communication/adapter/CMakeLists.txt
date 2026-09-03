aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} COM_ADAPT_SRCS)

add_library(ubs_mem_comm_sdk_object OBJECT)
target_sources(ubs_mem_comm_sdk_object PRIVATE ${COM_ADAPT_SRCS})
target_include_directories(ubs_mem_comm_sdk_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                                          ${CMAKE_SOURCE_DIR}/src
                                                          ${CMAKE_SOURCE_DIR}/src/process/common
                                                          ${CMAKE_SOURCE_DIR}/src/common/hdagger
                                                          ${CMAKE_SOURCE_DIR}/src/common
                                                          ${CMAKE_SOURCE_DIR}/src/app_lib/include
                                                          ${CMAKE_SOURCE_DIR}/src/include/ulog
                                                          ${CMAKE_SOURCE_DIR}/src/security/cryptor
                                                          ${CMAKE_SOURCE_DIR}/src/common/hdagger/logger
                                                          ${CMAKE_SOURCE_DIR}/src/mxm_message
                                                          ${CMAKE_SOURCE_DIR}/src/include
                                                          ${CMAKE_SOURCE_DIR}/src/ptracer/include)
target_compile_definitions(ubs_mem_comm_sdk_object PRIVATE CLIENT_LOG)
TARGET_HCOM_HEADER_ONLY(ubs_mem_comm_sdk_object)

add_library(ubs_mem_comm_daemon_object OBJECT)
target_sources(ubs_mem_comm_daemon_object PRIVATE ${COM_ADAPT_SRCS})
target_include_directories(ubs_mem_comm_daemon_object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                             ${CMAKE_SOURCE_DIR}/src
                                                             ${CMAKE_SOURCE_DIR}/src/process/common
                                                             ${CMAKE_SOURCE_DIR}/src/common/hdagger
                                                             ${CMAKE_SOURCE_DIR}/src/common
                                                             ${CMAKE_SOURCE_DIR}/src/app_lib/include
                                                             ${CMAKE_SOURCE_DIR}/src/include/ulog
                                                             ${CMAKE_SOURCE_DIR}/src/security/cryptor
                                                             ${CMAKE_SOURCE_DIR}/src/common/hdagger/logger
                                                             ${CMAKE_SOURCE_DIR}/src/mxm_message
                                                             ${CMAKE_SOURCE_DIR}/src/include
                                                             ${CMAKE_SOURCE_DIR}/src/ptracer/include)
TARGET_HCOM_HEADER_ONLY(ubs_mem_comm_daemon_object)
