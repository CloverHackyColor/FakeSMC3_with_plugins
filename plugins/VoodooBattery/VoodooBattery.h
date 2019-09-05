/*
 --- VoodooBattery ---
 (C) 2009 Superhai
 
 Contact	http://www.superhai.com/
 
 */

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/hidsystem/IOHIKeyboard.h>

//#include "FakeSMC.h"

// Constants

const UInt8	MaxBatteriesSupported   = 4;
const UInt8 MaxAcAdaptersSupported  = 2;
const UInt8	AverageBoundPercent     = 25;

const UInt32	QuickPollInterval   = 1000;
const UInt32	NormalPollInterval  = 60000;
const UInt32	QuickPollPeriod     = 60000;
const UInt32	QuickPollCount      = QuickPollPeriod / QuickPollInterval;

const UInt32	AcpiUnknown  = 0xFFFFFFFF;
const UInt32	AcpiMax      = 0x80000000;

const UInt32	DummyVoltage = 12000;


const UInt32	StartLocation = kIOPMPSLocationLeft;

// String constants

const char *	PnpDeviceIdBattery		      = "PNP0C0A";
const char *	PnpDeviceIdLid				      = "PNP0C0D";
const char *	PnpDeviceIdAcAdapter		    = "ACPI0003";
const char *  PnpDeviceIdPnlf             = "APP0002";
const char *	AcpiStatus				          = "_STA";
const char *	AcpiPowerSource			        = "_PSR";
const char *	AcpiBatteryInformation		  = "_BIF";
const char *	AcpiBatteryInformationEx	  = "_BIX";
const char *	AcpiBatteryStatus		        = "_BST";
const char *	LidStatus					          = "_LID";
const char *  MethodBQC                 = "_BQC";
const char *  MethodBCM                 = "_BCM";
const char *  MethodBCL                 = "_BCL";

/*
// _BIF
Package {
  Power Unit                      // Integer (DWORD)
  Design Capacity                 // Integer (DWORD)
  Last Full Charge Capacity       // Integer (DWORD)
  Battery Technology              // Integer (DWORD)
  Design Voltage                  // Integer (DWORD)
  Design Capacity of Warning      // Integer (DWORD)
  Design Capacity of Low          // Integer (DWORD)
  Battery Capacity Granularity 1  // Integer (DWORD)
  Battery Capacity Granularity 2  // Integer (DWORD)
  Model Number                    // String (ASCIIZ)
  Serial Number                   // String (ASCIIZ)
  Battery Type                    // String (ASCIIZ)
  OEM Information                 // String (ASCIIZ)
}

// _BIX
Package {
  // ASCIIZ is ASCII character string terminated with a 0x00.
  Revision      //Integer
  Power Unit
  Design Capacity
  Last Full Charge Capacity     //Integer (DWORD)
  Battery Technology
  Design Voltage
  Design Capacity of Warning
  Design Capacity of Low
  Cycle Count                   //Integer (DWORD)
  Measurement Accuracy          //Integer (DWORD)
  Max Sampling Time             //Integer (DWORD)
  Min Sampling Time             //Integer (DWORD)
  Max Averaging Interval        //Integer (DWORD)
  Min Averaging Interval        //Integer (DWORD)
  Battery Capacity Granularity 1
  Battery Capacity Granularity 2
  Model Number
  Serial Number
  Battery Type
  OEM Information
}
*/

static const OSSymbol * unknownObjectKey		  = OSSymbol::withCString("");
static const OSSymbol * designCapacityKey		  = OSSymbol::withCString(kIOPMPSDesignCapacityKey);
static const OSSymbol * deviceNameKey			    = OSSymbol::withCString(kIOPMDeviceNameKey);
static const OSSymbol * fullyChargedKey			  = OSSymbol::withCString(kIOPMFullyChargedKey);
static const OSSymbol * instantAmperageKey		= OSSymbol::withCString("InstantAmperage");
static const OSSymbol * instantTimeToEmptyKey	= OSSymbol::withCString("InstantTimeToEmpty");
static const OSSymbol * softwareSerialKey		  = OSSymbol::withCString("BatterySerialNumber");
static const OSSymbol * chargeStatusKey			  = OSSymbol::withCString(kIOPMPSBatteryChargeStatusKey);
static const OSSymbol * permanentFailureKey		= OSSymbol::withCString("Permanent Battery Failure");
//
static const OSSymbol * batteryTypeKey  = OSSymbol::withCString("BatteryType");

