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
	messageHandlerThread = std::thread(&Client::messageHandling, this);
	// messageHandlerThread.detach();

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
		DEBUG_MSG(ss.str());
	}

	// Set atomic variable that websocket is connected once it is open.
	if (msg->type == ix::WebSocketMessageType::Open) {
		wsConnected = 1;
		condWs.notify_all();
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
	DEBUG_MSG(j);

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
	DEBUG_MSG(j);

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
	DEBUG_MSG(j);

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
	DEBUG_MSG(j);

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

// Function that actually sends the message.
void Client::sendMessage(json msg, mhl::MessageTypes mType) {
	// First check whether a connection process is started.
	if (!isConnecting && !wsConnected) {
		DEBUG_MSG("Client is not connected and not started, start before sending a message");
		return;
	}
	// If started, wait for the socket to connect first.
	if (!wsConnected && isConnecting) {
		std::unique_lock<std::mutex> lock{msgMx};
		DEBUG_MSG("Waiting for socket to connect");
		auto wsConnStatus = [this]() {return wsConnected == 1; };
		condWs.wait(lock, wsConnStatus);
		DEBUG_MSG("Connected to socket");
		//webSocket.send(msg.dump());
	}
	// Once socket is connected, either wait for client to connect, or send a message if the message type
	// is a request server info, since this is our client connection message.
	if (!clientConnected && isConnecting) {
		DEBUG_MSG("Waiting for client to connect");
		if (mType == mhl::MessageTypes::RequestServerInfo) {
			webSocket.send(msg.dump());
			DEBUG_MSG(msg.dump());
			if (logging) logInfo.logSentMessage("RequestServerInfo", static_cast<unsigned int>(msg.at(0).at("RequestServerInfo").at("Id")));
			DEBUG_MSG("Started connection to client");
			return;
		}
		std::unique_lock<std::mutex> lock{msgMx};
		auto clientConnStatus = [this]() {return clientConnected == 1; };
		// Wait until client connection is established
		condClient.wait(lock, clientConnStatus);
		DEBUG_MSG("Connected to client");
		webSocket.send(msg.dump());
	}
	// If everything is connected, simply send message and log request if enabled.
	else if (wsConnected && clientConnected) webSocket.send(msg.dump());
	if (logging) {
		// Find the string representation of the message type for logging
		auto result = std::find_if(
			messageHandler.messageMap.begin(),
			messageHandler.messageMap.end(),
			[mType](std::pair<const mhl::MessageTypes, std::basic_string<char> > mo) {return mo.first == mType; });
		std::string msgTypeText = result->second;
		// Log the sent message with its type and ID
		logInfo.logSentMessage(msgTypeText, static_cast<unsigned int>(msg.at(0).at(msgTypeText).at("Id")));
	}
}

// Function to update the internal devices vector based on the current state of messageHandler.deviceList
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
            for (auto& el2 : el.DeviceMessages) {
                // Add each command type supported by the device
                tempDevice.commandTypes.push_back(el2.CmdType);
                
                // Store attributes for all command types
                if (!el2.DeviceCmdAttributes.empty()) {
                    tempDevice.commandAttributes[el2.CmdType] = el2.DeviceCmdAttributes;
                }
            }
        }
        // Push back the device in vector.
        tempDeviceVec.push_back(tempDevice);
    }
    // Update the class member with the new device list
    devices = tempDeviceVec;
}

// Mutex locked function to provide the user with available devices.
std::vector<DeviceClass> Client::getDevices() {
	std::lock_guard<std::mutex> lock{msgMx};
	return devices;
}

SensorClass Client::getSensors() {
	std::lock_guard<std::mutex> lock{msgMx};
	return sensorData;
}

int Client::findDevice(DeviceClass dev) {
	for (int i = 0; i < messageHandler.deviceList.Devices.size(); i++)
		if (messageHandler.deviceList.Devices[i].DeviceIndex == dev.deviceID)
			return i;
	return -1;
}

