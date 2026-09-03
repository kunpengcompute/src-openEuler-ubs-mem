function(split_debug_symbols target)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        return()
    endif ()
    string(TIMESTAMP BUILD_TIMESTAMP "%Y%m%d")
    message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")
    # 定义符号文件路径（与二进制同目录，添加.debug后缀）
    set(debug_file "$<TARGET_FILE:${target}>.debug.${BUILD_TIMESTAMP}")

    add_custom_command(TARGET ${target} POST_BUILD
            # 1. 提取完整调试符号到单独文件
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug "$<TARGET_FILE:${target}>" "${debug_file}"
            # 2. 从主二进制中剥离调试符号但保留最小关联信息
            COMMAND ${CMAKE_OBJCOPY} --strip-debug "$<TARGET_FILE:${target}>"
            # 为可执行文件添加符号表文件的链接
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${debug_file} "$<TARGET_FILE:${target}>"
            COMMENT "Splitting debug symbols for ${target} into ${debug_file}"
    )
endfunction()
