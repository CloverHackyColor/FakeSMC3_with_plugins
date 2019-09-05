/*
 *  IntelCPUMonitor.h
 *  HWSensors
 *
 *  Created by Slice on 20.12.10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOKitKeys.h>
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>
#include <i386/proc_reg.h>
#include <string.h>

#include "../../../utils/cpuid.h"

#define MSR_IA32_THERM_STATUS		    0x019C
#define MSR_IA32_PERF_STATUS		    0x0198;
#define MSR_IA32_TEMPERATURE_TARGET	0x01A2
#define MSR_PERF_FIXED_CTR_CTRL     (0x38d)
#define MSR_PERF_GLOBAL_CTRL        (0x38f)
//#define MSR_PLATFORM_INFO			      0xCE;

#define MaxCpuCount 128
#define MaxPStateCount	32

extern "C" {
  void mp_rendezvous_no_intrs(void (*action_func)(void *), void * arg);
  int cpu_number(void);
};

struct PState  {
	union {
		UInt16 Control;
		struct  {
			UInt8 VID;	// Voltage ID
			UInt8 FID;	// Frequency ID
		};
	};
	
	UInt8	DID;		// DID
	UInt8	CID;		// Compare ID
//  UInt32 Max;
};

class IntelCPUMonitor : public IOService {
    OSDeclareDefaultStructors(IntelCPUMonitor)   
public:
	UInt32					Frequency[MaxCpuCount];
	UInt32					Voltage;    //in millivolts
  UInt32          BaseFreqRatio;
  
private:
	bool				  	Active;	
	bool					  LoopLock;
	UInt64					BusClock;
	UInt64					FSBClock;
	UInt32					CpuFamily;
	UInt32					CpuModel; 
	UInt32					CpuStepping;
	bool					  CpuMobile;
	UInt8					  count;
	UInt8					  threads;
	UInt8					  tjmax[MaxCpuCount];
	UInt32					userTjmax;
	char*					  key[MaxCpuCount];
	char					  Platform[4];
	bool					  nehalemArch;
  bool            SandyArch;
	IOService*			fakeSMC;
	void					  Activate(void);
	void					  Deactivate(void);
	UInt32					IntelGetFrequency(UInt8 fid);
	UInt32					IntelGetVoltage(UInt16 vid);
  
  IOWorkLoop *		WorkLoop;
	IOTimerEventSource *	TimerEventSource;

	
public:
	virtual bool		    init(OSDictionary *properties=0);
	virtual IOService*	probe(IOService *provider, SInt32 *score);
  virtual bool        start(IOService *provider);
	virtual void		    stop(IOService *provider);
	virtual void		    free(void);
	virtual IOReturn setPowerState(unsigned long which, IOService *whom);
	virtual IOReturn	callPlatformFunction(const OSSymbol *functionName,
                                         bool waitForFunction,
                                         void *param1,
                                         void *param2,
                                         void *param3,
                                         void *param4); 
	virtual IOReturn	loopTimerEvent(void);
};
