// buttplugCpp.cpp : Defines the entry point for the application.
//

#include "buttplugCpp.h"


using namespace std;

void callbackFunction(const mhl::Messages msg) {
	if (msg.messageType == mhl::MessageTypes::DeviceList) {
		DEBUG_MSG("Device List callback");
	}
	if (msg.messageType == mhl::MessageTypes::DeviceAdded) {
		DEBUG_MSG("Device Added callback");
	}
	if (msg.messageType == mhl::MessageTypes::ServerInfo) {
		DEBUG_MSG("Server Info callback");
	}
	if (msg.messageType == mhl::MessageTypes::DeviceRemoved) {
		DEBUG_MSG("Device Removed callback");
	}
	if (msg.messageType == mhl::MessageTypes::SensorReading) {
		DEBUG_MSG("Sensor Reading callback");
	}
}

int main()
{
	std::string url = "ws://127.0.0.1";
    DEBUG_MSG("\n");
	Client client(url, 12345, "test_log");
	client.connect(callbackFunction);
	client.requestDeviceList();
	client.startScan();
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		client.stopScan();
		break;
	}
	std::vector<DeviceClass> myDevices = client.getDevices();
	// std::cout << myDevices.at(0).deviceID;
	client.getDeviceCommandAttributes(myDevices.at(0), "ScalarCmd");
	std::map<unsigned int, double> act_map = {{0, 0.5}, {1, 0.1}};
	client.sendScalarActuators(myDevices[0], act_map);
	// client.sendScalar(myDevices[0], 0.1);
	//client.sendScalar(myDevices[1], 0.5);
	//client.sensorSubscribe(myDevices[0], 0);
	//std::this_thread::sleep_for(std::chrono::milliseconds(20000));
	//client.sensorUnsubscribe(myDevices[0], 0);
	// client.stopAllDevices();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	// client.stopAllDevices();

	return 0;
}

