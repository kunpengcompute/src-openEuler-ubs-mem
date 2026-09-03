# Build gtest
message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
execute_process(COMMAND /usr/bin/cmake -S ${CMAKE_CURRENT_SOURCE_DIR}/googletest -B ${CMAKE_CURRENT_BINARY_DIR}/googletest)
execute_process(COMMAND /usr/bin/cmake --build ${CMAKE_CURRENT_BINARY_DIR}/googletest)
execute_process(COMMAND /usr/bin/cmake --install  ${CMAKE_CURRENT_BINARY_DIR}/googletest --prefix ${CMAKE_INSTALL_PREFIX})

# Build mockcpp
execute_process(COMMAND /usr/bin/cmake -S ${CMAKE_CURRENT_SOURCE_DIR}/mockcpp -B ${CMAKE_CURRENT_BINARY_DIR}/mockcpp -DCMAKE_INSTALL_PREFIX=${DCMAKE_INSTALL_PREFIX})
execute_process(COMMAND /usr/bin/cmake --build ${CMAKE_CURRENT_BINARY_DIR}/mockcpp)
execute_process(COMMAND /usr/bin/cmake --install  ${CMAKE_CURRENT_BINARY_DIR}/mockcpp --prefix ${CMAKE_INSTALL_PREFIX})