# CMakeList.txt : CMake project for buttplugCpp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add option for debug output
option(BUTTPLUG_DEBUG "Enable debug output" ON)

# Define a preprocessor macro if debug is enabled
if(BUTTPLUG_DEBUG)
    add_definitions(-DBUTTPLUG_DEBUG)
endif()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("buttplugCpp")

# Add source to this project's executable.
add_executable (buttplugCpp "example/buttplugCpp.cpp" "example/buttplugCpp.h")

file(GLOB SRC_FILES    
    "src/*.cpp"
    "include/*.h"
)

target_sources(buttplugCpp PUBLIC
    ${SRC_FILES}
)

find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(buttplugCpp PUBLIC ixwebsocket::ixwebsocket nlohmann_json::nlohmann_json Threads::Threads)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET buttplugCpp PROPERTY CXX_STANDARD 11)
endif()
# TODO: Add tests and install targets if needed.
