# create version
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${PROJECT_BINARY_DIR}/include/rtti/version.h @ONLY)

set(PKG rtti)
set(NAMESPACE RTTI)

# define target
add_library(${PKG})

# define namespace alias
add_library(${NAMESPACE}::${PKG} ALIAS ${PKG})

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
    message(STATUS "${PKG} library type: SHARED")
else()
    set_target_properties(${PKG} PROPERTIES
        OUTPUT_NAME "${PKG}-static"
    )
    message(STATUS "${PKG} library type: STATIC")
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

file(GLOB PUBLIC_HEADERS
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/rtti/*.h"
)

file(GLOB IMPL_HEADERS
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/rtti/detail/*.h"
)

file(GLOB PRIVATE_HEADERS
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/private/*.h"
)

file(GLOB SOURCES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_include_directories(${PKG}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/private)

target_sources(${PKG} PRIVATE ${PUBLIC_HEADERS} ${IMPL_HEADERS} ${PRIVATE_HEADERS} ${SOURCES})

# Strip binary for release builds
if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET ${PKG} POST_BUILD
        COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:${PKG}>")
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CONFIG_NAME}.cmake.in
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

