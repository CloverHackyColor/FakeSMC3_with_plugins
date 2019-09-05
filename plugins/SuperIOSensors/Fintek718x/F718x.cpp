/*
 *  F718x.cpp
 *  HWSensors
 *
 *  Created by mozo on 16/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#include "FakeSMC.h"
#include "F718x.h"
#include <architecture/i386/pio.h>

#define Debug FALSE

#define LogPrefix "F718x: "
#define DebugLog(string, args...)	do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)	do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super SuperIOMonitor
OSDefineMetaClassAndStructors(F718x, SuperIOMonitor)

UInt8 F718x::readByte(UInt8 reg) {
	outb(address + FINTEK_ADDRESS_REGISTER_OFFSET, reg);
	return inb(address + FINTEK_DATA_REGISTER_OFFSET);
} 

long F718x::readTemperature(unsigned long index) {
  float value;
  
  switch (model) {
    case F71858: {
      int tableMode = 0x3 & readByte(FINTEK_TEMPERATURE_CONFIG_REG);
      int high = readByte(FINTEK_TEMPERATURE_BASE_REG + 2 * index);
      int low = readByte(FINTEK_TEMPERATURE_BASE_REG + 2 * index + 1);
      
      if (high != 0xbb && high != 0xcc)  {
        int bits = 0;
        
        switch (tableMode) {
          case 0: bits = 0; break;
          case 1: bits = 0; break;
          case 2: bits = (high & 0x80) << 8; break;
          case 3: bits = (low & 0x01) << 15; break;
        }
        bits |= high << 7;
        bits |= (low & 0xe0) >> 1;
        
        short val = (short)(bits & 0xfff0);
        
        return (float)val / 128.0f;
      } else {
        return 0;
      }
    }
      break;
    default: {
      value = readByte(FINTEK_TEMPERATURE_BASE_REG + 2 * (index + 1));
    }
      break;
  }
  
  return value;
}

long F718x::readVoltage(unsigned long index) {
	//UInt16 raw = readByte(FINTEK_VOLTAGE_BASE_REG + index);
	//if (index == 0) m_RawVCore = raw;
	
	float V = (index == 1 ? 0.5f : 1.0f) * (readByte(FINTEK_VOLTAGE_BASE_REG + index) << 4); // * 0.001f Exclude by trauma
	
	return V;
}

long F718x::readTachometer(unsigned long index) {
	int value = readByte(FINTEK_FAN_TACHOMETER_REG[index]) << 8;
	value |= readByte(FINTEK_FAN_TACHOMETER_REG[index] + 1);
	
  if (value > 0) {
    value = (value < 0x0fff) ? 1.5e6f / value : 0;
  }
	
	return value;
}


void F718x::enter() {
	outb(registerPort, 0x87);
	outb(registerPort, 0x87);
}

void F718x::exit() {
	outb(registerPort, 0xAA);
	outb(registerPort, SUPERIO_CONFIGURATION_CONTROL_REGISTER);
	outb(valuePort, 0x02);
}

bool F718x::probePort() {
  UInt8 logicalDeviceNumber = 0;
  
  UInt8 id = listenPortByte(FINTEK_CHIP_ID_REGISTER);
  UInt8 revision = listenPortByte(FINTEK_CHIP_REVISION_REGISTER);
  
  if (id == 0 || id == 0xff || revision == 0 || revision == 0xff) {
    return false;
  }
  
  switch (id) {
    case 0x05: {
      switch (revision) {
        case 0x07:
          model = F71858;
          logicalDeviceNumber = F71858_HARDWARE_MONITOR_LDN;
          break;
        case 0x41:
          model = F71882;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x06: {
      switch (revision) {
        case 0x01:
          model = F71862;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x07: {
      switch (revision) {
        case 0x23:
          model = F71889F;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x08: {
      switch (revision) {
        case 0x14:
          model = F71869;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x09: {
      switch (revision) {
        case 0x01:                                                      /*Add F71808 */
          model = F71808;                                         /*Add F71808 */
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;         /*Add F71808 */
          break;                                                    /*Add F71808 */
        case 0x09:
          model = F71889ED;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x10: {
      switch (revision) {
        case 0x05:
          model = F71889AD;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
    case 0x11: {
      switch (revision) {
        case 0x06:
          model = F71868A;
          logicalDeviceNumber = FINTEK_HARDWARE_MONITOR_LDN;
          break;
      }
    }
      break;
  }
  
  if (!model) {
    InfoLog("Fintek: Found unsupported chip ID=0x%x REVISION=0x%x", id, revision);
    return false;
  }
  
  selectLogicalDevice(logicalDeviceNumber);
  
  if (!getLogicalDeviceAddress()) {
    return false;
  }
  
  return true;
}

const char *F718x::getModelName() {
	switch (model)  {
    case F71858:   return "F71858";
    case F71862:   return "F71862";
    case F71869:   return "F71869";
    case F71882:   return "F71882";
    case F71889ED: return "F71889ED";
    case F71889AD: return "F71889AD";
    case F71889F:  return "F71889F";
		case F71808:   return "F71808";	
	}
	
	return "unknown";
}

bool F718x::init(OSDictionary *properties) {
  //DebugLog("initialising...");
  if (!super::init(properties)) {
    return false;
  }
  
  return true;
}

IOService* F718x::probe(IOService *provider, SInt32 *score) {
  //  DebugLog("probing...");
  
  if (super::probe(provider, score) != this) {
    return 0;
  }
  
  InfoLog("based on code from Open Hardware Monitor project by Michael Möller (C) 2010");
  InfoLog("mozodojo (C) 2011");
  
  return this;
}

bool F718x::start(IOService * provider) {
  DebugLog("starting...");
  
  if (!super::start(provider)) {
    return false;
  }
  
  InfoLog("found Fintek %s", getModelName());
  
  OSDictionary* configuration = OSDynamicCast(OSDictionary, getProperty("Sensors Configuration"));
  
  // Heatsink
  if (!addSensor(KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, 0)) {
    WarningLog("error adding heatsink temperature sensor");
  }
  // Northbridge
  if (!addSensor(KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, 1)) {
    WarningLog("error adding system temperature sensor");
  }
  
  // Voltage
  switch (model)  {
    case F71858:
      break;
    default:
      // CPU Vcore
      if (!addSensor(KEY_CPU_VOLTAGE_RAW, TYPE_FP2E, 2, kSuperIOVoltageSensor, 1)) {
        WarningLog("error adding CPU voltage sensor");
      }
      break;
  }
  
  // Tachometers
  for (int i = 0; i < (model == F71882 ? 4 : 3); i++) {
    OSString* name = 0;
    
    if (configuration) {
      char key[7];
      snprintf(key, 7, "FANIN%X", i);
      name = OSDynamicCast(OSString, configuration->getObject(key));
    }
    
    size_t nameLength = name ? strlen(name->getCStringNoCopy()) : 0;
    
    if (readTachometer(i) > 10 || nameLength > 0) {
      if (!addTachometer(i, (nameLength > 0 ? name->getCStringNoCopy() : 0))) {
        WarningLog("error adding tachometer sensor %d", i);
      }
    }
  }
  
  return true;
}

void F718x::stop(IOService* provider) {
//	DebugLog("stoping...");
  if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCRemoveKeyHandler,
                                                        true,
                                                        this,
                                                        NULL,
                                                        NULL,
                                                        NULL)) {
    WarningLog("Can't remove key handler");
    IOSleep(500);
  }
	
	super::stop(provider);
}

void F718x::free() {
	//DebugLog("freeing...");
	super::free();
}
