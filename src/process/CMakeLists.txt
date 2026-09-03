# 安全编译选项
#set(DATA_KIT_LINK_FLAGS -Wl,-z,relro,-z,now,-z,noexecstack -s)
#add_compile_options(-D_FORTIFY_SOURCE=2)

# unit test
# mark gcov (don't delete the line, enable unit test will use)
# upper line will be replace to add_definitions(-DGCOV_ENABLE) when unit test enable
#if (DEFINED GCOV_ENABLE)
#    message("will add gcov information")
#    add_compile_options(-ftest-coverage -fprofile-arcs)
#    link_libraries(gcov)
#endif (DEFINED GCOV_ENABLE)
include(${PROJECT_SOURCE_DIR}/cmake/package.cmake)
add_compile_options(-fPIC)


string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

if (NOT DEFINED ${LIBRARY_OUTPUT_PATH})
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
endif (NOT DEFINED ${LIBRARY_OUTPUT_PATH})
if (NOT DEFINED ${EXECUTABLE_OUTPUT_PATH})
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif (NOT DEFINED ${EXECUTABLE_OUTPUT_PATH})

target_compile_definitions(ubs_mem_common_object PUBLIC OCK_VERSION="23.0.0")

if (NOT DEFINED ${OCK_3RDPARTY_BINARY_DIR})
    set(OCK_3RDPARTY_BINARY_DIR ${CMAKE_SOURCE_DIR}/output/ock-3rdparty)
endif (NOT DEFINED ${OCK_3RDPARTY_BINARY_DIR})

target_compile_definitions(ubs_mem_common_object PUBLIC THREADED)
target_compile_definitions(ubs_mem_common_object PUBLIC HAVE_CYRUS_SASL_H)

if (NOT DEFINED RELEASE_BUILD)
    target_compile_definitions(ubs_mem_common_object PUBLIC BUILD_RELEASE="88888")
    set(ENV{BUILD_RELEASE} "88888")
else ()
    target_compile_definitions(ubs_mem_common_object PUBLIC BUILD_RELEASE="${RELEASE_BUILD}")
    set(ENV{BUILD_RELEASE} "${RELEASE_BUILD}")
endif (NOT DEFINED RELEASE_BUILD)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message("-- system is ${CMAKE_SYSTEM_NAME}")
else ()
    message(FATAL_ERROR "don't support ${CMAKE_SYSTEM_NAME}")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")
    target_compile_definitions(ubs_mem_common_object PUBLIC __AARCH64__)
    set(CMAKE_ARC linux-aarch64)
elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
    target_compile_definitions(ubs_mem_common_object PUBLIC __X86_64__)
    set(CMAKE_ARC linux-x86_64)
else ()
    message(FATAL_ERROR "don't support ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")

message("CMAKE_ARC is ${CMAKE_ARC}")

add_subdirectory(common)
add_subdirectory(daemon)
add_subdirectory(monitor)

# install config files.
install(FILES ${CMAKE_SOURCE_DIR}/script/ubsmd.conf DESTINATION ${CMAKE_CONFIG_OUTPUT_DIRECTORY}
        PERMISSIONS ${DAEMON_INSTALL_DOC_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)
install(FILES ${CMAKE_SOURCE_DIR}/script/systemd/ubsmd.service DESTINATION ${CMAKE_SCRIPT_OUTPUT_DIRECTORY}
        PERMISSIONS ${DAEMON_INSTALL_DOC_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)
install(FILES ${CMAKE_SOURCE_DIR}/src/app_lib/include/ubs_mem.h DESTINATION ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
        PERMISSIONS ${DAEMON_INSTALL_DOC_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)
install(FILES ${CMAKE_SOURCE_DIR}/src/app_lib/include/ubs_mem_def.h DESTINATION ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
        PERMISSIONS ${DAEMON_INSTALL_DOC_PERMISSIONS}
        COMPONENT OCK_DAEMON_COMPONENT)