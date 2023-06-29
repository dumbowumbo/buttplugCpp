#include "../include/buttplugclient.h"

// Connection function with a function parameter which acts as a callback.
int Client::connect(void (*callFunc)(const mhl::Messages)) {
	FullUrl = lUrl + ":" + std::to_string(lPort);

	webSocket.setUrl(FullUrl);

	// Ping interval option.
	webSocket.setPingInterval(10);

	// Per message deflate connection is enabled by default. You can tweak its parameters or disable it
	webSocket.disablePerMessageDeflate();

	// Set the callback function of the websocket
	std::function<void(const ix::WebSocketMessagePtr&)> callback = std::bind(&Client::callbackFunction, this, std::placeholders::_1);
	messageCallback = callFunc;

	webSocket.setOnMessageCallback(callback);

	// Start websocket and indicate that it is connecting (non blocking function, other functions must wait for it to connect).
	webSocket.start();
	isConnecting = 1;

	// Start a message handler thread and detach it.
	std::thread messageHandler(&Client::messageHandling, this);
	messageHandler.detach();

	// Connect to server, specifically send a RequestServerInfo
	connectServer();

	return 0;
}

// Websocket callback function.
void Client::callbackFunction(const ix::WebSocketMessagePtr& msg) {
	// If a message is received to the websocket, pass it to the message handler and notify to stop waiting.
	if (msg->type == ix::WebSocketMessageType::Message)
	{
		// Mutex lock this scope.
		std::lock_guard<std::mutex> lock{msgMx};
		// Push message.
		q.push(msg->str);
		// Notify conditional variable to stop waiting.
		cond.notify_one();
	}

	// Handle websocket errors.
	if (msg->type == ix::WebSocketMessageType::Error)
	{
		std::stringstream ss;
		ss << "Error: " << msg->errorInfo.reason << std::endl;
		ss << "#retries: " << msg->errorInfo.retries << std::endl;
		ss << "Wait time(ms): " << msg->errorInfo.wait_time << std::endl;
		ss << "HTTP Status: " << msg->errorInfo.http_status << std::endl;
		std::cout << ss.str() << std::endl;
	}

	// Set atomic variable that websocket is connected once it is open.
	if (msg->type == ix::WebSocketMessageType::Open) {
		wsConnected = 1;
		wsConnected.notify_all();
	}
	
	// Set atomic variable that websocket is not connected if socket closes.
	if (msg->type == ix::WebSocketMessageType::Close) {
		wsConnected = 0;
	}
}

