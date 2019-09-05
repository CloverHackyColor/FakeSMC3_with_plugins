/*
 *  IT8718F.cpp
 *  HWSensors
 *
 *  Created by mozo on 08/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *  Modified by Navi 2012
 *
 */

#include <architecture/i386/pio.h>
#include "ITEIT87x.h"
#include "../../../fakesmc/FakeSMC.h"
//#include "FakeSMCUtils.h"
#include "../../../utils/utils.h"

#define Debug FALSE

#define LogPrefix "ITEIT87x: "
#define DebugLog(string, args...)	do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)	do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super SuperIOMonitor
OSDefineMetaClassAndStructors(IT87x, SuperIOMonitor)

OSDefineMetaClassAndStructors(IT87xSensor, SuperIOSensor)

#pragma mark IT87xSensor implementation

SuperIOSensor * IT87xSensor::withOwner(SuperIOMonitor *aOwner,
                                       const char* aKey,
                                       const char* aType,
                                       unsigned char aSize,
                                       SuperIOSensorGroup aGroup,
                                       unsigned long aIndex,
                                       long aRi,
                                       long aRf,
                                       long aVf) {
	SuperIOSensor *me = new IT87xSensor;
  //  DebugLog("with owner mults = %ld", aRi);
  if (me && !me->initWithOwner(aOwner, aKey, aType, aSize, aGroup, aIndex ,aRi,aRf,aVf)) {
    me->release();
    return 0;
  }
	
  return me;
}

long IT87xSensor::getValue() {
  UInt16 value = 0;
  switch (group) {
    case kSuperIOTemperatureSensor:
      value = owner->readTemperature(index);
      break;
    case kSuperIOVoltageSensor:
      value = owner->readVoltage(index);
      break;
    case kSuperIOTachometerSensor:
      value = owner->readTachometer(index);
      break;
    default:
      switch ((SuperIOSensorGroupEx)group) {
        case kSuperIOSmartGuardPWMControl:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardPWMControl(index);
          break;
        case kSuperIOSmartGuardTempFanStop:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardTempFanStop(index);
          break;
        case kSuperIOSmartGuardTempFanStart:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardTempFanStart(index);
          break;
        case kSuperIOSmartGuardTempFanFullOn:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardTempFanFullOn(index);
          break;
        case kSuperIOSmartGuardPWMStart:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardPWMStart(index);
          break;
        case kSuperIOSmartGuardTempFanFullOff:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardTempFanFullOff(index);
          break;
        case kSuperIOSmartGuardTempFanControl:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardFanControl(index);
          break;
        case kSuperIOSmartGuardMainControl:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardMainControl(index);
          break;
        case kSuperIOSmartGuardRegControl:
          value = OSDynamicCast(IT87x, owner)->readSmartGuardRegControl(index);
          break;
          
        default:
          break;
      }
  }
  
  if (Rf == 0) {
    Rf = 1;
    WarningLog("Rf == 0 when getValue index=%d value=%04x", (int)index, value);
  }
  //  DebugLog("value = %ld Ri=%ld Rf=%ld", (long)value, Ri, Rf);
  value =  value + ((value - Vf) * Ri)/Rf;
  
  if (*((uint32_t*)type) == *((uint32_t*)TYPE_FP2E)) {
    value = encode_fp2e(value);
  } else if (*((uint32_t*)type) == *((uint32_t*)TYPE_SP4B)) {
    value = encode_sp4b(value);
  } else if (*((uint32_t*)type) == *((uint32_t*)TYPE_FPE2)) {
    value = encode_fpe2(value);
  }
  
  //  value = encodeValue(value, scale);
  
  return value;
}


