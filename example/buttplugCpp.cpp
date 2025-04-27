// buttplugCpp.cpp : Defines the entry point for the application.
//
// This example demonstrates basic usage of the ButtplugCpp client library
// to connect to a Buttplug server, scan for devices, and control them.

#include "buttplugCpp.h"

using namespace std;

/**
 * Callback function that handles messages from the Buttplug server
 * 
 * This function is called whenever the client receives a message from the server.
 * It demonstrates how to handle different types of messages that might be received.
 * 
 * @param msg The message received from the server
 */
void callbackFunction(const mhl::Messages msg) {
    // Handle different message types from the server
    switch (msg.messageType) {
        case mhl::MessageTypes::DeviceList:
            DEBUG_MSG("Device List callback - Server sent list of available devices");
            break;
            
        case mhl::MessageTypes::DeviceAdded:
            DEBUG_MSG("Device Added callback - New device detected by server");
            break;
            
        case mhl::MessageTypes::ServerInfo:
            DEBUG_MSG("Server Info callback - Server information received");
            break;
            
        case mhl::MessageTypes::DeviceRemoved:
            DEBUG_MSG("Device Removed callback - Device disconnected from server");
            break;
            
        case mhl::MessageTypes::SensorReading:
            DEBUG_MSG("Sensor Reading callback - Received sensor data from device");
            break;
            
    }
}

/**
 * Main application entry point
 */
int main()
{
    // Server connection parameters
    std::string url = "ws://127.0.0.1";
    int port = 12345;
    
    DEBUG_MSG("\n");
    
    // Create client and connect to the Buttplug server
    // The third parameter enables logging to a file with timestamp prefix
    Client client(url, port, "test_log");
    
    // Connect to the server and register our callback function
    client.connect(callbackFunction);
    
    // Request the list of devices already connected to the server
    client.requestDeviceList();
    
    // Start scanning for new devices
    client.startScan();
    
    // Scan for devices for 2 seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // Stop scanning once we're done looking for devices
    client.stopScan();
    
    // Get the list of available devices
    std::vector<DeviceClass> myDevices = client.getDevices();
    
    // Device control examples
    if (!myDevices.empty()) {
        // Send a scalar command (vibration) at full intensity to the first device
        client.sendScalar(myDevices[0], 1.0);
        
        /* 
         * Other control commands (uncomment to use):
         */
        
        // Print the device ID
        // std::cout << "Device ID: " << myDevices.at(0).deviceID << std::endl;
        
        // Get specific command attributes for a device
        // std::vector<DeviceCmdAttr> attrs = client.getDeviceCommandAttributes(myDevices.at(0), "ScalarCmd");
        
        // Control multiple actuators with different intensities
        // std::map<unsigned int, double> actuator_map = {{0, 0.5}, {1, 0.1}};
        // client.sendScalarActuators(myDevices[0], actuator_map);
        
        // Send a linear movement command (position and duration)
        // client.sendLinear(myDevices[0], 1.0, 0.5); // duration 1.0 seconds, position 0.5
        
        // Control a second device if available
        // if (myDevices.size() > 1) {
        //     client.sendScalar(myDevices[1], 0.5);
        // }
        
        // Sensor subscription example
        // client.sensorSubscribe(myDevices[0], 0); // Subscribe to sensor 0
        // std::this_thread::sleep_for(std::chrono::milliseconds(20000)); // Collect data for 20 seconds
        // client.sensorUnsubscribe(myDevices[0], 0); // Unsubscribe from sensor
    }
    
    // Stop all devices and ensure all commands have been processed
    client.stopAllDevices();
    client.waitForEmptyConfirmQueue();

    return 0;
}

