/*
 *  IT87x.h
 *  HWSensors
 *
 *  Created by mozo on 08/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 *	Open Hardware Monitor Port
 *
 */

//Additional functionality added by Navi, inspired by FakeSMC development. Credits goes to Netkas, slice, Mozo, usr-sse2 and others...

#include <IOKit/IOService.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOKitKeys.h>

#include "../SuperIOFamily/SuperIOFamily.h"

const UInt8 ITE_ENVIRONMENT_CONTROLLER_LDN				= 0x04;

// ITE
const UInt8 ITE_VENDOR_ID                         = 0x90;
const UInt8 ITE_VERSION_REGISTER                  = 0x22;

// ITE Environment Controller
const UInt8 ITE_ADDRESS_REGISTER_OFFSET           = 0x05;
const UInt8 ITE_DATA_REGISTER_OFFSET              = 0x06;

// ITE Environment Controller Registers    
const UInt8 ITE_CONFIGURATION_REGISTER            = 0x00;
const UInt8 ITE_TEMPERATURE_BASE_REG              = 0x29;
const UInt8 ITE_VENDOR_ID_REGISTER                = 0x58;
const UInt8 ITE_FAN_TACHOMETER_16_BIT_ENABLE_REGISTER	= 0x0c;
const UInt8 ITE_FAN_TACHOMETER_REG[5]             = { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
const UInt8 ITE_FAN_TACHOMETER_EXT_REG[5]         = { 0x18, 0x19, 0x1a, 0x81, 0x83 };
const UInt8 ITE_VOLTAGE_REG[9]                    = { 0x20, 0x21, 0x22, 0x23, 0x24,0x25,0x26,0x27,0x28};
const float ITE_VOLTAGE_GAIN[]                    = { 1, 1, 1, (6.8f / 10 + 1), 1, 1, 1, 1, 1 };
const UInt8 ITE_ADC_CHANNEL_ENABLE                = 0x50;

const UInt8 ITE_SMARTGUARDIAN_MAIN_CONTROL            = 0x13;
const UInt8 ITE_SMARTGUARDIAN_REG_CONTROL             = 0x14;
const UInt8 ITE_SMARTGUARDIAN_PWM_CONTROL[5]          = { 0x15, 0x16, 0x17, 0x88, 0x89 };
const UInt8 ITE_SMARTGUARDIAN_TEMPERATURE_STOP[5]     = { 0x60, 0x68, 0x70, 0x90, 0x98 };
const UInt8 ITE_SMARTGUARDIAN_TEMPERATURE_START[5]		= { 0x61, 0x69, 0x71, 0x91, 0x99 };
const UInt8 ITE_SMARTGUARDIAN_TEMPERATURE_FULL_ON[5]	= { 0x62, 0x6a, 0x72, 0x92, 0x9a };
const UInt8 ITE_SMARTGUARDIAN_START_PWM[5]            = { 0x63, 0x6b, 0x73, 0x93, 0x9b };
const UInt8 ITE_SMARTGUARDIAN_CONTROL[5]              = { 0x64, 0x6c, 0x74, 0x94, 0x9c };
const UInt8 ITE_SMARTGUARDIAN_TEMPERATURE_DELTA[5]    = { 0x65, 0x6d, 0x75, 0x95, 0x9d };
// 

#define MAX_TEMP_THRESHOLD 127

enum IT87xModel
{
  IT8512F = 0x8512,
  IT8613F = 0x8613,
  IT8620F = 0x8620,
  IT8628F = 0x8628,
  IT8655F = 0x8655,
  IT8665F = 0x8665,
  IT8686E = 0x8686,
  IT8688E = 0x8688,
  IT8705F = 0x8705,
  IT8708F = 0x8708,
  IT8712F = 0x8712,
  IT8716F = 0x8716,
  IT8718F = 0x8718,
  IT8720F = 0x8720,
  IT8721F = 0x8721,
  IT8726F = 0x8726,
  IT8728F = 0x8728,
  IT8752F = 0x8752,
  IT8771E = 0x8771,
  IT8772E = 0x8772,
  IT8792E = 0x8792,
  IT8795E = 0x8795,
  IT8987E = 0x8987
};

enum SuperIOSensorGroupEx {
    kSuperIOSmartGuardPWMControl = kSuperIOVoltageSensor +1,
    kSuperIOSmartGuardTempFanStop,
    kSuperIOSmartGuardTempFanStart,
    kSuperIOSmartGuardTempFanFullOn,
    kSuperIOSmartGuardPWMStart,
    kSuperIOSmartGuardTempFanFullOff,
    kSuperIOSmartGuardTempFanControl,
    kSuperIOSmartGuardMainControl,
    kSuperIOSmartGuardRegControl
};

class IT87x;

class IT87xSensor : public SuperIOSensor {
  OSDeclareDefaultStructors(IT87xSensor)
  
  
public:
  static SuperIOSensor *withOwner(SuperIOMonitor *aOwner,
                                  const char* aKey,
                                  const char* aType,
                                  unsigned char aSize,
                                  SuperIOSensorGroup aGroup,
                                  unsigned long aIndex,
                                  long aRi = 0,
                                  long aRf = 1,
                                  long aVf = 0);
  
  virtual long    getValue();
  virtual void    setValue(UInt16 value);
};

class IT87x : public SuperIOMonitor {
  OSDeclareDefaultStructors(IT87x)
  
  
private:
  long              voltageGain;
  bool              has16bitFanCounter;
  bool              hasSmartGuardian;
  bool              vbat_updates;
  
  char              vendor[40];
  char              product[40];
  
  virtual void      enter();
  virtual void      exit();
  
  virtual long      readTemperature(unsigned long index);
  virtual long      readVoltage(unsigned long index);
  virtual long      readTachometer(unsigned long index);
  
  virtual int        getPortsCount();
  virtual const char * getModelName();
  
public:
  virtual bool      init(OSDictionary *properties=0);
  virtual IOService* probe(IOService *provider, SInt32 *score);
  virtual bool      start(IOService *provider);
  virtual void      stop(IOService *provider);
  virtual void      free(void);
  
  
  virtual bool      probePort();
  
  virtual long      readSmartGuardPWMControl(unsigned long index);
  virtual long      readSmartGuardTempFanStop(unsigned long index);
  virtual long      readSmartGuardTempFanStart(unsigned long index);
  virtual long      readSmartGuardTempFanFullOn(unsigned long index);
  virtual long      readSmartGuardPWMStart(unsigned long index);
  virtual long      readSmartGuardTempFanFullOff(unsigned long index);
  virtual long      readSmartGuardFanControl(unsigned long index);
  virtual long      readSmartGuardMainControl(unsigned long index);
  virtual long      readSmartGuardRegControl(unsigned long index);
  //  New write SMC key value to SmartGuardian registers methods
  virtual void      writeSmartGuardPWMControl(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardTempFanStop(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardTempFanStart(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardTempFanFullOn(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardPWMStart(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardTempFanFullOff(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardFanControl(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardMainControl(unsigned long index, UInt16 value);
  virtual void      writeSmartGuardRegControl(unsigned long index, UInt16 value);
  
  SuperIOSensor * addSensor(const char* key,
                            const char* type,
                            unsigned int size,
                            SuperIOSensorGroup group,
                            unsigned long index,
                            long aRi = 0,
                            long aRf = 1,
                            long aVf = 0);
  
  virtual IOReturn callPlatformFunction(const OSSymbol *functionName,
                                        bool waitForFunction,
                                        void *param1,
                                        void *param2,
                                        void *param3,
                                        void *param4);
};

inline UInt8 readByte(UInt16 address, UInt8 reg) {
  outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
  
  UInt8 value = inb(address + ITE_DATA_REGISTER_OFFSET);
  __unused UInt8 check = inb(address + ITE_DATA_REGISTER_OFFSET);
  return value;
}

inline UInt16 readWord(UInt16 address,UInt8 reg1, UInt8 reg2) {
  return (readByte(address,reg1) << 8) | readByte(address,reg2);
}

inline void writeByte(UInt16 address,UInt8 reg, UInt8 value) {
  outb(address + ITE_ADDRESS_REGISTER_OFFSET, reg);
  outb(address + ITE_DATA_REGISTER_OFFSET, value);
}