void IT87xSensor::setValue(UInt16 value) {
	switch ((SuperIOSensorGroupEx)group) {
    case kSuperIOSmartGuardPWMControl:
      OSDynamicCast(IT87x, owner)->writeSmartGuardPWMControl(index,value);
      break;
    case kSuperIOSmartGuardTempFanStop:
      OSDynamicCast(IT87x, owner)->writeSmartGuardTempFanStop(index,value);
      break;
    case kSuperIOSmartGuardTempFanStart:
      OSDynamicCast(IT87x, owner)->writeSmartGuardTempFanStart(index,value);
      break;
    case kSuperIOSmartGuardTempFanFullOn:
      OSDynamicCast(IT87x, owner)->writeSmartGuardTempFanFullOn(index,value);
      break;
    case kSuperIOSmartGuardPWMStart:
      OSDynamicCast(IT87x, owner)->writeSmartGuardPWMStart(index,value);
      break;
    case kSuperIOSmartGuardTempFanFullOff:
      OSDynamicCast(IT87x, owner)->writeSmartGuardTempFanFullOff(index,value);
      break;
    case kSuperIOSmartGuardTempFanControl:
      OSDynamicCast(IT87x, owner)->writeSmartGuardFanControl(index,value);
      break;
    case kSuperIOSmartGuardMainControl:
      OSDynamicCast(IT87x, owner)->writeSmartGuardMainControl(index,value);
      break;
    case kSuperIOSmartGuardRegControl:
      OSDynamicCast(IT87x, owner)->writeSmartGuardRegControl(index,value);
      break;
		default:
			break;
	}
  
  // Damn... need someone to explain the encoding scheme of SMC keys... but actually works well without it
  //	if (*((uint32_t*)type) == *((uint32_t*)TYPE_FP2E)) {
  //		value = encode_fp2e(value);
  //	}
  //	else if (*((uint32_t*)type) == *((uint32_t*)TYPE_FPE2)) {
  //		value = encode_fpe2(value);
  //	}
  //
  //	return value;
}

#pragma mark IT87x implementation

long IT87x::readSmartGuardPWMControl(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_PWM_CONTROL[index]);
}


void IT87x::writeSmartGuardPWMControl(unsigned long index,UInt16 value) {
	writeByte(address,ITE_SMARTGUARDIAN_PWM_CONTROL[index], value);
}

long IT87x::readSmartGuardTempFanStop(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_TEMPERATURE_STOP[index]);
}

void IT87x::writeSmartGuardTempFanStop(unsigned long index,UInt16 value) {
	writeByte(address,ITE_SMARTGUARDIAN_TEMPERATURE_STOP[index], value);
}

long IT87x::readSmartGuardTempFanStart(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_TEMPERATURE_START[index]);
}

void IT87x::writeSmartGuardTempFanStart(unsigned long index,UInt16 value) {
  writeByte(address, ITE_SMARTGUARDIAN_TEMPERATURE_START[index], value);
}

long IT87x::readSmartGuardTempFanFullOn(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_TEMPERATURE_FULL_ON[index]);
}

void IT87x::writeSmartGuardTempFanFullOn(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_TEMPERATURE_FULL_ON[index], value);
}

long IT87x::readSmartGuardPWMStart(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_START_PWM[index]);
}

void IT87x::writeSmartGuardPWMStart(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_START_PWM[index], value);
}

long IT87x::readSmartGuardTempFanFullOff(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_TEMPERATURE_DELTA[index]);
}

void IT87x::writeSmartGuardTempFanFullOff(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_TEMPERATURE_DELTA[index], value);
}

long IT87x::readSmartGuardFanControl(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_CONTROL[index]);
}

void IT87x::writeSmartGuardFanControl(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_CONTROL[index], value);
}

long IT87x::readSmartGuardMainControl(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_MAIN_CONTROL);
}

void IT87x::writeSmartGuardMainControl(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_MAIN_CONTROL, value);
}

long IT87x::readSmartGuardRegControl(unsigned long index) {
	return readByte(address, ITE_SMARTGUARDIAN_REG_CONTROL);
}

void IT87x::writeSmartGuardRegControl(unsigned long index,UInt16 value) {
  writeByte(address,ITE_SMARTGUARDIAN_REG_CONTROL, value);
}

long IT87x::readTemperature(unsigned long index) {
	return readByte(address, ITE_TEMPERATURE_BASE_REG + index);
}

long IT87x::readVoltage(unsigned long index) {
  // Refresh VBAT reading on each access to the key
  if (vbat_updates) {
    writeByte(address, ITE_CONFIGURATION_REGISTER, readByte(address, ITE_CONFIGURATION_REGISTER) | 0x40);
  }
  return readByte(address, ITE_VOLTAGE_REG[index]) * voltageGain;
  
}