//static IOPMPowerState PowerStates[2] = 
//{ {1,0,0,0,0,0,0,0,0,0,0,0}, {1,2,2,2,0,0,0,0,0,0,0,0} };

enum {																								// Apple loves to have the goodies private
    kIOPMSetValue				= (1<<16),
    kIOPMSetDesktopMode			= (1<<17),
    kIOPMSetACAdaptorConnected	= (1<<18)
};

// Battery state _BST
enum {
	BatteryFullyCharged	= 0,
	BatteryDischarging	= (1<<0),
	BatteryCharging		= (1<<1),
	BatteryCritical		= (1<<2),
//	BatteryWithAc		= (1<<7)
};

/*  Smart Battery Status Message Bits                   */
/*  Smart Battery Data Specification - rev 1.1          */
/*  Section 5.4 page 42                                 */
enum {
  kBOverChargedAlarmBit             = 0x8000,
  kBTerminateChargeAlarmBit         = 0x4000,
  kBOverTempAlarmBit                = 0x1000,
  kBTerminateDischargeAlarmBit      = 0x0800,
  kBRemainingCapacityAlarmBit       = 0x0200,
  kBRemainingTimeAlarmBit           = 0x0100,
  kBInitializedStatusBit            = 0x0080,
  kBDischargingStatusBit            = 0x0040,
  kBFullyChargedStatusBit           = 0x0020,
  kBFullyDischargedStatusBit        = 0x0010
};

class ButtonController;
class Button : public IOHIKeyboard
{
  OSDeclareDefaultStructors(Button);
private:
//
  bool pressed;
  ButtonController * _controller;
public:
  bool isPressed();
public:
  virtual bool sendEvent(UInt8);

  virtual bool attach(IOService * provider);
  virtual void detach(IOService * provider);

};

class ButtonController : public IOService
{
  OSDeclareDefaultStructors(ButtonController);

private:
  Button * ACButton2;

public:
  virtual bool init(OSDictionary * properties);
  virtual ButtonController * probe(IOService * provider, SInt32 * score);
  virtual bool start(IOService * provider);
  virtual void stop(IOService * provider);
  virtual bool sendEvent(UInt8 keyEvent);
};

struct BatteryClass {
	UInt32	LastFullChargeCapacity;
	UInt32	DesignCapacity;
	UInt32	DesignCapacityWarning;
	UInt32	DesignCapacityLow;
	UInt32	DesignVoltage;
	UInt32	Technology;
	UInt32	State;
	UInt32	PresentVoltage;
	UInt32	PresentRate;
	UInt32	AverageRate;
	UInt32	RemainingCapacity;
	UInt32	LastRemainingCapacity;
  UInt32	Cycle;
};

class AppleSmartBattery : public IOPMPowerSource {
	OSDeclareDefaultStructors(AppleSmartBattery)
private:
protected:
	IOService *	ParentService;
	void	BlankOutBattery(void);
	void	setDesignCapacity(unsigned int val);
	void	setDeviceName(OSSymbol * sym);
	void	setFullyCharged(bool charged);
	void	setInstantAmperage(int mA);
	void	setInstantaneousTimeToEmpty(int seconds);
	void	setSerialString(OSSymbol * sym);
	void	rebuildLegacyIOBatteryInfo(void);
  void	setBatteryType(OSSymbol * sym);
  //----- new for ElCapitan
  /* Protected "setter" methods for subclasses
   * Subclasses should use these setters to modify all battery properties.
   *
   * Subclasses must follow all property changes with a call to updateStatus()
   * to flush settings changes to upper level battery API clients.
   *
   */
#if 0
  void setExternalConnected(bool);
  void setExternalChargeCapable(bool);
  void setBatteryInstalled(bool);
  void setIsCharging(bool);
  void setAtWarnLevel(bool);
  void setAtCriticalLevel(bool);

  void setCurrentCapacity(unsigned int);
  void setMaxCapacity(unsigned int);
  void setTimeRemaining(int);
  void setAmperage(int);
  void setVoltage(unsigned int);
  void setCycleCount(unsigned int);
  void setAdapterInfo(int);
  void setLocation(int);

