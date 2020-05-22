project(RTTI VERSION 1.0.0 LANGUAGES CXX)

# create version
configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/version.h.in
    ${PROJECT_BINARY_DIR}/include/rtti/version.h @ONLY)

# define target
string(TOLOWER ${PROJECT_NAME} PKG)
add_library(${PKG})

# define namespace alias
add_library(${PROJECT_NAME}::${PKG} ALIAS ${PKG})

# set C++ standard
target_compile_features(${PKG} PUBLIC cxx_std_17)
set_target_properties(${PKG} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX "d"
)

# Turn on all warnings in DEBUG
target_compile_options(${PKG} PRIVATE $<$<CONFIG:Debug>:-Werror -Wall -Wextra>)
# Turn off default rtti
target_compile_options(${PKG} PRIVATE -fno-rtti)

# set shared library version and default code visibility
if (BUILD_SHARED_LIBS)
    set_target_properties(${PKG} PROPERTIES
        VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    message(STATUS "rtti library type: SHARED")
else()
    set_target_properties(${PKG} PROPERTIES
        OUTPUT_NAME "${PKG}-static"
    )
    message(STATUS "rtti library type: STATIC")
endif()

# set up directory structure
set_target_properties(${PKG} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# create import/export
include(GenerateExportHeader)
generate_export_header(${PKG}
    BASE_NAME RTTI
    EXPORT_MACRO_NAME RTTI_API
    NO_EXPORT_MACRO_NAME RTTI_PRIVATE
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/rtti/export.h
    STATIC_DEFINE RTTI_STATIC
)
if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PKG} PRIVATE -DRTTI_STATIC)
endif()

set(SOURCES
    argument.cpp
    metatype.cpp
    variant.cpp
    metaitem.cpp
    metacontainer.cpp
    metanamespace.cpp
    metaclass.cpp
    metaconstructor.cpp
    metamethod.cpp
    metaproperty.cpp
    metaenum.cpp
    metadefine.cpp
)

set(HEADERS
    include/rtti/export.h
    include/rtti/defines.h
    include/rtti/finally.h
    include/rtti/sfinae.h
    include/rtti/function_traits.h
    include/rtti/misc_traits.h
    include/rtti/signature.h
    include/rtti/tagged_id.h
    include/rtti/typename.h
    include/rtti/argument.h
    include/rtti/variant.h
    include/rtti/metaerror.h
    include/rtti/metatype.h
    metatype_p.h
    include/rtti/metaitem.h
    metaitem_p.h
    include/rtti/metacontainer.h
    metacontainer_p.h
    include/rtti/metanamespace.h
    metanamespace_p.h
    include/rtti/metaclass.h
    metaclass_p.h
    include/rtti/metamethod.h
    metamethod_p.h
    include/rtti/metaenum.h
    metaenum_p.h
    include/rtti/metaconstructor.h
    metaconstructor_p.h
    include/rtti/metaproperty.h
    metaproperty_p.h
    include/rtti/metadefine.h
)

target_include_directories(${PKG}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src)

target_sources(${PKG} PRIVATE ${SOURCES} ${HEADERS})

########################################################################################################################
## INSTALL && EXPORT
########################################################################################################################

include(CMakePackageConfigHelpers)

if (BUILD_SHARED_LIBS)
    set(PKG_NAME ${PKG})
else()
    set(PKG_NAME "${PKG}-static")
endif()

set(CONFIG_NAME ${PKG_NAME}-config)
set(TARGET_NAME ${PKG_NAME}-target)
set(PROJECT_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_NAME}-${PROJECT_VERSION}")

# Generate config and version
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/${CONFIG_NAME}.cmake.in
    ${PROJECT_BINARY_DIR}/cmake/${CONFIG_NAME}.cmake
    INSTALL_DESTINATION ${PROJECT_INSTALL_CMAKE_DIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/cmake/${CONFIG_NAME}-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config and version
install(
    FILES
        ${PROJECT_BINARY_DIR}/cmake/${CONFIG_NAME}.cmake
        ${PROJECT_BINARY_DIR}/cmake/${CONFIG_NAME}-version.cmake
    DESTINATION ${PROJECT_INSTALL_CMAKE_DIR}
)

# Install binary target and generate export target
install(
    TARGETS ${PKG}
    EXPORT ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install export target
install(
    EXPORT ${TARGET_NAME}
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${PROJECT_INSTALL_CMAKE_DIR}
)

# Install public include headers
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/rtti
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY ${PROJECT_BINARY_DIR}/include/rtti
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

