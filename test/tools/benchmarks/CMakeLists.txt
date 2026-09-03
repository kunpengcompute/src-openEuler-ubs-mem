add_executable(ubsm_perf_test ubsm_perf_test.cpp)
TARGET_SECUREC_SHARE(ubsm_perf_test)
target_link_libraries(ubsm_perf_test PRIVATE ubsm_sdk_shared)
target_include_directories(ubsm_perf_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ubsm_perf_test PUBLIC ${PROJECT_SOURCE_DIR}/src/app_lib/include)
target_include_directories(ubsm_perf_test PUBLIC ${PROJECT_SOURCE_DIR}/src/common/hdagger)
target_include_directories(ubsm_perf_test PUBLIC ${PROJECT_SOURCE_DIR}/src/ptracer/include)
target_compile_options(ubsm_perf_test PRIVATE -std=c++17)
target_compile_definitions(ubsm_perf_test PRIVATE ENABLE_PTRACER)
target_link_options(ubsm_perf_test PRIVATE -pie -rdynamic -Wl,--allow-multiple-definition)
target_compile_options(ubsm_perf_test PRIVATE -w)
install(TARGETS ubsm_perf_test DESTINATION ${MATRIX_MEM_TEST_BINARY_DIR}
        PERMISSIONS ${DAEMON_INSTALL_EXE_PERMISSIONS})

add_executable(ubsm_race_test ubsm_race_test.cpp)
target_include_directories(ubsm_race_test PUBLIC ${PROJECT_SOURCE_DIR}/src/app_lib/include)
target_link_libraries(ubsm_race_test PUBLIC ubsm_sdk_shared pthread)
TARGET_SECUREC_SHARE(ubsm_race_test)
install(TARGETS ubsm_race_test DESTINATION ${MATRIX_MEM_TEST_BINARY_DIR}
        PERMISSIONS ${DAEMON_INSTALL_EXE_PERMISSIONS})

# 创建共享库
add_library(decrypt SHARED decrypt_example.cpp)

# 设置输出名称为libdecrypt.so
set_target_properties(decrypt PROPERTIES
        OUTPUT_NAME "decrypt"
        PREFIX "lib"
        SUFFIX ".so"
)

# 安装规则
install(TARGETS decrypt
        LIBRARY DESTINATION lib
)