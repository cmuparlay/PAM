add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE pam)
add_test(unit_tests unit_tests)
add_executable(testParallel testParallel.cpp)
target_link_libraries(testParallel PRIVATE pam)

