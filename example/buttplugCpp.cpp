// buttplugCpp.cpp : Defines the entry point for the application.
//

#include "buttplugCpp.h"


using namespace std;

void callbackFunction(const mhl::Messages msg) {
	if (msg.messageType == mhl::MessageTypes::DeviceList) {
		cout << "Device List callback" << endl;
	}
	if (msg.messageType == mhl::MessageTypes::DeviceAdded) {
		cout << "Device Added callback" << endl;
	}
	if (msg.messageType == mhl::MessageTypes::ServerInfo) {
		cout << "Server Info callback" << endl;
	}
	if (msg.messageType == mhl::MessageTypes::DeviceRemoved) {
		cout << "Device Removed callback" << endl;
	}
}

int main()
{
	std::string url = "ws://127.0.0.1";
	Client client(url, 12345);
	client.connect(callbackFunction);
	client.requestDeviceList();
	client.startScan();
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		client.stopScan();
		break;
	}

	std::vector<DeviceClass> myDevices = client.getDevices();
	client.sendScalar(myDevices[0], 0.5);
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	return 0;
}

