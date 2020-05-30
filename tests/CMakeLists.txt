﻿find_package(DocTest REQUIRED)

add_executable(doctest_tests
    doctest_main.cpp
    register_rtti.cpp
)
target_sources(doctest_tests PRIVATE test_global_ns.cpp)
target_link_libraries(doctest_tests PRIVATE doctest::doctest RTTI::rtti)

set_target_properties(doctest_tests PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Integrate Catch test in CTest
include(doctest)
doctest_discover_tests(doctest_tests)

# Add target to run tests automatically
add_custom_target(check ALL
    COMMAND doctest_tests)