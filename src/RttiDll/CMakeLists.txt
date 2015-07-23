set(SOURCES
    ./argument.cpp
    ./metatype.cpp
    ./variant.cpp
    ./metaitem.cpp
    ./metacontainer.cpp
    ./metanamespace.cpp
    ./metaclass.cpp
    ./metaconstructor.cpp
    ./metamethod.cpp
    ./metaenum.cpp
    ./metadefine.cpp
)

set(HEADERS
    ./global.h
    ./signature.h
    ./c_string.h
    ./argument.h
    ./metatype.h
    ./metatype_p.h
    ./variant.h
    ./metaitem.h
    ./metaitem_p.h
    ./metacontainer.h
    ./metacontainer_p.h
    ./metanamespace.h
    ./metanamespace_p.h
    ./metaclass.h
    ./metaclass_p.h
    ./metaconstructor.h
    ./metaconstructor_p.h
    ./metamethod.h
    ./metamethod_p.h
    ./metaenum.h
    ./metaenum_p.h
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
