#include "../include/messageHandler.h"

namespace mhl {
	// Function that handles messages received from server.
	void Messages::handleServerMessage(json& msg) {
		// Grab the string of message type and find it in the map.
		auto msgType = msg.begin().key();
		auto result = std::find_if(
			messageMap.begin(),
			messageMap.end(),
			[msgType](const auto& mo) {return mo.second == msgType; });
		auto msgEnumType = result->first;

		int i = 0;
		// Switch that converts message to class.
		switch (msgEnumType) {
		case mhl::MessageTypes::Ok:
			break;
		case mhl::MessageTypes::Error:
			break;
		case mhl::MessageTypes::ServerInfo:
			// Set message type and convert to class from json.
			std::cout << "Server info!" << std::endl;
			messageType = mhl::MessageTypes::ServerInfo;
			serverInfo = msg.get<msg::ServerInfo>();
			break;
		case mhl::MessageTypes::ScanningFinished:
			break;
		case mhl::MessageTypes::DeviceList:
			std::cout << "Device list!" << std::endl;
			messageType = mhl::MessageTypes::DeviceList;
			deviceList = msg.get<msg::DeviceList>();
			break;
		case mhl::MessageTypes::DeviceAdded:
			deviceAdded = msg.get<msg::DeviceAdded>();
			messageType = mhl::MessageTypes::DeviceAdded;
			// Push back to message handler class device list the newly added device.
			deviceList.Devices.push_back(deviceAdded.device);
			break;
		case mhl::MessageTypes::DeviceRemoved:
			deviceRemoved = msg.get<msg::DeviceRemoved>();
			messageType = mhl::MessageTypes::DeviceRemoved;
			// Erase device from message handler class device list.
			for (auto& el : deviceList.Devices) {
				if (deviceRemoved.DeviceIndex == el.DeviceIndex) {
					deviceList.Devices.erase(deviceList.Devices.begin() + i);
					break;
				}
				i++;
			}
			break;
		case mhl::MessageTypes::SensorReading:
			break;
		}
	}

	// Convert client request classes to json.
	json Messages::handleClientRequest(Requests req) {
		json j;
		switch (messageType) {
		case mhl::MessageTypes::RequestServerInfo:
			j = req.requestServerInfo;
			break;
		case mhl::MessageTypes::RequestDeviceList:
			j = req.requestDeviceList;
			break;
		case mhl::MessageTypes::StartScanning:
			j = req.startScanning;
			break;
		case mhl::MessageTypes::StopScanning:
			j = req.stopScanning;
			break;
		case mhl::MessageTypes::ScalarCmd:
			j = req.scalarCmd;
			break;
		}
		return j;
	}
}