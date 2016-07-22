set(SOURCES
    ./test_dll.cpp
    ./test_register.cpp
    ./test_variant.cpp
    ./test_cast.cpp
    ./rtti_std_vector.cpp
    ./rtti_std_string.cpp
    ./rtti_std_unique_ptr.cpp
    ./rtti_test_namespace.cpp
)

set(HEADERS
    ./test_dll.h
    ./test_register.h
    ./test_variant.h
    ./test_cast.h
)

add_executable(TestDll ${SOURCES} ${HEADERS})
target_include_directories(TestDll PRIVATE "../include")
target_compile_definitions(TestDll PRIVATE -DBUILD_SHARED)
#target_compile_options(TestDll PRIVATE "-Wa,-mbig-obj")
target_link_libraries (TestDll  rtti)
set_property(TARGET TestDll PROPERTY RUNTIME_OUTPUT_DIRECTORY ${BIN_DIRECTORY})

target_compile_definitions(TestDll PRIVATE $<$<CONFIG:Debug>:DEBUG>)
