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
#target_compile_options(TestDll PRIVATE "-Wa,-mbig-obj")
target_link_libraries (TestDll RTTI::rtti)

target_compile_definitions(TestDll PRIVATE $<$<CONFIG:Debug>:DEBUG>)

install(
    TARGETS TestDll
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
