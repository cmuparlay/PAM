add_executable(range_test rt_test.cpp)
add_executable(range_sweep rs_test.cpp)
target_link_libraries(range_test PRIVATE pam)
target_link_libraries(range_sweep PRIVATE pam)
