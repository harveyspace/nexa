# Minimal tests — no external dependencies
add_executable(test_runner test_runner.cpp)
target_include_directories(test_runner PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/core/pool
)
target_link_libraries(test_runner PRIVATE nexa_core nexa_engine)
add_test(NAME nexatest COMMAND test_runner)
