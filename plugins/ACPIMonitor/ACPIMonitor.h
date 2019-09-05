/*
 *  ACPIMonitor.h
 *  HWSensors
 *
 *  Created by Slice.
 *
 */

#include <IOKit/IOService.h>
#include "IOKit/acpi/IOACPIPlatformDevice.h"
#include <IOKit/IOTimerEventSource.h>

class ACPIMonitor : public IOService
{
    OSDeclareDefaultStructors(ACPIMonitor)    
private:
	IOService*				    fakeSMC;
	IOACPIPlatformDevice*	acpiDevice;
	OSDictionary*			    sensors;
	
	bool				      addSensor(const char* method,
                              const char* key,
                              const char* type,
                              unsigned long long size);
	bool				      addTachometer(const char* method, const char* caption);
	
public:
	virtual IOService* probe(IOService *provider, SInt32 *score);
  virtual bool		  start(IOService *provider);
	virtual bool		  init(OSDictionary *properties=0);
	virtual void		  free(void);
	virtual void		  stop(IOService *provider);
	
	virtual IOReturn	callPlatformFunction(const OSSymbol *functionName,
                                         bool waitForFunction,
                                         void *param1,
                                         void *param2,
                                         void *param3,
                                         void *param4); 
};
