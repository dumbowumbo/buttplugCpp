#include <string>
#include <vector>
#include <iostream>

// Define debug macro that checks for BUTTPLUG_DEBUG being defined
#ifdef BUTTPLUG_DEBUG
#define DEBUG_MSG(str) do { std::cout << "[DEBUG] " << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

// Class representing device command attributes used to define properties of device features
class DeviceCmdAttr {
public:
	std::string FeatureDescriptor = "";
	unsigned int StepCount = 0;
	std::string ActuatorType = "";
	std::string SensorType = "";
	std::vector<int> SensorRange; // every two steps
	//std::vector<std::string> Endpoints;
};

// Class representing a command that a device supports
class DeviceCmd {
public:
	std::string CmdType = "";
	std::string StopDeviceCmd = "";
	std::vector<DeviceCmdAttr> DeviceCmdAttributes;
};

// Class representing a device connected to the buttplug server
class Device {
public:
	std::string DeviceName;
	unsigned int DeviceIndex;
	unsigned int DeviceMessageTimingGap = 0;
	std::string DeviceDisplayName = "";
	std::vector<DeviceCmd> DeviceMessages;
};

// Class representing a scalar command that can be sent to a device
class Scalar {
public:
	unsigned int Index;
	double ScalarVal;
	std::string ActuatorType;
};

// Class representing a linear movement command for a device
class Linear {
public:
	unsigned int Index;
	double Duration;
	double Position;
};

// Class representing a rotational movement command for a device
class Rotate {
public:
	unsigned int Index;
	double Speed;
	bool Clockwise;
};
