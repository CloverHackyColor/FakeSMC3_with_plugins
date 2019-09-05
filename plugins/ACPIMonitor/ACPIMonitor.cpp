/*
 *  ACPIMonitor.cpp
 *  HWSensors
 *
 *  Created by mozo on 12/11/10.
 *  Copyright 2010 Slice. All rights reserved.
 *
 */

#include "ACPIMonitor.h"
#include "../../fakesmc/FakeSMC.h"
#include "../../utils/utils.h"

#define Debug FALSE

#define LogPrefix "ACPIMonitor: "
#define DebugLog(string, args...)	do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)	do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super IOService
OSDefineMetaClassAndStructors(ACPIMonitor, IOService)

bool ACPIMonitor::addSensor(const char* method, const char* key, const char* type, unsigned long long size) {
	if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false,
                                                        (void *)key,
                                                        (void *)type,
                                                        (void *)size,
                                                        (void *)this)) {
    if (sensors && sensors->setObject(key, OSString::withCString(method))) {
      InfoLog("%s registered", method);
      return true;
    }
  }
	return false;
}
// for example addTachometer("FAN0", "System Fan");
bool ACPIMonitor::addTachometer(const char* method, const char* id) {
	UInt8 length = 0;
	void * data = 0;
	
	if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                        true,
                                                        (void *)KEY_FAN_NUMBER,
                                                        (void *)&length,
                                                        (void *)&data,
                                                        0)) {
		length = 0;
		
		bcopy(data, &length, 1);
		
		char name[5];
		
		snprintf(name, 5, KEY_FORMAT_FAN_SPEED, length);
		
		if (addSensor(method, name, TYPE_FPE2, 2)) {
			if (id) {
        FanTypeDescStruct fds;
		snprintf(name, 5, KEY_FORMAT_FAN_ID, length);
        fds.type = FAN_PWM_TACH;
        fds.ui8Zone = 1;
        fds.location = LEFT_LOWER_FRONT;
        strncpy(fds.strFunction, id, DIAG_FUNCTION_STR_LEN);
        
        if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCAddKeyValue,
                                                              false,
                                                              (void *)name,
                                                              (void *)TYPE_FDESC,
                                                              (void *)((UInt64)sizeof(fds)),
                                                              (void *)&fds)) {
          
			WarningLog("error adding tachometer id value");
        }
			}
			length++;
			
			if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCSetKeyValue,
                                                            true,
                                                            (void *)KEY_FAN_NUMBER,
                                                            (void *)1,
                                                            (void *)&length, 0)) {
        WarningLog("error updating FNum value");
      }
			
			return true;
		}
  } else {
    WarningLog("error reading FNum value");
  }
  
	return false;
}

IOService* ACPIMonitor::probe(IOService *provider, SInt32 *score) {
  if (super::probe(provider, score) != this) { return 0; }
	
	return this;
}

