find_package(Catch2 REQUIRED)

add_executable(catch_tests
    catch_test_main.cpp
    register_rtti.cpp
)
target_sources(catch_tests PRIVATE test_global_ns.cpp)
target_link_libraries(catch_tests Catch2::Catch2 RTTI::rtti)

set_target_properties(catch_tests PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Integrate Catch test in CTest
include(Catch)
catch_discover_tests(catch_tests)

# Add target to run tests
add_custom_target(check ALL COMMAND catch_tests)