  void setErrorCondition(OSSymbol *);
  void setManufacturer(OSSymbol *);
  void setModel(OSSymbol *);
  void setSerial(OSSymbol *);
  void setLegacyIOBatteryInfo(OSDictionary *);
#endif
public:
	static	AppleSmartBattery * NewBattery(void);
	virtual IOReturn	message(UInt32 type, IOService * provider, void * argument);
  IOReturn setPowerState(unsigned long which, IOService *whom);
	friend class VoodooBattery;
};

class VoodooBattery : public IOService {
	OSDeclareDefaultStructors(VoodooBattery)
private:
  IOService*			fakeSMC;
  OSDictionary*		sensors;

	// *** Integers ***
	UInt8	BatteryCount;
	UInt8	AcAdapterCount;
	UInt32	QuickPoll;
	// *** Booleans ***
	bool	BatteryConnected[MaxBatteriesSupported];
	bool	AcAdapterConnected[MaxAcAdaptersSupported];
	bool	CalculatedAcAdapterConnected[MaxBatteriesSupported];
	bool	ExternalPowerConnected;
	bool	BatteriesConnected;
	bool	BatteriesAreFull;
	bool	PowerUnitIsWatt;
  bool  firstTime;
	// *** Other ***
	BatteryClass				      Battery[MaxBatteriesSupported];
	IOACPIPlatformDevice *		BatteryDevice[MaxBatteriesSupported];
	IOACPIPlatformDevice *		AcAdapterDevice[MaxAcAdaptersSupported];
	AppleSmartBattery *			  BatteryPowerSource[MaxBatteriesSupported];
	IOACPIPlatformDevice *		LidDevice;
  ButtonController *        ACButtonController;
  IOACPIPlatformDevice *    PNLFDevice;
  Button *                  ACButton;
	IOWorkLoop *				      WorkLoop;
	IOTimerEventSource *		  Poller;
  IOCommandGate *           fBatteryGate[MaxBatteriesSupported];
  int *                     brightnessLevels;
  int                       brightnessCount;

//  Button * ACButton;
	// *** Methods ***
	void	Update(void);
	void	CheckDevices(void);
  void  GetBatteryInfoEx(UInt8 battery, OSObject * acpi);
  void  GetBatteryInfo(UInt8 battery, OSObject * acpi);
  void  PublishBatteryInfo(UInt8 battery, OSObject * acpi, int Ext);
	void	BatteryInformation(UInt8 battery);
	void	BatteryStatus(UInt8 battery);
	void	ExternalPower(bool status);
  bool	addSensor(const char* key, const char* type, unsigned int size, int index);
  void  ChangeBrightness(int Shift);

protected:
//	bool	settingsChangedSinceUpdate;
public:
	// *** IOService ***
  virtual bool        init(OSDictionary *properties=0);
  virtual void        free(void);
	virtual	IOService *	probe(IOService * provider, SInt32 * score);
	virtual	bool      start(IOService * provider);
	virtual void      stop(IOService * provider);
	virtual IOReturn  setPowerState(unsigned long state, IOService * device);
	virtual IOReturn  message(UInt32 type, IOService * provider, void * argument);
  
  virtual IOReturn	callPlatformFunction(const OSSymbol *functionName,
                                         bool waitForFunction,
                                         void *param1,
                                         void *param2,
                                         void *param3,
                                         void *param4);

};

UInt32 GetValueFromArray(OSArray * array, UInt8 index) {
	OSObject * object = array->getObject(index);
	if (object && (OSTypeIDInst(object) == OSTypeID(OSNumber))) {
		OSNumber * number = OSDynamicCast(OSNumber, object);
    if (number) {
      return number->unsigned32BitValue();
    }
	}
	return -1;
}

OSSymbol * GetSymbolFromArray(OSArray * array, UInt8 index) {
	OSObject * object = array->getObject(index);
	if (object && (OSTypeIDInst(object) == OSTypeID(OSData))) {
		OSData * data = OSDynamicCast(OSData, object);
		if (data->appendByte(0x00, 1)) {
			return (OSSymbol *) OSSymbol::withCString((const char *) data->getBytesNoCopy());
		}
	}
  
	if (object && (OSTypeIDInst(object) == OSTypeID(OSString))) {
		OSString * string = OSDynamicCast(OSString, object);
    if (string) {
      return OSDynamicCast(OSSymbol, string);
    }
	}
	return (OSSymbol *) unknownObjectKey;
}
