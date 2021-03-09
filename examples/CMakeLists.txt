add_executable(tutorial_01 tutorial_01.cpp)

target_link_libraries(tutorial_01 PRIVATE RTTI::rtti)

set_target_properties(tutorial_01 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Strip binary for release builds
if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET tutorial_01 POST_BUILD
        COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:tutorial_01>")
endif()
