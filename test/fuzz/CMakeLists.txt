message(WARNING "in fuzz dir...")
message(WARNING "PLUGIN_VERSION is ON.")
if (NOT DEFINED ASCEND_TOOLKIT_PATH)
    set(ASCEND_TOOLKIT_PATH "/usr/local/Ascend/ascend-toolkit/latest" CACHE STRING "")
endif ()
include_directories(${ASCEND_TOOLKIT_PATH}/include)
link_directories(
        ${ASCEND_TOOLKIT_PATH}/lib64
)

set(ROOT_DIR ${CMAKE_SOURCE_DIR})
set(FUZZ_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 检查SecodeFuzz
set(SECODEFUZZ_INSTALL_DIR "${ROOT_DIR}/test/secodefuzz")

if (NOT EXISTS ${SECODEFUZZ_INSTALL_DIR})
    message(WARNING "SECODEFUZZ_INSTALL_DIR(${SECODEFUZZ_INSTALL_DIR}) is invalid, trying to git clone secodefuzz...")
    execute_process(
            COMMAND /bin/bash ${ROOT_DIR}/test/fuzz/secode_fuzz.sh
            RESULT_VARIABLE result
            OUTPUT_VARIABLE output
            ERROR_VARIABLE error
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Script execution failed: ${error}")
    else()
        message(STATUS "Script output: ${output}")
    endif()
endif()

include_directories(${SECODEFUZZ_INSTALL_DIR}/include)
link_directories(${SECODEFUZZ_INSTALL_DIR}/lib)

message(STATUS "fuzz build")

if (DEBUG_FUZZ STREQUAL "ON")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../conf/comp.cmake")
    message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/../conf/comp.cmake")
    set(COMPILE_FUZZ_CODE ON)
    add_compile_definitions(DEBUG_MEM_FUZZ)
else ()
    add_compile_definitions(DEBUG_MEM_FUZZ)
    IF (EXISTS "${PROJECT_SOURCE_DIR}/test/conf/comp.cmake")
        include("${CMAKE_CURRENT_SOURCE_DIR}/../conf/comp.cmake")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/../conf/comp.cmake")
    ELSE ()
        message(STATUS "${PROJECT_SOURCE_DIR}/test/conf/comp.cmake not exist.")
        set(COMPILE_FUZZ_CODE OFF)
    ENDIF ()
endif ()

include_directories(
        ${CMAKE_SOURCE_DIR}/src/app_lib/
        ${CMAKE_SOURCE_DIR}/src/app_lib/common
        ${CMAKE_SOURCE_DIR}/src/app_lib/include
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_lease_lib
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_lease_lib/borrow_app_meta_mgr
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_lease_lib/ipc_message_sender
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_shm_lib
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_shm_lib/ipc_message_sender
        ${CMAKE_SOURCE_DIR}/src/app_lib/mxm_shm_lib/shm_meta_data_mgr
        ${CMAKE_SOURCE_DIR}/src/common/
        ${CMAKE_SOURCE_DIR}/src/common/hcom_adapter
        ${CMAKE_SOURCE_DIR}/src/htracer/
        ${CMAKE_SOURCE_DIR}/src/htracer/common
        ${CMAKE_SOURCE_DIR}/src/htracer/manager
        ${CMAKE_SOURCE_DIR}/src/htracer/service
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/include/common
        ${CMAKE_SOURCE_DIR}/src/include/ipc
        ${CMAKE_SOURCE_DIR}/src/include/service
        ${CMAKE_SOURCE_DIR}/src/include/uc
        ${CMAKE_SOURCE_DIR}/src/include/ulog
        ${CMAKE_SOURCE_DIR}/src/include/utils
        ${CMAKE_SOURCE_DIR}/src/communication/adapter
        ${CMAKE_SOURCE_DIR}/src/communication/ipc/client
        ${CMAKE_SOURCE_DIR}/src/communication/ipc/server
        ${CMAKE_SOURCE_DIR}/src/communication/rpc/server
        ${CMAKE_SOURCE_DIR}/src/mxm_lease/
        ${CMAKE_SOURCE_DIR}/src/mxm_message/
        ${CMAKE_SOURCE_DIR}/src/mxm_shm/
        ${CMAKE_SOURCE_DIR}/src/process/common/util
        ${CMAKE_SOURCE_DIR}/src/process/common
        ${CMAKE_SOURCE_DIR}/src/process/daemon
        ${CMAKE_SOURCE_DIR}/src/dlock_utils
        ${CMAKE_SOURCE_DIR}/src/store
        ${CMAKE_SOURCE_DIR}/test/mxm-unit-tests
        ${CMAKE_SOURCE_DIR}/test/mxm-unit-tests/rack_com_stub
        ${CMAKE_SOURCE_DIR}/test/mxm-unit-tests/under_api)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub/linux)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/dlock)

IF (COMPILE_FUZZ_CODE)
    # 添加测试源代码
    aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/shmem/ MXM_MEM_TEST_SRC)
    aux_source_directory(${CMAKE_SOURCE_DIR}/test/mxm-unit-tests/under_api MXM_MEM_TEST_SRC)

    message(WARNING "compile mxmd_fuzz...")
    # 生成目标
    add_executable(mxmd_fuzz ${MXM_MEM_TEST_SRC} fuzz_main.cpp)
    target_include_directories(mxmd_fuzz PUBLIC ${CMAKE_SOURCE_DIR}/src/app_lib/)
    target_include_directories(mxmd_fuzz PUBLIC ${CMAKE_SOURCE_DIR}/test/mxm-unit-tests/under_api)

    TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/test/mxm-unit-tests/rack_com_stub/)
    TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/test/mxm-unit-tests/under_api/)

    target_link_libraries(mxmd_fuzz PUBLIC ubs_mem_daemon_object)
    target_link_libraries(mxmd_fuzz PUBLIC ubs_mem_sdk_object)
    target_link_libraries(mxmd_fuzz PUBLIC ubs_mem_common_object)
    target_link_libraries(mxmd_fuzz PUBLIC Secodefuzz)
    target_link_libraries(mxmd_fuzz PUBLIC
            -Wl,--start-group
            numa
            -Wl,--end-group)
    target_link_options(mxmd_fuzz PRIVATE -Wl,--allow-multiple-definition)
    install(TARGETS mxmd_fuzz DESTINATION ${MEM_RUNTIME_DESTINATION}
            PERMISSIONS ${MEM_INSTALL_EXE_PERMISSIONS})
ENDIF ()