void Client::stopDevice(DeviceClass dev) {
	std::lock_guard<std::mutex> lock{msgMx};

	mhl::Requests req;
	req.stopDeviceCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::StopDeviceCmd);
	req.stopDeviceCmd.DeviceIndex = dev.deviceID;
	
	messageHandler.messageType = mhl::MessageTypes::StopDeviceCmd;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	DEBUG_MSG(j);

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

void Client::stopAllDevices() {
	std::lock_guard<std::mutex> lock{msgMx};

	mhl::Requests req;
	req.stopDeviceCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::StopAllDevices);

	messageHandler.messageType = mhl::MessageTypes::StopAllDevices;

	json j = json::array({ messageHandler.handleClientRequest(req) });
	DEBUG_MSG(j);

	std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
	sendHandler.detach();
}

void Client::sendScalar(DeviceClass dev, double str) {
	std::lock_guard<std::mutex> lock{msgMx};
	int idx = findDevice(dev);
	if (idx > -1) {
		mhl::Requests req;

		for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
			std::string testScalar = "ScalarCmd";
			if (!el1.CmdType.compare(testScalar)) {
				req.scalarCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
				req.scalarCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::ScalarCmd);
				int i = 0;
				for (auto& el2: el1.DeviceCmdAttributes) {
					Scalar sc;
					sc.ActuatorType = el2.ActuatorType;
					sc.ScalarVal = str;
					sc.Index = i;
					req.scalarCmd.Scalars.push_back(sc);
					i++;
				}
				messageHandler.messageType = mhl::MessageTypes::ScalarCmd;

				json j = json::array({ messageHandler.handleClientRequest(req) });
				DEBUG_MSG(j);

				std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
				sendHandler.detach();
			}
		}
	}
}

std::vector<DeviceCmdAttr> Client::getDeviceCommandAttributes(DeviceClass dev, const std::string& commandType) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == commandType) {
				for (auto &el2 : el1.DeviceCmdAttributes)
				DEBUG_MSG(el2.ActuatorType << " " << el2.FeatureDescriptor << " " << el2.StepCount);
                return el1.DeviceCmdAttributes;
            }
        }
    }
    return {};
}

void Client::sendScalarActuators(DeviceClass dev, const std::map<unsigned int, double>& actuatorValues) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        mhl::Requests req;

        // Find the ScalarCmd entry
        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == "ScalarCmd") {
                req.scalarCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
                req.scalarCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::ScalarCmd);
                
                // Use C++11 compatible map iteration
                for (auto it = actuatorValues.begin(); it != actuatorValues.end(); ++it) {
                    unsigned int actuatorIdx = it->first;
                    double value = it->second;
                    
                    // Check if this actuator index is valid
                    if (actuatorIdx < el1.DeviceCmdAttributes.size()) {
                        Scalar sc;
                        sc.ActuatorType = el1.DeviceCmdAttributes[actuatorIdx].ActuatorType;
                        sc.ScalarVal = value;
                        sc.Index = actuatorIdx;
                        req.scalarCmd.Scalars.push_back(sc);
                    }
                }
                
                if (!req.scalarCmd.Scalars.empty()) {
                    messageHandler.messageType = mhl::MessageTypes::ScalarCmd;

                    json j = json::array({ messageHandler.handleClientRequest(req) });
                    DEBUG_MSG(j);

                    std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
                    sendHandler.detach();
                }
                break; // Exit after finding ScalarCmd
            }
        }
    }
}

// Sends a LinearCmd to all linear actuators on a device
void Client::sendLinear(DeviceClass dev, double duration, double position) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        mhl::Requests req;

        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == "LinearCmd") {
                req.linearCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
                req.linearCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::LinearCmd);
                int i = 0;
                for (auto& el2 : el1.DeviceCmdAttributes) {
                    Linear lin;
                    lin.Duration = duration;
                    lin.Position = position;
                    lin.Index = i;
                    req.linearCmd.Vectors.push_back(lin);
                    i++;
                }
                
                if (!req.linearCmd.Vectors.empty()) {
                    messageHandler.messageType = mhl::MessageTypes::LinearCmd;

                    json j = json::array({ messageHandler.handleClientRequest(req) });
                    DEBUG_MSG(j);

                    std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
                    sendHandler.detach();
                }
                break; // Exit after finding LinearCmd
            }
        }
    }
}

