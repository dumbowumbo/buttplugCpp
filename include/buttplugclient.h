#include <string>
#include <functional>
#include <iostream>
#include <atomic>
#include <sstream>
#include <queue>
#include <map>

#include <ixwebsocket/IXWebSocket.h>
#ifdef _WIN32
#include <ixwebsocket/IXNetSystem.h>
#endif
#include "messageHandler.h"
#include "log.h"
// #include "thread_safe_queue.hpp"

// Alias for sensor reading class to make it more accessible
typedef msg::SensorReading SensorClass;

// Helper class to store devices and access them outside of the library.
class DeviceClass {
public:
	std::string deviceName;
	std::string displayName;
	std::vector<std::string> commandTypes;
	std::vector<std::string> sensorTypes;
	std::map<std::string, std::vector<DeviceCmdAttr>> commandAttributes;
	unsigned int deviceID;
};

// Main client class
class Client {
public:
	// Constructor which initializes websockets for Windows. Add an IFDEF depending on compilation OS for portability.
	Client(std::string url, unsigned int port) {
		#ifdef _WIN32
		ix::initNetSystem();
		#endif
		lUrl = url;
		lPort = port;
	}
	
	// Constructor with logging capability
	Client(std::string url, unsigned int port, std::string logfile) {
		#ifdef _WIN32
		ix::initNetSystem();
		#endif
		lUrl = url;
		lPort = port;
		if (!logfile.empty()) {
			logging = 1;
			logInfo.start(logfile);
		}
		// else logInfo.init("log.txt");
	}
	
	// Destructor that cleans up resources and ensures thread termination
	~Client() {
		#ifdef _WIN32
		ix::uninitNetSystem();
		#endif
		stopRequested = true;
		cond.notify_one(); // Notify the condition variable to wake up the thread
		if (messageHandlerThread.joinable()) {
			messageHandlerThread.join();
		}
		logInfo.stop();
		webSocket.stop();
	}

	// Connects to the server and sets up the message handling callbacks
	int connect(void (*callFunc)(const mhl::Messages));
	
	// Atomic variables to store connection status. Can be accessed outside library too since atomic.
	std::atomic<int> wsConnected{0};
	std::atomic<int> isConnecting{0};
	std::atomic<int> clientConnected{0};
	
	// Condition variables for thread synchronization, maintaining C++11 support
	std::condition_variable condClient;
	std::condition_variable condWs;

	// Public functions that send requests to server.
	void startScan();
	void stopScan();
	void requestDeviceList();
	void stopDevice(DeviceClass dev);
	void stopAllDevices();
	void sendScalar(DeviceClass dev, double str);
	void sendScalarActuators(DeviceClass dev, const std::map<unsigned int, double>& actuatorValues);
	void sendLinear(DeviceClass dev, double duration, double position);
    void sendLinearActuators(DeviceClass dev, const std::map<unsigned int, std::pair<double, double>>& actuatorValues);
    void sendRotation(DeviceClass dev, double speed, bool clockwise);
    void sendRotationActuators(DeviceClass dev, const std::map<unsigned int, std::pair<double, bool>>& actuatorValues);
	std::vector<DeviceCmdAttr> getDeviceCommandAttributes(DeviceClass dev, const std::string& commandType);
	void sensorRead(DeviceClass dev, int senIndex);
	void sensorSubscribe(DeviceClass dev, int senIndex);
	void sensorUnsubscribe(DeviceClass dev, int senIndex);

	void waitForEmptyConfirmQueue();

	// Mutex blocked function which grabs the currently connected devices and sensor reads.
	std::vector<DeviceClass> getDevices();
	SensorClass getSensors();
private:
	// URL variables for the websocket.
	std::string FullUrl;
	std::string lUrl;
	unsigned int lPort;

	// Logging flag and logger object
	int logging = 0;
	Logger logInfo;

	// WebSocket client for server communication
	ix::WebSocket webSocket;

	// Message handler class, which takes messages, parses them and makes them to classes.
	mhl::Messages messageHandler;

	// Queue variable for passing received messages from server.
	std::queue<std::string> q;
	// Condition variable to wait for received messages in the queue.
	std::condition_variable cond;
	std::condition_variable condQueue;
	// Mutex to ensure no race conditions.
	std::mutex msgMx;
	// Callback function for when a message is received and handled.
	std::function<void(const mhl::Messages&)> messageCallback;

	// Device and sensor class vector which is grabbed outside of the library.
	std::vector<DeviceClass> devices;
	SensorClass sensorData;

	// Thread for handling incoming messages
	std::thread messageHandlerThread;
    std::atomic<bool> stopRequested{false};

	// Private helper methods
	void connectServer();
	void callbackFunction(const ix::WebSocketMessagePtr& msg);
	void messageHandling();
	void sendMessage(json msg, mhl::MessageTypes mType);
	void updateDevices();
	int findDevice(DeviceClass dev);
};