bool ACPIMonitor::start(IOService * provider) {
  if (!provider || !super::start(provider)) { return false; }
	
	if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
		WarningLog("Can't locate fake SMC device, kext will not load");
		return false;
	}
	char key[5];
	acpiDevice = (IOACPIPlatformDevice *)provider;
  
	//Here is Fan in ACPI
	OSArray* fanNames = OSDynamicCast(OSArray, getProperty("FanNames"));
	
	for (int i=0; i<10; i++) {
		snprintf(key, 5, "FAN%X", i);
		
		if (kIOReturnSuccess == acpiDevice->validateObject(key)) {
			OSString* name = NULL;
			
      if (fanNames) {
				name = OSDynamicCast(OSString, fanNames->getObject(i));
      }
      
      if (!addTachometer(key, name ? name->getCStringNoCopy() : 0)) {
				WarningLog("Can't add tachometer sensor, key %s", key);
      }
		} else {
			snprintf(key, 5, "FTN%X", i);
			if (kIOReturnSuccess == acpiDevice->validateObject(key)){
				OSString* name = NULL;
				
        if (fanNames) {
					name = OSDynamicCast(OSString, fanNames->getObject(i));
        }
        
        if (!addTachometer(key, name ? name->getCStringNoCopy() : 0)) {
					WarningLog("Can't add tachometer sensor, key %s", key);
        }
      } else {
        break;
      }
		}
	}
  
	//Next step - temperature keys
  if (kIOReturnSuccess == acpiDevice->validateObject("TCPU")) {
		addSensor("TCPU", KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, 2); //Th0H
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("TSYS")) {
		addSensor("TSYS", KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, 2); //TN0P
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("TDIM")) {
		addSensor("TDIM", KEY_DIMM_TEMPERATURE, TYPE_SP78, 2);      //Tm0P
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("TAMB")) {
		addSensor("TAMB", KEY_AMBIENT_TEMPERATURE, TYPE_SP78, 2); //TA0P
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("TCPP")) {
    addSensor("TCPP", KEY_CPU_PROXIMITY_TEMPERATURE, TYPE_SP78, 2); //TC0P
  }
	// We should add also GPU reading stuff for those who has no supported plug in but have the value on EC registers
	
	
	
	//Voltage
  if (kIOReturnSuccess == acpiDevice->validateObject("VCPU")) {
		addSensor("VCPU", KEY_CPU_VOLTAGE, TYPE_FP2E, 2); //VC0C
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("VMEM")) {
		addSensor("VMEM", KEY_MEMORY_VOLTAGE, TYPE_FP2E, 2); //VM0R
  }
    
  if (kIOReturnSuccess == acpiDevice->validateObject("VP0R")) {
		addSensor("VP0R", KEY_12V_VOLTAGE, TYPE_SP4B, 2); //VP0R
  }

  if (kIOReturnSuccess == acpiDevice->validateObject("VSN0")) {
		addSensor("VSN0", KEY_N12VC_VOLTAGE, TYPE_SP4B, 2); //Vp0C
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("VSN1")) {
		addSensor("VSN1", KEY_5VC_VOLTAGE, TYPE_SP4B, 2); //Vp1C
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("VSN2")) {
		addSensor("VSN2", KEY_5VSB_VOLTAGE, TYPE_SP4B, 2); //Vp2C"
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("VSN3")) {
		addSensor("VSN3", KEY_3VCC_VOLTAGE, TYPE_FP2E, 2); //Vp3C"
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("VSN4")) {
		addSensor("VSN4", KEY_3VSB_VOLTAGE, TYPE_FP2E, 2); //Vp4C"
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("VSN5")) {
		addSensor("VSN5", KEY_AVCC_VOLTAGE, TYPE_FP2E, 2); //Vp5C"
  }
	
	//Amperage
  if (kIOReturnSuccess == acpiDevice->validateObject("ISN0")) {
		addSensor("ISN0", "ICAC", TYPE_UI16, 2);
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("ISN1")) {
		addSensor("ISN1", "Ip0C", TYPE_UI16, 2);
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("ISN2")) {
		addSensor("ISN2", "Ip1C", TYPE_UI16, 2);
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("ISN3")) {
    addSensor("ISN3", "Ip2C", TYPE_UI16, 2);
  }
	
	//Power
  if (kIOReturnSuccess == acpiDevice->validateObject("PSN0")) {
		addSensor("PSN0", "PC0C", TYPE_UI16, 2);
  }
  
  if (kIOReturnSuccess == acpiDevice->validateObject("PSN1")) {
		addSensor("PSN1", "PC1C", TYPE_UI16, 2);
  }
	
	// AC Power/Battery
  if (kIOReturnSuccess == acpiDevice->validateObject("ACDC")) { // Power Source Read AC/Battery
		addSensor("ACDC", "ACEN", TYPE_UI8, 1);
		addSensor("ACDC", "ACFP", TYPE_FLAG, 1);
		addSensor("ACDC", "ACIN", TYPE_FLAG, 1);
	}
	// TODO real SMC returns ACID only when AC is plugged, if not is zeroed, so hardcoding it in plist is not OK IMHO
	// Same goes for ACIC, but no idea how we can get the AC current value..
	
	// Here if ACDC returns 0 we need to set the on battery BATP flag
	
	// Battery stuff, need to implement rest of the keys once I figure those
  if (kIOReturnSuccess == acpiDevice->validateObject("BAK0")) { // Battery 0 Current
    addSensor("BAK0", "B0AC", TYPE_SI16, 2);
  }
	
  if (kIOReturnSuccess == acpiDevice->validateObject("BAK1")) { // Battery 0 Voltage
    addSensor("BAK1", "B0AV", TYPE_UI16, 2);
  }
	
	//Keys from info.plist
	OSDictionary *keysToAdd = 0;
	
	keysToAdd = OSDynamicCast(OSDictionary, getProperty("keysToAdd"));
	if (keysToAdd) {
		OSIterator *iter = OSCollectionIterator::withCollection(keysToAdd);
		if (iter) {
			const OSSymbol *dictKey = 0;
			while ((dictKey = (const OSSymbol *)iter->getNextObject())) {
				char acpiName[5];
				snprintf(acpiName, 5, "%s", dictKey->getCStringNoCopy());
				//WarningLog(" Found key %s", acpiName);
				OSString *tmpString = OSDynamicCast(OSString, keysToAdd->getObject(dictKey));
				if (tmpString) {
					char aKey[8];
					snprintf(aKey, 8, "%s", tmpString->getCStringNoCopy());
					InfoLog("Custom name=%s key=%s", acpiName, aKey);
					if (kIOReturnSuccess == acpiDevice->validateObject(acpiName)) {
						if (aKey[0] == 'F') {
              if (!addTachometer(aKey, acpiName)) {
								WarningLog("Can't add tachometer sensor, key %s", aKey);
              }
						} else {
              const char *Type = TYPE_UI16;
              int Size = 2;
              if (aKey[4] == '-') {
                int Len = aKey[5] - '0';                
                aKey[4] = '\0';
                aKey[5] = '\0';
                switch (Len) {
                  case 0:
                    Type = TYPE_FLAG;
                    Size = 1;
                    break;
                  case 1:
                    Type = TYPE_UI8;
                    Size = 1;
                    break;
                  case 4:
                    Type = TYPE_UI32;
                    Size = 4;
                    break;
                  case 3:
                    Type = TYPE_SP78;
                    Size = 2;
                    break;
                  case 2:
                  default:
                    Type = TYPE_UI16;
                    break;
                }
              }
              addSensor(acpiName, aKey, Type, Size);
						}
					}
				} else {
					WarningLog(" no value for key %s", acpiName);
				}
			}
		} else {
			WarningLog(" can't interate keysToAdd");
		}
	} else {
		WarningLog(" keysToAdd not found");
	}
  
	registerService(0);
  
	return true;
}