// Sends a LinearCmd to specific linear actuators on a device
void Client::sendLinearActuators(DeviceClass dev, const std::map<unsigned int, std::pair<double, double>>& actuatorValues) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        mhl::Requests req;

        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == "LinearCmd") {
                req.linearCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
                req.linearCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::LinearCmd);

                for (auto it = actuatorValues.begin(); it != actuatorValues.end(); ++it) {
                    unsigned int actuatorIdx = it->first;
                    double duration = it->second.first;
                    double position = it->second.second;

                    if (actuatorIdx < el1.DeviceCmdAttributes.size()) {
                        Linear lin;
                        lin.Duration = duration;
                        lin.Position = position;
                        lin.Index = actuatorIdx;
                        req.linearCmd.Vectors.push_back(lin);
                    }
                }

                if (!req.linearCmd.Vectors.empty()) {
                    messageHandler.messageType = mhl::MessageTypes::LinearCmd;

                    json j = json::array({ messageHandler.handleClientRequest(req) });
                    DEBUG_MSG(j);

                    std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
                    sendHandler.detach();
                }
                break; // Exit after finding LinearCmd
            }
        }
    }
}

// Sends a RotateCmd to all rotational actuators on a device
void Client::sendRotation(DeviceClass dev, double speed, bool clockwise) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        mhl::Requests req;

        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == "RotateCmd") {
                req.rotateCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
                req.rotateCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::RotateCmd);
                int i = 0;
                for (auto& el2 : el1.DeviceCmdAttributes) {
                    Rotate rot;
                    rot.Speed = speed;
                    rot.Clockwise = clockwise;
                    rot.Index = i;
                    req.rotateCmd.Rotations.push_back(rot);
                    i++;
                }
                
                if (!req.rotateCmd.Rotations.empty()) {
                    messageHandler.messageType = mhl::MessageTypes::RotateCmd;

                    json j = json::array({ messageHandler.handleClientRequest(req) });
                    DEBUG_MSG(j);

                    std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
                    sendHandler.detach();
                }
                break; // Exit after finding RotateCmd
            }
        }
    }
}

// Sends a RotateCmd to specific rotational actuators on a device
void Client::sendRotationActuators(DeviceClass dev, const std::map<unsigned int, std::pair<double, bool>>& actuatorValues) {
    std::lock_guard<std::mutex> lock{msgMx};
    int idx = findDevice(dev);
    if (idx > -1) {
        mhl::Requests req;

        for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
            if (el1.CmdType == "RotateCmd") {
                req.rotateCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
                req.rotateCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::RotateCmd);

                for (auto it = actuatorValues.begin(); it != actuatorValues.end(); ++it) {
                    unsigned int actuatorIdx = it->first;
                    double speed = it->second.first;
                    bool clockwise = it->second.second;

                    if (actuatorIdx < el1.DeviceCmdAttributes.size()) {
                        Rotate rot;
                        rot.Speed = speed;
                        rot.Clockwise = clockwise;
                        rot.Index = actuatorIdx;
                        req.rotateCmd.Rotations.push_back(rot);
                    }
                }

                if (!req.rotateCmd.Rotations.empty()) {
                    messageHandler.messageType = mhl::MessageTypes::RotateCmd;

                    json j = json::array({ messageHandler.handleClientRequest(req) });
                    DEBUG_MSG(j);

                    std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
                    sendHandler.detach();
                }
                break; // Exit after finding RotateCmd
            }
        }
    }
}