long IT87x::readTachometer(unsigned long index) {
	int value = readByte(address, ITE_FAN_TACHOMETER_REG[index]);
	
	value |= readByte(address, ITE_FAN_TACHOMETER_EXT_REG[index]) << 8;
	
	return (value > 0x3f && value < 0xffff) ? (float)(1350000 + value) / (float)(value * 2) : 0;
}

bool IT87x::probePort() {
  UInt16 id = listenPortWord(SUPERIO_CHIP_ID_REGISTER);
  
  if (id == 0 || id == 0xffff) {
    //DebugLog("invalid super I/O chip ID=0x%x", id);
    return false;
  }
  
  hasSmartGuardian = false;
  switch (id) {
    case IT8512F:
    case IT8613F:
    case IT8620F:
    case IT8628F:
    case IT8655F:
    case IT8665F:
    case IT8686E:
    case IT8688E:
    case IT8705F:
    case IT8708F:
    case IT8712F:
    case IT8716F:
    case IT8718F:
    case IT8720F:
    case IT8721F:
    case IT8726F:
    case IT8728F:
    case IT8752F:
    case IT8771E:
    case IT8772E:
    case IT8792E:
    case IT8795E:
    case IT8987E:
      model = id;
      break;
    default:
      DebugLog("found unsupported chip ID=0x%x", id);
      return false;
  }
  
  selectLogicalDevice(ITE_ENVIRONMENT_CONTROLLER_LDN);
  
  IOSleep(50);
  
  if (!getLogicalDeviceAddress()) {
    DebugLog("can't get monitoring LDN address");
    return false;
  }
  
  UInt8 vendor16 = readByte(address, ITE_VENDOR_ID_REGISTER);
  
  if (vendor16 != ITE_VENDOR_ID) {
    DebugLog("invalid vendor ID=0x%x", vendor16);
    return false;
  }
  
  if ((readByte(address, ITE_CONFIGURATION_REGISTER) & 0x10) == 0) {
    DebugLog("invalid configuration register value");
    return false;
  }
  
  if (id == IT8721F || id == IT8726F || id == IT8728F || id == IT8752F ||
      id == IT8771E || id == IT8772E || id == IT8792E || id == IT8987E ||
      id == IT8620F || id == IT8628F || id == IT8613F || id == IT8795E ||
      id == IT8655F || id == IT8665F || id == IT8686E || id == IT8688E) {
    voltageGain = 12;
  } else {
    voltageGain = 16;
  }
  UInt8 version = readByte(address, ITE_VERSION_REGISTER) & 0x0F;
  
  if (id == IT8712F && version < 8) {
    has16bitFanCounter = false;
  } else {
    has16bitFanCounter = true;
  }
  return true;
}

void IT87x::enter() {
	outb(registerPort, 0x87);
	outb(registerPort, 0x01);
	outb(registerPort, 0x55);
	
	if (registerPort == 0x4e) {
		outb(registerPort, 0xaa);
	} else {
		outb(registerPort, 0x55);
	}
}

void IT87x::exit() {
	outb(registerPort, SUPERIO_CONFIGURATION_CONTROL_REGISTER);
	outb(valuePort, 0x02);
}

const char *IT87x::getModelName() {
  switch (model) {
    case IT8512F: return "IT8512F";
    case IT8613F: return "IT8613F";
    case IT8620F: return "IT8620F";
    case IT8628F: return "IT8628F";
    case IT8655F: return "IT8655F";
    case IT8665F: return "IT8665F";
    case IT8686E: return "IT8686E";
    case IT8688E: return "IT8688E";
    case IT8705F: return "IT8705F";
    case IT8708F: return "IT8708F";
    case IT8712F: return "IT8712F";
    case IT8716F: return "IT8716F";
    case IT8718F: return "IT8718F";
    case IT8720F: return "IT8720F";
    case IT8721F: return "IT8721F";
    case IT8726F: return "IT8726F";
    case IT8728F: return "IT8728F";
    case IT8752F: return "IT8752F";
    case IT8771E: return "IT8771E";
    case IT8772E: return "IT8772E";
    case IT8792E: return "IT8792E";
    case IT8795E: return "IT8795E";
    case IT8987E: return "IT8987E";
  }
  
  return "unknown";
}

