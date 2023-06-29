# buttplugCpp
## How to use
I made this project in Visual Studio 2022, but it uses CMake. I currently am struggling to get a proper library CMake going, so simply include the src and include directories in your project and add them with:
```
file(GLOB SRC_FILES    
    "src/*.cpp"
    "include/*.h"
)

target_sources(buttplugCpp PUBLIC
    ${SRC_FILES}
)
```
in your CMake file. Also make sure to build IXWebSockets (without zlib, unless you want to deal with getting it working with CMake). Nlohmann Json dependency is handled by Hunter, so make sure you have Hunter in your project too. I provide an example CMake file which makes the example program in example directory.

## More info
This is currently for Windows only C++20, although the port to Linux is not too difficult and C++ will also be changed to C++11. This library and its documentation is still WIP.
