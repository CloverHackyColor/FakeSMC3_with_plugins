/*
 *  Radeon.cpp
 *  HWSensors
 *
 *  Created by Sergey on 20.12.10.
 *  Copyright 2010 Slice. All rights reserved.
 *
 */

#include "Radeon.h"


#define kGenericPCIDevice "IOPCIDevice"
#define kTimeoutMSecs 1000
#define fVendor "vendor-id"
#define fATYVendor "ATY,VendorID"
#define fDevice "device-id"
#define fClass	"class-code"
#define kIOPCIConfigBaseAddress0 0x10

#define INVID8(offset)		(mmio_base[offset])
#define INVID16(offset)		OSReadLittleInt16((mmio_base), offset)
#define INVID(offset)		OSReadLittleInt32((mmio_base), offset)
#define OUTVID(offset,val)	OSWriteLittleInt32((mmio_base), offset, val)

//TODO
/*
CARD32
_RHDReadPLL(int scrnIndex, CARD16 offset)
{
  _RHDRegWrite(scrnIndex, CLOCK_CNTL_INDEX, (offset & PLL_ADDR));
  return _RHDRegRead(scrnIndex, CLOCK_CNTL_DATA);
}
*/

#define super IOService
OSDefineMetaClassAndStructors(RadeonMonitor, IOService)

bool RadeonMonitor::addSensor(const char* key, const char* type, unsigned int size, int index) {
  void *tmp = (void *)(long long)size;
  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false,
                                                        (void *)key,
                                                        (void *)type,
                                                        tmp,
                                                        (void *)this)) {
    return sensors->setObject(key, OSNumber::withNumber(index, 32));
  }
	return false;
}

IOService* RadeonMonitor::probe(IOService *provider, SInt32 *score) {
  if (super::probe(provider, score) != this) { return 0; }
  bool ret = 0;
  if (OSDictionary * dictionary = serviceMatching(kGenericPCIDevice)) {
    if (OSIterator * iterator = getMatchingServices(dictionary)) {
      IOPCIDevice* device = 0;
      do {
        device = OSDynamicCast(IOPCIDevice, iterator->getNextObject());
        if (!device) {
          break;
        }
        vendor_id = 0;
        OSData *data = OSDynamicCast(OSData, device->getProperty(fVendor));
        if (data) {
          vendor_id = *(UInt32*)data->getBytesNoCopy();
        } else {
          data = OSDynamicCast(OSData, device->getProperty(fATYVendor));
          if (data) {
            vendor_id = *(UInt32*)data->getBytesNoCopy();
          }
        }

        device_id = 0;
        data = OSDynamicCast(OSData, device->getProperty(fDevice));
        if (data) {
          device_id = *(UInt32*)data->getBytesNoCopy();
        }
        
        class_id = 0;
        data = OSDynamicCast(OSData, device->getProperty(fClass));
        if (data) {
          class_id = *(UInt32*)data->getBytesNoCopy();
        }
        
        if ((vendor_id==0x1002) && (class_id == 0x030000)) {
          InfoLog("found Radeon chip id=%x ", (unsigned int)device_id);
          VCard = device;
          ret = 1; //TODO - count a number of cards
          break;
        }
        /*else {
         WarningLog("ATI Radeon not found!");
         }*/
      } while (device);
    }
  }
  
  if(ret) {
    return this;
  }
  
  return 0;
}

bool RadeonMonitor::start(IOService * provider) {
  if (!provider || !super::start(provider)) { return false; }
	
	if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
		WarningLog("Can't locate fake SMC device, kext will not load");
		return false;
	}
  
	Card = new ATICard();
	Card->VCard = VCard;
	Card->chipID = device_id;	
	if(Card->initialize()) {
		char name[5];
		//try to find empty key
		for (int i = 0; i < 0x10; i++) {
			snprintf(name, 5, KEY_FORMAT_GPU_DIODE_TEMPERATURE, i); 
			
			UInt8 length = 0;
			void * data = 0;
			
			IOReturn result = fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                      true,
                                                      (void *)name,
                                                      (void *)&length,
                                                      (void *)&data,
                                                      0);
			
			if (kIOReturnSuccess == result) {
				continue;
			}
      
			if (addSensor(name, TYPE_SP78, 2, i)) {
				numCard = i;
				break;
			}
		}
		
		if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                          false,
                                                          (void *)name,
                                                          (void *)TYPE_SP78,
                                                          (void *)2, this)) {
			WarningLog("Can't add key to fake SMC device, kext will not load");
			return false;
		}
		
		return true;	
	} else {
		return false;
	}
}


