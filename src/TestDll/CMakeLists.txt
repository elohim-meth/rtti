set(SOURCES
    ./test_dll.cpp
    ./test_register.cpp
)

set(HEADERS
    ./test_dll.h
    ./test_register.h
)

add_executable(TestDll ${SOURCES} ${HEADERS})
target_compile_definitions(TestDll PRIVATE -DBUILD_SHARED)
target_link_libraries (TestDll  rtti)
set_property(TARGET TestDll PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_compile_definitions(TestDll PRIVATE $<$<CONFIG:Debug>:DEBUG>)
