set(SOURCES
    ./metatype.cpp
    ./variant.cpp
    ./metaitem.cpp
    ./metadefine.cpp
)

set(HEADERS
    ./global.h
    ./c_string.h
    ./argument.h
    ./metatype.h
    ./metatype_p.h
    ./metaitem.h
    ./metadefine.h
    ./metaerror.h
    ./rtti.h
)

add_library(rtti SHARED ${SOURCES} ${HEADERS})
target_compile_options(rtti PRIVATE -fvisibility=hidden)
target_compile_definitions(rtti PRIVATE -DBUILD_SHARED -DBUILD_DLL)
set_property(TARGET rtti PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_property(TARGET rtti PROPERTY RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_property(TARGET rtti PROPERTY LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_library(rtti_static STATIC ${SOURCES} ${HEADERS})
set_property(TARGET rtti_static PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
