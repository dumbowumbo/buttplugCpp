# Buttplug C++ Client Library

A C++ client implementation for the Buttplug intimate hardware control protocol.

## Overview

This library provides a C++ client for the Buttplug Server, allowing developers to connect to and control various intimate hardware devices. The implementation supports C++11 and higher with minimal external dependencies.

Key features:
- WebSocket-based communication with Buttplug servers
- Thread-safe device control
- Support for various device commands (vibration, linear, rotation)
- Support for sensor reading and subscription
- Built-in logging capabilities

## Dependencies

The library has the following dependencies:
- [IXWebSocket](https://github.com/machinezone/IXWebSocket) - WebSocket client
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- C++11 compatible compiler

## Building

### Linux (Ubuntu)

1. Install the required dependencies:

```bash
# Install build tools
sudo apt-get update
sudo apt-get install build-essential cmake git

# Install nlohmann/json (via package manager if available)
sudo apt-get install nlohmann-json3-dev

# Or manually (if not available in package manager)
git clone https://github.com/nlohmann/json.git
cd json
mkdir build && cd build
cmake ..
make
sudo make install
cd ../..
```

2. Build IXWebSocket:

```bash
git clone https://github.com/machinezone/IXWebSocket.git
cd IXWebSocket
mkdir build && cd build
cmake ..
make
sudo make install
cd ../..
```

3. Clone and build this repository:

```bash
git clone https://github.com/yourusername/buttplug-cpp-client.git
cd buttplug-cpp-client
mkdir build && cd build
cmake ..
make
```

4. Install the library system-wide (optional):

```bash
sudo make install
```

This will install:
- Library files to `/usr/local/lib` (or as configured)
- Headers to `/usr/local/include/buttplug`
- CMake configuration files for dependent projects

5. To uninstall the library (if needed):

```bash
sudo make uninstall
```

### Windows

#### Using Visual Studio and vcpkg

1. Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ development tools

2. Install [vcpkg](https://github.com/microsoft/vcpkg):

```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

3. Install dependencies via vcpkg:

```cmd
vcpkg install nlohmann-json:x64-windows
vcpkg install ixwebsocket:x64-windows
vcpkg integrate install
```

4. Clone this repository and open it in Visual Studio:

```cmd
git clone https://github.com/yourusername/buttplug-cpp-client.git
cd buttplug-cpp-client
```

5. Create a new CMake project in Visual Studio, or run the following from Developer Command Prompt:

```cmd
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path\to\vcpkg]\scripts\buildsystems\vcpkg.cmake
```

6. Build the project:

```cmd
cmake --build . --config Release
```

7. Install the library (optional, run as Administrator):

```cmd
cmake --install . --config Release
```

This will install:
- Library files to the system library directory 
- Headers to the system include directory
- CMake configuration files for dependent projects

8. To uninstall the library (if needed, run as Administrator):

```cmd
cmake -P cmake_uninstall.cmake
```

## Usage

### Basic Example

```cpp
#include <buttplug/buttplugclient.h>
#include <iostream>
#include <thread>
#include <chrono>

// Callback for handling device messages
void messageHandler(const mhl::Messages& msg) {
    if (msg.messageType == mhl::MessageTypes::DeviceAdded) {
        std::cout << "New device added: " << msg.deviceAdded.device.DeviceName << std::endl;
    }
}

int main() {
    // Create client connection to a buttplug server at localhost:12345
    Client client("ws://localhost", 12345);
    
    // Connect to the server
    client.connect(messageHandler);
    
    // Start scanning for devices
    client.startScan();
    
    // Wait for devices to connect
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Get list of connected devices
    auto devices = client.getDevices();
    
    if (!devices.empty()) {
        // Control the first device (simple vibration at 50% power)
        client.sendScalar(devices[0], 0.5);
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Stop the device
        client.stopDevice(devices[0]);
    }
    
    return 0;
}
```

### Using as a Dependency in CMake Projects

After installing the library, you can easily use it in your CMake projects:

```cmake
find_package(ButtplugClient REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ButtplugClient::buttplugclient)
```

## Documentation

The Buttplug protocol documentation is available at:
- [Buttplug Protocol Spec](https://docs.buttplug.io/docs/spec/)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.