// Function to start scanning in the server.
void Client::startScan() {
	// Mutex lock scope.
	std::lock_guard<std::mutex> lock{msgMx};

	// Get a request class from message handling header.
	mhl::Requests req;
	// Set the ID of message according to the message enum in message handling header.
	req.startScanning.Id = static_cast<unsigned int>(mhl::MessageTypes::StartScanning);
	// Set message type for the handler to recognize what message it is.
	messageHandler.messageType = mhl::MessageTypes::StartScanning;

	// Convert the returned handled request message class to json.
	json j = json::array({ messageHandler.handleClientRequest(req) });
	std::cout << j << std::endl;

	// Start a thread that sends the message.
	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Function to stop scanning, same as before but different type.
void Client::stopScan() {
	std::lock_guard<std::mutex> lock{msgMx};

	mhl::Requests req;
	req.stopScanning.Id = static_cast<unsigned int>(mhl::MessageTypes::StopScanning);
	messageHandler.messageType = mhl::MessageTypes::StopScanning;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	std::cout << j << std::endl;

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Function to get device list, same as before but different type.
void Client::requestDeviceList() {
	std::lock_guard<std::mutex> lock{msgMx};

	mhl::Requests req;
	req.stopScanning.Id = static_cast<unsigned int>(mhl::MessageTypes::RequestDeviceList);
	messageHandler.messageType = mhl::MessageTypes::RequestDeviceList;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	std::cout << j << std::endl;

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Function to send RequestServerInfo, same as before but different type.
void Client::connectServer() {
	std::lock_guard<std::mutex> lock{msgMx};

	mhl::Requests req;
	req.requestServerInfo.Id = static_cast<unsigned int>(mhl::MessageTypes::RequestServerInfo);
	req.requestServerInfo.ClientName = "Testing";
	req.requestServerInfo.MessageVersion = 3;
	messageHandler.messageType = mhl::MessageTypes::RequestServerInfo;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	std::cout << j << std::endl;

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Function that actually sends the message.
void Client::sendMessage(json msg, mhl::MessageTypes mType) {
	// First check whether a connection process is started.
	if (!isConnecting && !wsConnected) {
		std::cout << "Client is not connected and not started, start before sending a message" << std::endl;
		return;
	}
	// If started, wait for the socket to connect first.
	if (!wsConnected && isConnecting) {
		std::DBOUT << "Waiting for socket to connect" << std::endl;
		wsConnected.wait(0);
		std::cout << "Connected to socket" << std::endl;
		//webSocket.send(msg.dump());
	}
	// Once socket is connected, either wait for client to connect, or send a message if the message type
	// is a request server info, since this is our client connection message.
	if (!clientConnected && isConnecting) {
		std::cout << "Waiting for client to connect" << std::endl;
		if (mType == mhl::MessageTypes::RequestServerInfo) {
			webSocket.send(msg.dump());
			std::cout << "Started connection to client" << std::endl;
			return;
		}
		clientConnected.wait(0);
		std::cout << "Connected to client" << std::endl;
		webSocket.send(msg.dump());
	}
	// If everything is connected, simply send message.
	else if (wsConnected && clientConnected) webSocket.send(msg.dump());
}

// This takes the device data from message handler and puts it in to main device class
// for user access.
void Client::updateDevices() {
	std::vector<DeviceClass> tempDeviceVec;
	// Iterate through available devices.
	for (auto& el : messageHandler.deviceList.Devices) {
		DeviceClass tempDevice;
		// Set the appropriate class variables.
		tempDevice.deviceID = el.DeviceIndex;
		tempDevice.deviceName = el.DeviceName;
		tempDevice.displayName = el.DeviceDisplayName;
		if (el.DeviceMessages.size() > 0) {
			for (auto& el2 : el.DeviceMessages) tempDevice.commandTypes.push_back(el2.CmdType);
		}
		// Push back the device in vector.
		tempDeviceVec.push_back(tempDevice);
	}
	devices = tempDeviceVec;
}

// Mutex locked function to provide the user with available devices.
std::vector<DeviceClass> Client::getDevices() {
	std::lock_guard<std::mutex> lock{msgMx};
	return devices;
}

int Client::findDevice(DeviceClass dev) {
	for (int i = 0; i < messageHandler.deviceList.Devices.size(); i++)
		if (messageHandler.deviceList.Devices[i].DeviceIndex == dev.deviceID)
			return i;
	return -1;
}

void Client::sendScalar(DeviceClass dev, double str) {
	std::lock_guard<std::mutex> lock{msgMx};
	int idx = findDevice(dev);
	mhl::Requests req;
	
	req.scalarCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
	req.scalarCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::ScalarCmd);
	int i = 0;
	for (auto& el : messageHandler.deviceList.Devices[idx].DeviceMessages[0].DeviceCmdAttributes) {
		Scalar sc;
		sc.ActuatorType = el.ActuatorType;
		sc.ScalarVal = str;
		sc.Index = i;
		req.scalarCmd.Scalars.push_back(sc);
		i++;
	}
	messageHandler.messageType = mhl::MessageTypes::ScalarCmd;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	std::cout << j << std::endl;

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Message handling function.
// TODO: add client disconnect which stops this thread too.
void Client::messageHandling() {
	// Start infinite loop.
	while (1) {
		std::unique_lock<std::mutex> lock{msgMx};

		// A lambda that waits to receive messages in the queue.
		cond.wait(
			lock,
			[this] {
				return !q.empty();
			}
		);

		// If received, grab the message and pop it out.
		std::string value = q.front();
		q.pop();

		// Handle the message.
		json j = json::parse(value);
		// Iterate through messages since server can send array.
		for (auto& el : j.items()) {
			// Pass the message to actual handler.
			messageHandler.handleServerMessage(el.value());

			// If server info received, it means client is connected so set the connection atomic variables
			// and notify all send threads that they are good to go.
			if (messageHandler.messageType == mhl::MessageTypes::ServerInfo) {
				isConnecting = 0;
				clientConnected = 1;
				clientConnected.notify_all();
			}
			// If a device updated message, make sure to update the devices for the user.
			if (messageHandler.messageType == mhl::MessageTypes::DeviceAdded ||
				messageHandler.messageType == mhl::MessageTypes::DeviceList  ||
				messageHandler.messageType == mhl::MessageTypes::DeviceRemoved) updateDevices();

			// Callback function for the user.
			messageCallback(messageHandler);
		}

		lock.unlock();

		std::cout << "[subscriber] Received " << value << std::endl;
	}
}
