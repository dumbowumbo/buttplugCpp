#include <nlohmann/json.hpp>
#include "helperClasses.h"
#include <string>
#include <vector>
#include <iostream>

using json = nlohmann::json;

// Classes for the various message types and functions to convert to/from JSON.
namespace msg {
	// Response indicating successful operation
	class Ok {
	public:
		unsigned int Id = 1;
		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ok, Id);
	};
	
	// Response indicating error condition
	class Error {
	public:
		unsigned int Id = 1;
		std::string ErrorMessage = "";
		int ErrorCode = 0;

		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(Error, Id, ErrorMessage, ErrorCode);
	};
	
	// Request to initialize server connection
	class RequestServerInfo {
	public:
		unsigned int Id = 1;
		std::string ClientName = "Default Client";
		unsigned int MessageVersion = 3;
	};
	
	// Response with server details
	class ServerInfo {
	public:
		unsigned int Id;
		std::string ServerName;
		unsigned int MessageVersion;
		unsigned int MaxPingTime;

		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(ServerInfo, Id, ServerName, MessageVersion, MaxPingTime);
	};
	
	// Request to start device scanning
	class StartScanning {
	public:
		unsigned int Id = 1;
	};
	
	// Request to stop device scanning
	class StopScanning {
	public:
		unsigned int Id = 1;
	};
	
	// Notification that scanning has completed
	class ScanningFinished {
	public:
		unsigned int Id = 1;
		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScanningFinished, Id);
	};
	
	// Request to get a list of connected devices
	class RequestDeviceList {
	public:
		unsigned int Id = 1;
	};
	
	// Response containing list of connected devices
	class DeviceList {
	public:
		unsigned int Id = 1;
		std::vector<Device> Devices;
	};
	
	// Notification that a device has been added
	class DeviceAdded {
	public:
		unsigned int Id = 1;
		Device device;
	};
	
	// Notification that a device has been removed
	class DeviceRemoved {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
	};
	
	// Command to stop a specific device
	class StopDeviceCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
	};
	
	// Command to stop all devices
	class StopAllDevices {
	public:
		unsigned int Id = 1;
	};
	
	// Command to send scalar (vibration) values to a device
	class ScalarCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		std::vector<Scalar> Scalars;
	};
	
	// Command to send linear movement values to a device
	class LinearCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		std::vector<Linear> Vectors;
	};
	
	// Command to send rotation values to a device
	class RotateCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		std::vector<Rotate> Rotations;
	};
	
	// Command to read sensor data from a device
	class SensorReadCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		unsigned int SensorIndex;
		std::string SensorType;
	};
	
	// Response containing sensor reading data
	class SensorReading {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		unsigned int SensorIndex;
		std::string SensorType;
		std::vector<int> Data;
	};
	
	// Command to subscribe to sensor updates
	class SensorSubscribeCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		unsigned int SensorIndex;
		std::string SensorType;
	};
	
	// Command to unsubscribe from sensor updates
	class SensorUnsubscribeCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		unsigned int SensorIndex;
		std::string SensorType;
	};

	// JSON serialization function declarations for request messages
	extern void to_json(json& j, const RequestServerInfo& k);
	extern void to_json(json& j, const StartScanning& k);
	extern void to_json(json& j, const StopScanning& k);
	extern void to_json(json& j, const RequestDeviceList& k);
	extern void to_json(json& j, const StopDeviceCmd& k);
	extern void to_json(json& j, const StopAllDevices& k);
	extern void to_json(json& j, const ScalarCmd& k);
	extern void to_json(json& j, const LinearCmd& k);
	extern void to_json(json& j, const RotateCmd& k);
	extern void to_json(json& j, const SensorReadCmd& k);
	extern void to_json(json& j, const SensorSubscribeCmd& k);
	extern void to_json(json& j, const SensorUnsubscribeCmd& k);
	
	// JSON deserialization function declarations for response messages
	extern void from_json(const json& j, Ok& k);
	extern void from_json(const json& j, Error& k);
	extern void from_json(const json& j, ServerInfo& k);
	extern void from_json(const json& j, DeviceList& k);
	extern void from_json(const json& j, DeviceAdded& k);
	extern void from_json(const json& j, DeviceRemoved& k);
	extern void from_json(const json& j, SensorReading& k);
}
