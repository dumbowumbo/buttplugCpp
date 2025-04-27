# Example application
add_executable(buttplugCpp buttplugCpp.cpp)

# Link with the main library
target_link_libraries(buttplugCpp PRIVATE buttplugclient)

# The installation section has been removed so the example will be built but not installed