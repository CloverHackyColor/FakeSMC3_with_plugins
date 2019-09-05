/*
 *  SuperIOFamily.h
 *  HWSensors
 *
 *  Created by mozo on 08/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#ifndef _SUPERIOMONITOR_H
#define _SUPERIOMONITOR_H

#include <IOKit/IOLib.h>
#include <libkern/c++/OSArray.h>
#include <IOKit/IOService.h>

OSString * vendorID(OSString * smbios_manufacturer);

// Ports
const UInt8 SUPERIO_STANDART_PORT[]					    = { 0x2e, 0x4e };

// Registers
const UInt8 SUPERIO_CONFIGURATION_CONTROL_REGISTER	= 0x02;
const UInt8 SUPERIO_DEVICE_SELECT_REGISTER      = 0x07;
const UInt8 SUPERIO_CHIP_ID_REGISTER            = 0x20;
const UInt8 SUPERIO_CHIP_REVISION_REGISTER			= 0x21;
const UInt8 SUPERIO_BASE_ADDRESS_REGISTER       = 0x60;
const UInt8 WINBOND_CHIP_IPD_REGISTER           = 0x23; //Immediate Power Down
const UInt8 ITE_CHIP_SUSPEND_REGISTER           = 0x24;
const UInt8 FINTEK_CHIP_IPD_REGISTER            = 0x25; //Software Power Down


enum SuperIOSensorGroup {
	kSuperIOTemperatureSensor,
	kSuperIOTachometerSensor,
	kSuperIOVoltageSensor,
  kSuperIOFrequency
};

class SuperIOMonitor;

class SuperIOSensor : public OSObject {
	 OSDeclareDefaultStructors(SuperIOSensor)
	
protected:
	SuperIOMonitor *    owner;
	char *              name;
	char *              type;
	unsigned char       size;
	SuperIOSensorGroup	group;
	unsigned long       index;
  int                 scale;
  long                Ri;
  long                Rf;
  long                Vf;
  
	
public:
	static SuperIOSensor *withOwner(SuperIOMonitor *aOwner,
                                  const char* aKey,
                                  const char* aType,
                                  unsigned char aSize,
                                  SuperIOSensorGroup aGroup,
                                  unsigned long aIndex,
                                  long aRi=0,
                                  long aRf=1,
                                  long aVf=0);
	
	const char *        getName();
	const char *        getType();
	unsigned char       getSize();
	SuperIOSensorGroup	getGroup();
	unsigned long       getIndex();
  long                encodeValue(UInt32 value, int scale);
	
	virtual bool		initWithOwner(SuperIOMonitor *aOwner,
                                const char* aKey,
                                const char* aType,
                                unsigned char aSize,
                                SuperIOSensorGroup aGroup,
                                unsigned long aIndex,
                                long aRi,
                                long aRf,
                                long aVf);
  
	virtual long        getValue();
	virtual void        free();
};

class SuperIOMonitor : public IOService {
	OSDeclareAbstractStructors(SuperIOMonitor)
	
protected:
	IOService *				fakeSMC;
		
	UInt16            address;
	UInt8             registerPort;
	UInt8             valuePort;
	
	UInt32            model;
	
	OSArray *         sensors;
	
	UInt8             listenPortByte(UInt8 reg);
	UInt16            listenPortWord(UInt8 reg);
	void              selectLogicalDevice(UInt8 num);
	bool              getLogicalDeviceAddress(UInt8 reg = SUPERIO_BASE_ADDRESS_REGISTER);
	
	virtual int				getPortsCount();
	virtual void			selectPort(unsigned char index);
	virtual bool			probePort();
	virtual void			enter();
	virtual void			exit();
	
	virtual const char *getModelName();
	
	SuperIOSensor *addSensor(const char* key,
                           const char* type,
                           unsigned int size,
                           SuperIOSensorGroup group,
                           unsigned long index,
                           long aRi=0,
                           long aRf=1,
                           long aVf=0);
  
	SuperIOSensor *addTachometer(unsigned long index, const char* id = 0);
  
	SuperIOSensor *getSensor(const char* key);
  
	virtual bool  updateSensor(const char *key,
                             const char *type,
                             unsigned int size,
                             SuperIOSensorGroup group,
                             unsigned long index);
		
public:
	virtual long    readTemperature(unsigned long index);
	virtual long    readVoltage(unsigned long index);
	virtual long    readTachometer(unsigned long index);
	
	virtual bool    init(OSDictionary *properties=0);
	virtual IOService*  probe(IOService *provider, SInt32 *score);
  virtual bool    start(IOService *provider);
	virtual void  stop(IOService *provider);
	virtual void  free(void);
	
	virtual IOReturn  callPlatformFunction(const OSSymbol *functionName,
                                         bool waitForFunction,
                                         void *param1,
                                         void *param2,
                                         void *param3,
                                         void *param4); 
};

#endif
