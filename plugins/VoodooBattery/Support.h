/*
 --- Support ---
 (C) 2008 Superhai
 
 Contact	http://www.superhai.com/
 
 */

// Information helpers

#include <IOKit/IOLib.h>

#define defString(s) defXString(s)
#define defXString(s) #s

const char * KextVersion = defString(KEXT_VERSION);
const char * KextProductName = defString(KEXT_PRODUCTNAME);
const char * KextOSX = defString(KEXT_OSX);
const char * KextConfig = defString(KEXT_CONFIG);
const char * KextBuildDate = __DATE__;
const char * KextBuildTime = __TIME__;

const unsigned long Kilo = 1000;
const unsigned long Mega = Kilo * 1000;
const unsigned long Giga = Mega * 1000;


// Debug output support

#define INFO 1
#define ERROR 1

#if DEBUG
#define DebugLog(string, args...) IOLog("%s: [Debug] [%05u] " string "\n", KextProductName, __LINE__, ## args); IOSleep(100)
#define DebugMarker IOLog("%s: [Debug] [%s][%u] \n", KextProductName, __PRETTY_FUNCTION__, __LINE__); IOSleep(100)
#else
#define DebugLog(string, args...)
#define DebugMarker
#endif

#if INFO
#define InfoLog(string, args...) IOLog("%s: " string "\n", KextProductName, ## args)
#else
#define InfoLog(string, args...)
#endif

#if ERROR
#define ErrorLog(string, args...) IOLog("%s: [Error] " string "\n", KextProductName, ## args)
#define WarningLog(string, args...) IOLog("%s: [Warning] " string "\n", KextProductName, ## args)
#else
#define ErrorLog(string, args...)
#define WarningLog(string, args...)
#endif

#if 0

// IOPMPowerSource class descriptive strings
// Power Source state is published as properties to the IORegistry under these
// keys.
#define kIOPMPSExternalConnectedKey                 "ExternalConnected"
#define kIOPMPSExternalChargeCapableKey             "ExternalChargeCapable"
#define kIOPMPSBatteryInstalledKey                  "BatteryInstalled"
#define kIOPMPSIsChargingKey                        "IsCharging"
#define kIOPMFullyChargedKey                        "FullyCharged"
#define kIOPMPSAtWarnLevelKey                       "AtWarnLevel"
#define kIOPMPSAtCriticalLevelKey                   "AtCriticalLevel"
#define kIOPMPSCurrentCapacityKey                   "CurrentCapacity"
#define kIOPMPSMaxCapacityKey                       "MaxCapacity"
#define kIOPMPSDesignCapacityKey                    "DesignCapacity"
#define kIOPMPSTimeRemainingKey                     "TimeRemaining"
#define kIOPMPSAmperageKey                          "Amperage"
#define kIOPMPSVoltageKey                           "Voltage"
#define kIOPMPSCycleCountKey                        "CycleCount"
#define kIOPMPSMaxErrKey                            "MaxErr"
#define kIOPMPSAdapterInfoKey                       "AdapterInfo"
#define kIOPMPSLocationKey                          "Location"
#define kIOPMPSErrorConditionKey                    "ErrorCondition"
#define kIOPMPSManufacturerKey                      "Manufacturer"
#define kIOPMPSManufactureDateKey                   "ManufactureDate"
#define kIOPMPSModelKey                             "Model"
#define kIOPMPSSerialKey                            "Serial"
#define kIOPMDeviceNameKey                          "DeviceName"
#define kIOPMPSLegacyBatteryInfoKey                 "LegacyBatteryInfo"
#define kIOPMPSBatteryHealthKey                     "BatteryHealth"
#define kIOPMPSHealthConfidenceKey                  "HealthConfidence"
#define kIOPMPSCapacityEstimatedKey	                "CapacityEstimated"
#define kIOPMPSBatteryChargeStatusKey               "ChargeStatus"
#define kIOPMPSBatteryTemperatureKey                "Temperature"
#define kIOPMPSAdapterDetailsKey                    "AdapterDetails"
#define kIOPMPSChargerConfigurationKey              "ChargerConfiguration"

// kIOPMPSBatteryChargeStatusKey may have one of the following values, or may have
// no value. If kIOPMBatteryChargeStatusKey has a NULL value (or no value) associated with it
// then charge is proceeding normally. If one of these battery charge status reasons is listed,
// then the charge may have been interrupted.
#define kIOPMBatteryChargeStatusTooHot              "HighTemperature"
#define kIOPMBatteryChargeStatusTooCold             "LowTemperature"
#define kIOPMBatteryChargeStatusTooHotOrCold        "HighOrLowTemperature"
#define kIOPMBatteryChargeStatusGradient            "BatteryTemperatureGradient"

// Definitions for battery location, in case of multiple batteries.
// A location of 0 is unspecified
// Location is undefined for single battery systems
enum {
  kIOPMPSLocationLeft = 1001,
  kIOPMPSLocationRight = 1002
};

// Battery quality health types, specified by BatteryHealth and HealthConfidence
// properties in an IOPMPowerSource battery kext.
enum {
  kIOPMUndefinedValue = 0,
  kIOPMPoorValue      = 1,
  kIOPMFairValue      = 2,
  kIOPMGoodValue      = 3
};

// Keys for kIOPMPSAdapterDetailsKey dictionary
#define kIOPMPSAdapterDetailsIDKey              "AdapterID"
#define kIOPMPSAdapterDetailsWattsKey           "Watts"
#define kIOPMPSAdapterDetailsRevisionKey        "AdapterRevision"
#define kIOPMPSAdapterDetailsSerialNumberKey	  "SerialNumber"
#define kIOPMPSAdapterDetailsFamilyKey          "FamilyCode"
#define kIOPMPSAdapterDetailsAmperageKey        "Amperage"
#define kIOPMPSAdapterDetailsDescriptionKey	    "Description"
#define kIOPMPSAdapterDetailsPMUConfigurationKey    "PMUConfiguration"
#define kIOPMPSAdapterDetailsVoltage            "AdapterVoltage"
#endif

