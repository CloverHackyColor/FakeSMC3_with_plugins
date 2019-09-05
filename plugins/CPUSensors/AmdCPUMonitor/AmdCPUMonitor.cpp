/*
 *  AmdCPUMonitor.cpp
 *  HWSensors
 *
 *  Copyright 2014 Slice. All rights reserved.
 *  First created at 25.02.2014
 */

#include "AmdCPUMonitor.h"
#include "FakeSMC.h"


#define super IOService
OSDefineMetaClassAndStructors(AmdCPUMonitor, IOService)

bool AmdCPUMonitor::addSensor(const char* key,
                              const char* type,
                              unsigned int size,
                              int index) {
  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false, (void *)key,
                                                        (void *)type,
                                                        (void *)(long long)size,
                                                        (void *)this)) {
    return sensors->setObject(key, OSNumber::withNumber(index, 32));
  }
  return false;
}

IOService* AmdCPUMonitor::probe(IOService *provider, SInt32 *score) {
  if (super::probe(provider, score) != this) { return 0; }
  UInt32 vendor_id = 0, device_id = 0, class_id = 0;
  if (OSDictionary * dictionary = serviceMatching(kGenericPCIDevice)) {
    if (OSIterator * iterator = getMatchingServices(dictionary)) {
      
      IOPCIDevice* device = 0;
      
      while ((device = OSDynamicCast(IOPCIDevice, iterator->getNextObject()))) {
        OSData *data = OSDynamicCast(OSData, device->getProperty(fVendor));
        if (data) {
          vendor_id = *(UInt32*)data->getBytesNoCopy();
        }
        
        data = OSDynamicCast(OSData, device->getProperty(fDevice));
        if (data) {
          device_id = *(UInt32*)data->getBytesNoCopy();
        }
        
        data = OSDynamicCast(OSData, device->getProperty(fClass));
        if (data) {
          class_id = *(UInt32*)data->getBytesNoCopy();
        }
        
        if (((vendor_id==0x1022) && ((device_id & 0xF0FF)== 0x1003)) ||
            ((vendor_id==0x1022) && (device_id== 0x1463))) { //Rizen
          InfoLog("found AMD Miscellaneous Control id=%x class_id=%x", (UInt16)device_id, class_id);
          VCard = device;
        }
      }
    }
  }
  return this;
}

bool AmdCPUMonitor::start(IOService * provider) {
  if (!provider || !super::start(provider)) { return false; }
	
	if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
		WarningLog("Can't locate fake SMC device, kext will not load");
		return false;
	}
  if (!VCard) {
    return false;
  }
	
	char name[5];
	snprintf(name, 5, KEY_CPU_PROXIMITY_TEMPERATURE); 
			
	UInt8 length = 0;
	void * data = 0;
			
	IOReturn result = fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                  true, (void *)name,
                                                  (void *)&length,
                                                  (void *)&data, 0);
			
	if (kIOReturnSuccess == result) {
		WarningLog("Key TC0P already exists, kext will not load");
		return false;    
	}
  
	if (addSensor(name, TYPE_SP78, 2, 0)) {
    InfoLog(" AMD CPU temperature sensor added");
	}
			
	if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false, (void *)name,
                                                        (void *)TYPE_SP78,
                                                        (void *)2, this)) {
		WarningLog("Can't add key to fake SMC device, kext will not load");
		return false;
	}
	
	return true;	
}


bool AmdCPUMonitor::init(OSDictionary *properties) {
  if (!super::init(properties)) {
    return false;
  }
	
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  
	return true;
}

void AmdCPUMonitor::stop (IOService* provider) {
	sensors->flushCollection();
	super::stop(provider);
}

void AmdCPUMonitor::free () {
	sensors->release();
	super::free();
}

IOReturn AmdCPUMonitor::callPlatformFunction(const OSSymbol *functionName,
                                             bool waitForFunction,
                                             void *param1,
                                             void *param2,
                                             void *param3,
                                             void *param4 ) {
  //UInt16 t;
  UInt32 value=0;
  
  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    const char* name = (const char*)param1;
    void* data = param2;
    
    if (name && data) {
      switch (name[0]) {
        case 'T':
          if (strcasecmp(name, KEY_CPU_PROXIMITY_TEMPERATURE) == 0) {
            value = (VCard->configRead32(0xA4) >> 21) / 8;
            bcopy(&value, data, 2);
            
            return kIOReturnSuccess;
          }
          break;
        default:
          return kIOReturnBadArgument;
      }
      
      //bcopy(&value, data, 2);
      
      return kIOReturnSuccess;
    }
    
    //DebugLog("bad argument key name or data");
    
    return kIOReturnBadArgument;
  }
  
  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}
