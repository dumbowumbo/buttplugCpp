cmake_minimum_required(VERSION 3.12)
project(ButtplugClient VERSION 0.1.0 LANGUAGES CXX)

# Set C++11 as minimum standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUTTPLUG_BUILD_EXAMPLES "Build example applications" ON)
option(BUTTPLUG_DEBUG "Enable debug output" ON)

# Library sources
set(BUTTPLUG_SOURCES
    src/buttplugclient.cpp
    src/log.cpp
    src/messageHandler.cpp
    src/messages.cpp
)

set(BUTTPLUG_HEADERS
    include/buttplugclient.h
    include/log.h
    include/messageHandler.h
    include/messages.h
    include/helperClasses.h
)

# Create library
add_library(buttplugclient ${BUTTPLUG_SOURCES})

# Add include directory
target_include_directories(buttplugclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Enable debug mode if requested
if(BUTTPLUG_DEBUG)
    target_compile_definitions(buttplugclient PRIVATE BUTTPLUG_DEBUG)
endif()

# Find dependencies
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ixwebsocket REQUIRED)

# Link dependencies
if(MSVC)
    target_link_libraries(buttplugclient
        PUBLIC
            nlohmann_json::nlohmann_json
            ixwebsocket::ixwebsocket
	    Threads::Threads
            bcrypt
    )
else()
    target_link_libraries(buttplugclient
        PUBLIC
            nlohmann_json::nlohmann_json
            ixwebsocket::ixwebsocket
	    Threads::Threads
    )
endif()

# Compiler-specific settings
if(MSVC)
    # Windows/MSVC settings
    target_compile_options(buttplugclient PRIVATE /W4)
else()
    # Linux/GCC/Clang settings
    target_compile_options(buttplugclient PRIVATE -Wall -Wextra)
endif()

# Build examples if requested
if(BUTTPLUG_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# Installation configuration
include(GNUInstallDirs)

# Install the library and headers
install(TARGETS buttplugclient
    EXPORT ButtplugClientTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${BUTTPLUG_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/buttplug
)

# Install CMake config files
install(EXPORT ButtplugClientTargets
    FILE ButtplugClientTargets.cmake
    NAMESPACE ButtplugClient::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ButtplugClient
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ButtplugClientConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ButtplugClientConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ButtplugClient
)

# Create and install version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ButtplugClientConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ButtplugClientConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ButtplugClientConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ButtplugClient
)

# Add uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
