/*
 *  F718x.h
 *  HWSensors
 *
 *  Created by mozo on 16/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#include <IOKit/IOService.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOKitKeys.h>


#include "SuperIOFamily.h"

// Registers
const UInt8 FINTEK_CONFIGURATION_CONTROL_REGISTER = 0x02;
const UInt8 FINTEK_DEVCIE_SELECT_REGISTER         = 0x07;
const UInt8 FINTEK_CHIP_ID_REGISTER               = 0x20;
const UInt8 FINTEK_CHIP_REVISION_REGISTER         = 0x21;
const UInt8 FINTEK_BASE_ADDRESS_REGISTER          = 0x60;

const UInt8 FINTEK_VENDOR_ID_REGISTER             = 0x23;
const UInt16 FINTEK_VENDOR_ID                     = 0x1934;
const UInt8 F71858_HARDWARE_MONITOR_LDN           = 0x02;
const UInt8 FINTEK_HARDWARE_MONITOR_LDN           = 0x04;

// Hardware Monitor
const UInt8 FINTEK_ADDRESS_REGISTER_OFFSET        = 0x05;
const UInt8 FINTEK_DATA_REGISTER_OFFSET           = 0x06;

// Hardware Monitor Registers
const UInt8 FINTEK_VOLTAGE_BASE_REG               = 0x20;
const UInt8 FINTEK_TEMPERATURE_CONFIG_REG         = 0x69;
const UInt8 FINTEK_TEMPERATURE_BASE_REG           = 0x70;
const UInt8 FINTEK_FAN_TACHOMETER_REG[]           = { 0xA0, 0xB0, 0xC0, 0xD0 };

enum F718xMode  {
	F71858		= 0x0507,
  F71862		= 0x0601,
  F71869		= 0x0814,
  F71882		= 0x0541,
  F71889ED	= 0x0909,
  F71889F		= 0x0723,
	F71808		= 0x0901,
  F71889AD	= 0x1005,
  F71868A   = 0x1106,
};

class F718x : public SuperIOMonitor {
  OSDeclareDefaultStructors(F718x)
  
private:
  char              vendor[40];
  char              product[40];
  
  UInt8             readByte(UInt8 reg);
  
  virtual bool      probePort();
  virtual void      enter();
  virtual void      exit();
  
  virtual long      readTemperature(unsigned long index);
  virtual long      readVoltage(unsigned long index);
  virtual long      readTachometer(unsigned long index);
  
  virtual const char *  getModelName();
  
public:
  virtual bool      init(OSDictionary *properties=0);
  virtual IOService*    probe(IOService *provider, SInt32 *score);
  virtual bool      start(IOService *provider);
  virtual void      stop(IOService *provider);
  virtual void      free(void);
};
