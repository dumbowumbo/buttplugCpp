#include <nlohmann/json.hpp>
#include "helperClasses.h"
#include <string>
#include <vector>
#include <iostream>

using json = nlohmann::json;

// Classes for the various message types and functions to convert to/from json.
namespace msg {
	class Ok {
	public:
		unsigned int Id = 5;
		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ok, Id);
	};

	class Error {
	public:
		unsigned int Id = 0;
		std::string ErrorMessage = "test";
		int ErrorCode = 0;

		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(Error, Id, ErrorMessage, ErrorCode);
	};

	class RequestServerInfo {
	public:
		unsigned int Id = 1;
		std::string ClientName = "Default Client";
		unsigned int MessageVersion = 3;
	};

	class ServerInfo {
	public:
		unsigned int Id;
		std::string ServerName;
		unsigned int MessageVersion;
		unsigned int MaxPingTime;

		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(ServerInfo, Id, ServerName, MessageVersion, MaxPingTime);
	};

	class StartScanning {
	public:
		unsigned int Id = 1;
	};

	class StopScanning {
	public:
		unsigned int Id = 1;
	};

	class ScanningFinished {
	public:
		unsigned int Id = 1;
		//NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScanningFinished, Id);
	};

	class RequestDeviceList {
	public:
		unsigned int Id = 1;
	};

	class DeviceList {
	public:
		unsigned int Id = 1;
		std::vector<Device> Devices;
	};

	class DeviceAdded {
	public:
		unsigned int Id = 1;
		Device device;
	};

	class DeviceRemoved {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
	};

	class StopDeviceCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
	};

	class StopAllDevices {
	public:
		unsigned int Id = 1;
	};

	class ScalarCmd {
	public:
		unsigned int Id = 1;
		unsigned int DeviceIndex;
		std::vector<Scalar> Scalars;
	};

	extern void to_json(json& j, const RequestServerInfo& k);
	extern void to_json(json& j, const StartScanning& k);
	extern void to_json(json& j, const StopScanning& k);
	extern void to_json(json& j, const RequestDeviceList& k);
	extern void to_json(json& j, const StopDeviceCmd& k);
	extern void to_json(json& j, const StopAllDevices& k);
	extern void to_json(json& j, const ScalarCmd& k);
	extern void from_json(const json& j, ServerInfo& k);
	extern void from_json(const json& j, DeviceList& k);
	extern void from_json(const json& j, DeviceAdded& k);
	extern void from_json(const json& j, DeviceRemoved& k);
}