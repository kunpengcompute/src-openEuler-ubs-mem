aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} IPC_CLIENT_SRC)
target_sources(ubs_mem_sdk_object PRIVATE ${IPC_CLIENT_SRC})
target_include_directories(ubs_mem_sdk_object PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/communication/adapter)