void Client::sensorRead(DeviceClass dev, int senIndex) {
	std::lock_guard<std::mutex> lock{msgMx};
	int idx = findDevice(dev);
	if (idx > -1) {
		mhl::Requests req;

		for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
			std::string testSensor = "SensorReadCmd";
			if (!el1.CmdType.compare(testSensor)) {
				req.sensorReadCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
				req.sensorReadCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::SensorReadCmd);
				req.sensorReadCmd.SensorIndex = senIndex;
				req.sensorReadCmd.SensorType = el1.DeviceCmdAttributes[senIndex].SensorType;
				int i = 0;
				messageHandler.messageType = mhl::MessageTypes::SensorReadCmd;

				json j = json::array({ messageHandler.handleClientRequest(req) });
				DEBUG_MSG(j);

				std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
				sendHandler.detach();
			}
		}
	}
}

void Client::sensorSubscribe(DeviceClass dev, int senIndex) {
	std::lock_guard<std::mutex> lock{msgMx};
	int idx = findDevice(dev);
	if (idx > -1) {
		mhl::Requests req;

		for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
			std::string testSensor = "SensorReadCmd";
			if (!el1.CmdType.compare(testSensor)) {
				req.sensorSubscribeCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
				req.sensorSubscribeCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::SensorSubscribeCmd);
				req.sensorSubscribeCmd.SensorIndex = senIndex;
				req.sensorSubscribeCmd.SensorType = el1.DeviceCmdAttributes[senIndex].SensorType;
				int i = 0;
				messageHandler.messageType = mhl::MessageTypes::SensorSubscribeCmd;

				json j = json::array({ messageHandler.handleClientRequest(req) });
				DEBUG_MSG(j);

				std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
				sendHandler.detach();
			}
		}
	}
}

void Client::sensorUnsubscribe(DeviceClass dev, int senIndex) {
	std::lock_guard<std::mutex> lock{msgMx};
	int idx = findDevice(dev);
	if (idx > -1) {
		mhl::Requests req;

		for (auto& el1 : messageHandler.deviceList.Devices[idx].DeviceMessages) {
			std::string testSensor = "SensorReadCmd";
			if (!el1.CmdType.compare(testSensor)) {
				req.sensorUnsubscribeCmd.DeviceIndex = messageHandler.deviceList.Devices[idx].DeviceIndex;
				req.sensorUnsubscribeCmd.Id = static_cast<unsigned int>(mhl::MessageTypes::SensorUnsubscribeCmd);
				req.sensorUnsubscribeCmd.SensorIndex = senIndex;
				req.sensorUnsubscribeCmd.SensorType = el1.DeviceCmdAttributes[senIndex].SensorType;
				int i = 0;
				messageHandler.messageType = mhl::MessageTypes::SensorUnsubscribeCmd;

				json j = json::array({ messageHandler.handleClientRequest(req) });
				DEBUG_MSG(j);

				std::thread sendHandler(&Client::sendMessage, this, j, messageHandler.messageType);
				sendHandler.detach();
			}
		}
	}
}

// Message handling function.
void Client::messageHandling() {
	// Start infinite loop.
	while (!stopRequested) {
		std::unique_lock<std::mutex> lock{msgMx};

		// A lambda that waits to receive messages in the queue.
        cond.wait(
            lock,
            [this] {
                // Add stopRequested check here
                return !q.empty() || stopRequested;
            }
        );

		// Exit if stop was requested
		if (stopRequested) {
			return;
		}

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
				condClient.notify_all();
			}
			// If a device updated message, make sure to update the devices for the user.
			if (messageHandler.messageType == mhl::MessageTypes::DeviceAdded ||
				messageHandler.messageType == mhl::MessageTypes::DeviceList  ||
				messageHandler.messageType == mhl::MessageTypes::DeviceRemoved) updateDevices();

			if (messageHandler.messageType == mhl::MessageTypes::SensorReading) sensorData = messageHandler.sensorReading;

			// Log if logging is enabled.
			if (logging)
				logInfo.logReceivedMessage(el.value().begin().key(), static_cast<unsigned int>(el.value().begin().value().at("Id")));

			// Callback function for the user.
			messageCallback(messageHandler);
		}

		lock.unlock();

		DEBUG_MSG("[subscriber] Received " << value);
	}
}
