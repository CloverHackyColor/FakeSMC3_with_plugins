/*
 *  IntelMCHMonitor.cpp
 *  HWSensors3
 *
 *  Copyright 2021 Slice. All rights reserved.
 *  First created at 16.02.2021
 */

#include "IntelMCHMonitor.h"
#include "FakeSMC.h"

#define INVID8(offset) (mmio_base[offset])
#define INVID16(offset) OSReadLittleInt16((mmio_base), offset)
#define INVID(offset) OSReadLittleInt32((mmio_base), offset)
#define OUTVID(offset,val) OSWriteLittleInt32((mmio_base), offset, val)


#define super IOService
OSDefineMetaClassAndStructors(IntelMCHMonitor, IOService)

bool IntelMCHMonitor::addSensor(const char* key,
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

IOService* IntelMCHMonitor::probe(IOService *provider, SInt32 *score) {
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
        
        if (((vendor_id==0x8086) && ((device_id & 0xFF00) == 0x3E00)) ||
            ((vendor_id==0x8086) && (class_id == 0x060000))) {
          InfoLog("found Intel MCH id=%x class_id=%x", (UInt16)device_id, class_id);
          VCard = device;
        }
      }
    }
  }
  return this;
}

bool IntelMCHMonitor::start(IOService * provider) {
  if (!provider || !super::start(provider)) { return false; }
	
	if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
		WarningLog("Can't locate fake SMC device, kext will not load");
		return false;
	}
  if (!VCard) {
    return false;
  }
  
  IOMemoryDescriptor *    theDescriptor;
  IOPhysicalAddress bar = (IOPhysicalAddress)((VCard->configRead32(kMCHBAR)) & ~0xf);
  DebugLog("register space=%08lx\n", (long unsigned int)bar);
  theDescriptor = IOMemoryDescriptor::withPhysicalAddress (bar, 0x1000, kIODirectionOutIn); // | kIOMapInhibitCache);
  
  if (theDescriptor != NULL) {
    mmio = theDescriptor->map();
    if (mmio != NULL) {
      mmio_base = (volatile UInt8 *)mmio->getVirtualAddress();
#if DEBUG
      DebugLog(" MCHBAR mapped\n");
      for (int i=0; i<0x2f; i +=16) {
        DebugLog("%04lx: ", (long unsigned int)i+0x1000);
        for (int j=0; j<16; j += 1) {
          DebugLog("%02lx ", (long unsigned int)INVID8(i+j+0x1000));
        }
        DebugLog("\n");
      }
#endif
    } else {
      InfoLog(" MCHBAR failed to map\n");
      return -1;
    }
  }

	
	char name[5];
	snprintf(name, 5, KEY_NORTHBRIDGE_TEMPERATURE); //TN0P
			
	UInt8 length = 0;
	void * data = 0;
  bool keyAdded = false;
			
	IOReturn result = fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                  true, (void *)name,
                                                  (void *)&length,
                                                  (void *)&data, 0);
			
	if (kIOReturnSuccess == result) {
		WarningLog("Key TN0P already exists, kext will not load");
		return false;    
	}

  if (!addSensor(name, TYPE_SP78, 2, 0)) {
    WarningLog("error adding Intel MCH temperature sensor");
  } else {
    keyAdded = true;
  }

  
  // DIMM
  for (int i = 0; i<4; i++) {
    snprintf(name, 5, KEY_DIMMn_TEMPERATURE, i); //Tm0P, Tm1P as in HWMonitor, Tm2P, Tm3P also possible
    if (!addSensor(name, TYPE_SP78, 2, i)) {
      WarningLog("error adding DIMM temperature sensor");
    } else {
      keyAdded = true;
    }
  }
	
	return keyAdded;
}


bool IntelMCHMonitor::init(OSDictionary *properties) {
  if (!super::init(properties)) {
    return false;
  }
	
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  
	return true;
}

void IntelMCHMonitor::stop (IOService* provider) {
	sensors->flushCollection();
	super::stop(provider);
}

void IntelMCHMonitor::free () {
	sensors->release();
	super::free();
}

IOReturn IntelMCHMonitor::callPlatformFunction(const OSSymbol *functionName,
                                             bool waitForFunction,
                                             void *param1,
                                             void *param2,
                                             void *param3,
                                             void *param4 ) {
  //UInt16 t;
  UInt32 value=0;
  
  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    const char* name = (const char*)param1;
    char* data = (char*)param2;
    
    if (name && data) {
      *data = 0;
      UInt32 index;
      switch (name[1]) {
        case 'N':
          if (strcasecmp(name, KEY_NORTHBRIDGE_TEMPERATURE) == 0) {
            value = INVID8(TPKG);
            bcopy(&value, data, 1);
            return kIOReturnSuccess;
          }
          break;
        case 'm':
          index = 0;
          if (OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(name))) {
            index = number->unsigned16BitValue();
          } else {
            return kIOReturnBadArgument;
          }
          if (index == 0 || index == 2) {
            value = INVID16(DDR1);
          } else {
            value = INVID16(DDR2);
          }
          if (index & 1) {
            value >>= 8;
          }
          value &= 0xFF;
          bcopy(&value, data, 1);
          break;
        default:
          return kIOReturnBadArgument;
      }
      return kIOReturnSuccess;
    }
    //DebugLog("bad argument key name or data");
    return kIOReturnBadArgument;
  }
  
  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}
