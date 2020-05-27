find_package(Catch2 REQUIRED)

add_executable(test_global_ns test.cpp)
target_link_libraries(test_global_ns Catch2::Catch2 RTTI::rtti)

set_target_properties(test_global_ns PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX "d"
)

include(Catch)
catch_discover_tests(test_global_ns)

#add_test(NAME test_global_ns COMMAND $<TARGET_FILE:${TESTNAME}>)
#add_test(NAME test_global_ns COMMAND $<TARGET_FILE:test_global_ns>)
#set_tests_properties(test_global_ns PROPERTIES ENVIRONMENT "PATH=$ENV{PATH}")

install(
    TARGETS test_global_ns
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
