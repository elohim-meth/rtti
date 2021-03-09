find_package(doctest REQUIRED)

add_executable(doctest_tests
    doctest_main.cpp
    register_global_rtti.cpp
    register_std_rtti.cpp
)
target_sources(doctest_tests PRIVATE
    test_type_name.cpp
    test_meta_type.cpp
    test_global_ns.cpp
    test_std_ns.cpp
    test_single_inheritance.cpp
    test_multiple_inheritance.cpp
    test_virtual_inheritance.cpp
    test_variant.cpp)

target_link_libraries(doctest_tests PRIVATE doctest::doctest RTTI::rtti)

set_target_properties(doctest_tests PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Integrate Catch test in CTest
include(doctest)
doctest_discover_tests(doctest_tests)

# Strip binary for release builds
if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET doctest_tests POST_BUILD
        COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:doctest_tests>")
endif()

# Add target to run tests automatically
add_custom_target(check ALL
    COMMAND doctest_tests)