bool RadeonMonitor::init(OSDictionary *properties) {
  if (!super::init(properties)) { return false; }
  
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  
  return true;
}

void RadeonMonitor::stop (IOService* provider) {
  sensors->flushCollection();
  Card->release();  //?
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

void RadeonMonitor::free() {
  sensors->release();
  //Card->release();
  super::free();
}

IOReturn RadeonMonitor::callPlatformFunction(const OSSymbol *functionName,
                                             bool waitForFunction,
                                             void *param1,
                                             void *param2,
                                             void *param3,
                                             void *param4) {
	UInt16 t;
	
	if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
		const char* name = (const char*)param1;
		void* data = param2;
		
		if (name && data) {
			if (OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(name))) {				
				UInt32 index = number->unsigned16BitValue();
				if (index != numCard) {  //TODO - multiple card support
					return kIOReturnBadArgument;
				}
			}
			
			switch (Card->tempFamily) {
				case R6xx:
					Card->R6xxTemperatureSensor(&t);
					break;
				case R7xx:
					Card->R7xxTemperatureSensor(&t);
					break;
				case R8xx:
					Card->EverTemperatureSensor(&t);
					break;
				case R9xx:
					Card->TahitiTemperatureSensor(&t); 
					break;
				case RCIx:
					Card->HawaiiTemperatureSensor(&t);
					break;
        case RAIx:
          Card->ArcticTemperatureSensor(&t);
          break;
        case RVEx:
          Card->VegaTemperatureSensor(&t);
          break;
				default:
					break;
			}
			//t = Card->tempSensor->readTemp(index);
			bcopy(&t, data, 2);
			
			return kIOReturnSuccess;
		}
		
		//DebugLog("bad argument key name or data");
		
		return kIOReturnBadArgument;
	}
	
	return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}

//what about sleep/wake?
/*
#pragma mark -
#pragma mark ••• Power Management •••
#pragma mark -

//---------------------------------------------------------------------------

enum {
  kPowerStateOff = 0,
	kPowerStateDoze,
  kPowerStateOn,
  kPowerStateCount
};

IOReturn RadeonMonitor::registerWithPolicyMaker( IOService * policyMaker ) {
  static IOPMPowerState powerStateArray[ kPowerStateCount ] = {
    { 1,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,kIOPMDoze,kIOPMDoze,0,0,0,0,0,0,0,0 },
    { 1,kIOPMDeviceUsable,kIOPMPowerOn,kIOPMPowerOn,0,0,0,0,0,0,0,0 }
  };
	
  fCurrentPowerState = kPowerStateOn;
	
  return policyMaker->registerPowerDriver( this, powerStateArray,
                                          kPowerStateCount );
}

//---------------------------------------------------------------------------

IOReturn RadeonMonitor::setPowerState( unsigned long powerStateOrdinal,
                                IOService * policyMaker) {
	if (!pciNub || (powerStateOrdinal == fCurrentPowerState))
    return IOPMAckImplied;
	
  switch (powerStateOrdinal) {
    case kPowerStateOff:
      // Now that the driver knows if Magic Packet support was enabled,
      // tell PCI Family whether PME_EN should be set or not.
			
      //hwSetMagicPacketEnable( fMagicPacketEnabled );
			
      pciNub->hasPCIPowerManagement( fMagicPacketEnabled ?
                                    kPCIPMCPMESupportFromD3Cold : kPCIPMCD3Support );
      break;
			
    case kPowerStateDoze:
      break;
			
    case kPowerStateOn:
    if (fCurrentPowerState == kPowerStateOff) {
        initPCIConfigSpace(pciNub);
    }
      break;
  }
	
  fCurrentPowerState = powerStateOrdinal;
	
  return IOPMAckImplied;
}
*/
