add_subdirectory(ubse_sdk)
include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/../build/include
        ${PROJECT_SOURCE_DIR}/src/common/hdagger
        ${CMAKE_SOURCE_DIR}/src
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
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/include/common
        ${CMAKE_SOURCE_DIR}/src/include/communication
        ${CMAKE_SOURCE_DIR}/src/include/communication/ipc
        ${CMAKE_SOURCE_DIR}/src/include/communication/rpc
        ${CMAKE_SOURCE_DIR}/src/include/communication/adapter
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
        ${CMAKE_SOURCE_DIR}/src/under_api/ubse
        ${CMAKE_SOURCE_DIR}/src/under_api/openssl
        ${CMAKE_SOURCE_DIR}/src/zen_discovery
)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/obmm_ub/linux)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/dlock)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

# 添加测试源代码
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/app_testcase/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/app_testcase/common/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/app_testcase/mxm_lease_lib MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/app_testcase/mxm_shm_lib MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/ulog/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/security/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/util/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/zen_discovery/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/mxm_lease/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/daemon/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/dlock_utils/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/common/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/common/hdagger/ MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/mxm_shm MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/communication/rpc MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/communication/adapter MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/store MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/mxm_lease MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/mxm_message MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/communication/ipc MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/under_api MXM_MEM_TEST_SRC)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/monitor MXM_MEM_TEST_SRC)

TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/test/mxm-unit-tests/rack_com_stub/)
TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/test/mxm-unit-tests/under_api/)
TARGET_INCLUDE_DIRECTORIES(ubs_mem_common_object PUBLIC ${PROJECT_SOURCE_DIR}/src/mxm_shm)
target_sources(ubs_mem_common_object PRIVATE ${CMAKE_CURRENT_LIST_DIR}/under_api/ubse_mem_adapter_stub.cpp)

aux_source_directory(${CMAKE_SOURCE_DIR}/src/process/daemon MXM_MEM_TEST_SRC)
list(REMOVE_ITEM MXM_MEM_TEST_SRC ${CMAKE_SOURCE_DIR}/src/process/daemon/main.cpp)

message(WARNING "compile mxmd_ut...")
# 生成目标
add_executable(mxmd_ut ${MXM_MEM_TEST_SRC} new_gtest_main.cpp)
target_include_directories(mxmd_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app_testcase)

target_link_libraries(mxmd_ut PUBLIC ubs_mem_daemon_object ubs_mem_comm_daemon_object)
target_link_libraries(mxmd_ut PUBLIC ubs_mem_sdk_object)
target_link_libraries(mxmd_ut PUBLIC ubs_mem_common_object)
target_link_libraries(mxmd_ut PUBLIC
        ${CMAKE_INSTALL_PREFIX}/lib64/libgmock.a
        ${CMAKE_INSTALL_PREFIX}/lib64/libgmock_main.a
        ${CMAKE_INSTALL_PREFIX}/lib64/libgtest.a
        ${CMAKE_INSTALL_PREFIX}/lib64/libgtest_main.a
        ${CMAKE_INSTALL_PREFIX}/lib/libmockcpp.a
)
target_compile_options(mxmd_ut PRIVATE -Wno-array-bounds)
target_compile_options(mxmd_ut PRIVATE -Wno-cast-qual)
target_compile_options(mxmd_ut PRIVATE -Wno-pmf-conversions)
target_link_libraries(mxmd_ut PUBLIC
        -Wl,--start-group
        numa
        -Wl,--end-group)
target_link_options(mxmd_ut PRIVATE -Wl,--allow-multiple-definition)
install(TARGETS mxmd_ut DESTINATION ${MEM_RUNTIME_DESTINATION}
        PERMISSIONS ${MEM_INSTALL_EXE_PERMISSIONS})