bool ACPIMonitor::init(OSDictionary *properties) {
  if (!super::init(properties)) {
    return false;
  }
  
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  
  return true;
}

void ACPIMonitor::stop(IOService* provider) {
	sensors->flushCollection();
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

void ACPIMonitor::free() {
	sensors->release();
	
	super::free();
}

#define MEGA10 10000000ull
IOReturn ACPIMonitor::callPlatformFunction(const OSSymbol *functionName,
                                           bool waitForFunction,
                                           void *param1,
                                           void *param2,
                                           void *param3,
                                           void *param4) {
  const char* name = (const char*)param1;
  void * data = param2;
  UInt64 size = (UInt64)param3;
  //callPlatformFunction(kFakeSMCAddKeyHandler, false, (void *)key, (void *)type, (void *)size, (void *)this))
  OSString* key;
#if __LP64__
  UInt64 value = 0;
#else
  UInt32 value;
#endif
  UInt16 val = 0;

  if (functionName->isEqualTo(kFakeSMCSetValueCallback)) {
    if (name && data) {
      key = OSDynamicCast(OSString, sensors->getObject(name));
      if (key) {
        InfoLog("Writing key=%s by method=%s value=%x", name, key->getCStringNoCopy(), *(UInt16*)data);
        OSObject * params[1];
        if (key->getChar(0) == 'F') {
          val = decode_fpe2(*(UInt16*)data);
        } else {
          val = *(UInt16*)data;
        }
        params[0] = OSDynamicCast(OSObject, OSNumber::withNumber((unsigned long long)val, 32));
        return acpiDevice->evaluateInteger(key->getCStringNoCopy(), &value, params, 1);

        /*
         virtual IOReturn evaluateInteger( const OSSymbol * objectName,
         UInt32 *         resultInt32,
         OSObject *       params[]   = 0,
         IOItemCount      paramCount = 0,
         IOOptionBits     options    = 0 );
         flags_num = OSNumber::withNumber((unsigned long long)flags, 32);
         */

      }
      return kIOReturnBadArgument;
    }
    return kIOReturnBadArgument;
  }

  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    if (name && data) {
      key = OSDynamicCast(OSString, sensors->getObject(name)); //ACPI name
      if (key) {
        if (kIOReturnSuccess == acpiDevice->evaluateInteger(key->getCStringNoCopy(), &value)) {
          val = 0;
          if (key->getChar(0) == 'V') {
            if  (key->getChar(3) <= '2' || key->getChar(3) == 'R')  {
              val = encode_sp4b(value); //VSN 0 1 2 R
            } else {
              val = encode_fp2e(value);
            }
          } else if (key->getChar(0) == 'F') {
            if (key->getChar(1) == 'A') {
              val = encode_fpe2(value);
            } else {
              if (key->getChar(1) == 'T') {
                val = value?encode_fpe2(MEGA10 / value):0;
              } else {
                val = value;
              }
            }
          } else {
            val = value;
          }
          bcopy(&val, data, size);
          return kIOReturnSuccess;
        }
      }
      return kIOReturnBadArgument;
    }

    //DebugLog("bad argument key name or data");
    return kIOReturnBadArgument;
  }

  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}
