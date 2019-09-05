/*
 *  Radeon.h
 *  HWSensors
 *
 *  Created by Sergey on 20.12.10.
 *  Copyright 2010 Slice. All rights reserved.
 *
 */

#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>
#include "ATICard.h"

class RadeonMonitor : public IOService
{
    OSDeclareDefaultStructors(RadeonMonitor)    
	
private:
	IOService*    fakeSMC;
	OSDictionary* sensors;
	volatile      UInt8* mmio_base;
	int					    numCard;  //numCard=0 if only one Video, but may be any other value
	IOPCIDevice * VCard;
	IOMemoryMap * mmio;
	UInt32				vendor_id;
	UInt32				device_id;
	UInt32				class_id;
	
	bool          addSensor(const char* key, const char* type, unsigned int size, int index);
protected:	
	ATICard*			Card; 
	
public:
	virtual IOService*	probe(IOService *provider, SInt32 *score);
  virtual bool  start(IOService *provider);
	virtual bool  init(OSDictionary *properties=0);
	virtual void  free(void);
	virtual void  stop(IOService *provider);
	
  virtual IOReturn callPlatformFunction(const OSSymbol *functionName,
                                        bool waitForFunction,
                                        void *param1,
                                        void *param2,
                                        void *param3,
                                        void *param4);
};