bool IT87x::init(OSDictionary *properties) {
  //  DebugLog("initialising...");
  if (!super::init(properties))
    return false;
  
  return true;
}

IOService* IT87x::probe(IOService *provider, SInt32 *score) {
  //  DebugLog("probing...");
  if (super::probe(provider, score) != this)
    return 0;
  
  return this;
}

void IT87x::stop (IOService* provider) {
  DebugLog("stoping...");
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

void IT87x::free() {
  //DebugLog("freeing...");
	super::free();
}

bool IT87x::start(IOService * provider) {
  DebugLog("starting ...");
  
  if (!super::start(provider)) {
    return false;
  }
  
  InfoLog("found ITE %s", getModelName());
  OSDictionary* list = OSDynamicCast(OSDictionary, getProperty("Sensors Configuration"));
  
  OSDictionary *configuration=NULL;
  IORegistryEntry * rootNode = fromPath("/efi/platform", gIODTPlane);
  
  if (rootNode) {
    OSData *data = OSDynamicCast(OSData, rootNode->getProperty("OEMVendor"));
    if (data) {
      bcopy(data->getBytesNoCopy(), vendor, data->getLength());
      OSString * VendorNick = vendorID(OSString::withCString(vendor));
      if (VendorNick) {
        data = OSDynamicCast(OSData, rootNode->getProperty("OEMBoard"));
        if (!data) {
          WarningLog("no OEMBoard");
          data = OSDynamicCast(OSData, rootNode->getProperty("OEMProduct"));
        }
        
        if (data) {
          bcopy(data->getBytesNoCopy(), product, data->getLength());
          OSDictionary *link = OSDynamicCast(OSDictionary, list->getObject(VendorNick));
          
          if (link) {
            configuration = OSDynamicCast(OSDictionary, link->getObject(OSString::withCString(product)));
            InfoLog(" mother vendor=%s product=%s", vendor, product);
          }
        }
      } else {
        WarningLog("unknown OEMVendor %s", vendor);
      }
    } else {
      WarningLog("no OEMVendor");
    }
  }
  
  if (list && !configuration) {
    configuration = OSDynamicCast(OSDictionary, list->getObject("Default"));
    WarningLog("set default configuration");
  }
  
  if (configuration) {
    this->setProperty("Current Configuration", configuration);
  }
  
  // Temperature Sensors
  if (configuration) {
    for (int i = 0; i < 3; i++) {
      char key[8];
      
      snprintf(key, 8, "TEMPIN%X", i);
      if (readTemperature(i)<MAX_TEMP_THRESHOLD) { // Need to check if temperature sensor valid
        if (OSString* name = OSDynamicCast(OSString, configuration->getObject(key))) {
          if (name->isEqualTo("CPU")) {
            if (!addSensor(KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, i)) {
              WarningLog("error adding heatsink temperature sensor");
            }
          } else if (name->isEqualTo("System")) {
            if (!addSensor(KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor,i)) {
              WarningLog("error adding system temperature sensor");
            }
          } else if (name->isEqualTo("Ambient")) {
            if (!addSensor(KEY_AMBIENT_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor,i)) {
              WarningLog("error adding Ambient temperature sensor");
            }
          } else if (name->isEqualTo("Memory")) {
            if (!addSensor(KEY_DIMM_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor,i)) {
              WarningLog("error adding Memory temperature sensor");
            }
          }
        }
      }
    }
  } else {
    if (readTemperature(0)<MAX_TEMP_THRESHOLD)  // Need to check if temperature sensor valid
      if (!addSensor(KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, 0)) {
        WarningLog("error adding heatsink temperature sensor");
      }
    if (readTemperature(1)<MAX_TEMP_THRESHOLD)  // Need to check if temperature sensor valid
      if (!addSensor(KEY_DIMM_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, 1)) {
        WarningLog("error adding Ambient temperature sensor");
      }
    if (readTemperature(2)<MAX_TEMP_THRESHOLD)  // Need to check if temperature sensor valid
      if (!addSensor(KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, 2, kSuperIOTemperatureSensor, 2)) {
        WarningLog("error adding system temperature sensor");
      }
  }
  
  
  // Voltage
  
  UInt8 tmp = readByte(address, ITE_ADC_CHANNEL_ENABLE);
  IOLog (LogPrefix "[Debug] ADC Enable register = %X",tmp);
  
  
  vbat_updates = false;
  if (configuration) {
    OSBoolean* smartGuard = OSDynamicCast(OSBoolean, configuration->getObject("VBATNeedUpdates"));
    if (smartGuard && smartGuard->isTrue())
      vbat_updates=true;
  }
  // Refresh VBAT reading on each access to the key
  if (vbat_updates) {
    writeByte(address, ITE_CONFIGURATION_REGISTER, readByte(address, ITE_CONFIGURATION_REGISTER) | 0x40);
  }
  
  if (configuration) {
    for (int i = 0; i < 9; i++) {
      char key[5];
      OSString * name;
      long Ri=0;
      long Rf=1;
      long Vf=0;
      
      snprintf(key, 5, "VIN%X", i);
      
      if (process_sensor_entry(configuration->getObject(key), &name, &Ri, &Rf, &Vf)) {
        //DebugLog("Entry %d name=%s Ri=%ld", i, name->getCStringNoCopy(), Ri);
        if (name->isEqualTo("CPU")) {
          if (!addSensor(KEY_CPU_VRM_SUPPLY0, TYPE_FP2E, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("error adding CPU voltage sensor");
          }
        } else if (name->isEqualTo("Memory")) {
          if (!addSensor(KEY_MEMORY_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("error adding memory voltage sensor");
          }
        } else if (name->isEqualTo("+5VC")) {
          if (!addSensor(KEY_5VC_VOLTAGE, TYPE_SP4B, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding AVCC Voltage Sensor!");
          }
        } else if (name->isEqualTo("+5VSB")) {
          if (!addSensor(KEY_5VSB_VOLTAGE, TYPE_SP4B, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding AVCC Voltage Sensor!");
          }
        } else if (name->isEqualTo("+12VC")) {
          if (!addSensor(KEY_12V_VOLTAGE, TYPE_SP4B, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding 12V Voltage Sensor!");
          }
        } else if (name->isEqualTo("-12VC")) {
          if (!addSensor(KEY_N12VC_VOLTAGE, TYPE_SP4B, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding 12V Voltage Sensor!");
          }
        } else if (name->isEqualTo("3VCC")) {
          if (!addSensor(KEY_3VCC_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding 3VCC Voltage Sensor!");
          }
        } else if (name->isEqualTo("3VSB")) {
          if (!addSensor(KEY_3VSB_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding 3VSB Voltage Sensor!");
          }
        } else if (name->isEqualTo("VBAT")) {
          if (!addSensor(KEY_VBAT_VOLTAGE, TYPE_FP2E, 2, kSuperIOVoltageSensor, i,Ri,Rf,Vf)) {
            WarningLog("ERROR Adding VBAT Voltage Sensor!");
          }
        }
      }
    }
  }
  
  // Tachometers
  for (int i = 0; i < 5; i++) {
    OSString* name = NULL;
    
    if (configuration) {
      char key_temp[7];
      snprintf(key_temp, 7, "FANIN%X", i);
      name = OSDynamicCast(OSString, configuration->getObject(key_temp));
    }
    
    UInt32 nameLength = name ? (UInt32)strlen(name->getCStringNoCopy()) : 0;
    
    if (readTachometer(i) > 10 || nameLength > 0) {
      // Pff WTF ??? Add tachometer if it doesn't exist in a system but only the name defined in the config???
      
      if (!addTachometer(i, (nameLength > 0 ? name->getCStringNoCopy() : 0))) {
        // Need to look at this a bit later
        WarningLog("error adding tachometer sensor %d", i);
      }
    }
    
    // Check if this chip support SmartGuardian feature
    
    hasSmartGuardian = false;
    if (configuration) {
      if (OSBoolean* smartGuard=OSDynamicCast(OSBoolean, configuration->getObject("SmartGuardian"))) {
        if (smartGuard->isTrue()) {
          hasSmartGuardian = true;
        }
      }
      
    }
    
    if (hasSmartGuardian) {
      char key[5];
      // Ugly development hack started for (SuperIOSensorGroup)
      snprintf(key,5,KEY_FORMAT_FAN_TARGET_SPEED,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardPWMControl, i)) {
        WarningLog("error adding PWM fan control");
      }
      
      snprintf(key,5,KEY_FORMAT_FAN_START_TEMP,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardTempFanStart, i)) {
        WarningLog("error adding start temp fan control");
      }
      
      snprintf(key,5,KEY_FORMAT_FAN_OFF_TEMP,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardTempFanStop, i)) {
        WarningLog("error adding stop temp fan control");
      }
      
      snprintf(key,5,KEY_FORMAT_FAN_FULL_TEMP,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardTempFanFullOn, i)) {
        WarningLog("error adding full speed temp fan control");
      }
      snprintf(key,5,KEY_FORMAT_FAN_START_PWM,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardPWMStart, i)) {
        WarningLog("error adding start PWM fan control");
      }
      
      snprintf(key,5,KEY_FORMAT_FAN_TEMP_DELTA,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardTempFanFullOff, i)) {
        WarningLog("error adding temp full off fan control");
      }
      
      snprintf(key,5,KEY_FORMAT_FAN_CONTROL,i);
      if (!addSensor(key, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardTempFanControl, i)) {
        WarningLog("error adding register fan control");
      }
    }
  }
  
  if (hasSmartGuardian) {
    if (!addSensor(KEY_FORMAT_FAN_MAIN_CONTROL, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardMainControl, 0)) {
      WarningLog("error adding Main fan control");
    }
    
    if (!addSensor(KEY_FORMAT_FAN_REG_CONTROL, TYPE_UI8, 1, (SuperIOSensorGroup)kSuperIOSmartGuardRegControl, 0)) {
      WarningLog("error adding Main fan control");
    }
  }
  
  return true;
}

int IT87x::getPortsCount() {
  return 2;
}


SuperIOSensor * IT87x::addSensor(const char* name,
                                 const char* type,
                                 unsigned int size,
                                 SuperIOSensorGroup group,
                                 unsigned long index,
                                 long aRi,
                                 long aRf,
                                 long aVf) {
  if (NULL != getSensor(name)) {
    return 0;
  }
  //DebugLog("mults = %ld", aRi);
  SuperIOSensor *sensor = IT87xSensor::withOwner(this, name, type, size, group, index, aRi, aRf, aVf);
  
  if (sensor && sensors->setObject(sensor)) {
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                          false,
                                                          (void *)name,
                                                          (void *)type,
                                                          (void *)(long long)size,
                                                          (void *)this)) {
      return sensor;
    }
  }
  return 0;
}

IOReturn IT87x::callPlatformFunction(const OSSymbol *functionName,
                                     bool waitForFunction,
                                     void *param1,
                                     void *param2,
                                     void *param3,
                                     void *param4) {
  
  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    const char* name = (const char*)param1;
    void * data = param2;
    //UInt32 size = (UInt64)param3;
    
    if (name && data) {
      SuperIOSensor * sensor = getSensor(name);
      if (sensor) {
        UInt16 value = sensor->getValue();
        bcopy(&value, data, 2);
        return kIOReturnSuccess;
      }
    }
    
    return kIOReturnBadArgument;
  }
  
  if (functionName->isEqualTo(kFakeSMCSetValueCallback)) {
    const char* name = (const char*)param1;
    void * data = param2;
    //UInt32 size = (UInt64)param3;
    
    if (name && data) {
      IT87xSensor *sensor = OSDynamicCast(IT87xSensor, getSensor(name));
      if (sensor) {
        UInt16 value;
        bcopy(data, &value, 2);
        sensor->setValue(value);
        return kIOReturnSuccess;
      }
    }
    return kIOReturnBadArgument;
  }
  
  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}
