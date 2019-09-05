/*
 *  NVClockX.h
 *  HWSensors
 *
 *  Created by mozo on 15/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#include <IOKit/IOService.h>
#include "NVClock/nvclock.h"

#define kGenericPCIDevice "IOPCIDevice"
#define kNVGraphicsDevice "IONDRVDevice"

//NVClock nvclock;
//NVCard* nv_card;

class NVClockX : public IOService {
    OSDeclareDefaultStructors(NVClockX)    
	
private:
	IOService *     fakeSMC;
	OSDictionary *  sensors;
	
	IOMemoryMap *	  nvio;
	
	int				      probeDevices();
	bool			      addSensor(const char* key, const char* type, unsigned int size, int index);
	int				      addTachometer(int index);
	
public:
	virtual bool        init(OSDictionary *properties=0);
	virtual IOService*	probe(IOService *provider, SInt32 *score);
  virtual bool        start(IOService *provider);
	virtual void        stop(IOService *provider);
	virtual void        free(void);
	
	virtual IOReturn    callPlatformFunction(const OSSymbol *functionName,
                                           bool waitForFunction,
                                           void *param1,
                                           void *param2,
                                           void *param3,
                                           void *param4); 
	
};
