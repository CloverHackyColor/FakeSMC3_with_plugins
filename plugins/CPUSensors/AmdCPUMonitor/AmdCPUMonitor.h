/*
 *  AmdCPUMonitor.h
 *  HWSensors
 *
 *  Created by Sergey on 19.12.10 with templates of Mozodojo.
 *  Copyright 2010 Slice.
 *
 */

#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>

#define kGenericPCIDevice "IOPCIDevice"
#define kTimeoutMSecs 1000
#define fVendor "vendor-id"
#define fDevice "device-id"
#define fClass	"class-code"
#define kIOPCIConfigBaseAddress0 0x10

//Intel Ibex Peak PCH and Intel Cougar Point
#define TBAR  0x10  /* OS address */
#define TBARB 0x40  /* BIOS address */
//on the BAR
#define TSIU  0x00  /* wait for 0, read T, then write 1 to release */
#define TSE   0x01  /* enable thermometer -> 0xB8 */
#define TSTR  0x03  /* Thermal Sensor Thermometer Read 1byte 00-7F*/
#define TRC   0x1A  /* enable mask for sensors 0x91FF */
#define PTV   0x60  /* Processor Max Temperature Value 2bytes*/
#define DTV   0xB0  /* DIMM Temperature Value 4bytes 4values for DIMM0,1,2,3*/
#define ITV   0xD8  /* Internal Temperature Value 4bytes byte0=PCH byte1=GPU*/
//
//Intel Ibex Peak PCH only
#define CTV1  0x30  /* Core Temperature Value 1 2bytes >>6 for Celsius*/
#define CTV2  0x32  /* Core Temperature Value 2 2bytes*/
#define CEV1  0x34  /* Core Energy Value 1 4bytes CEV1/65535*1000 = mW */
#define MGTV  0x58  /* Graphics Temperature Value 8bytes ???*/
/*
 #define kMCHBAR  0x40
 #define TSC1	    0x1001
 #define TSS1	    0x1004
 #define TR1		  0x1006
 #define RTR1 	  0x1008
 #define TIC1	    0x100B
 #define TSC2	    0x1041
 #define TSS2	    0x1044
 #define TR2		  0x1046
 #define RTR2	    0x1048
 #define TIC2	    0x104B
 */


#define INVID8(offset) (mmio_base[offset])
#define INVID16(offset) OSReadLittleInt16((mmio_base), offset)
#define INVID(offset) OSReadLittleInt32((mmio_base), offset)
#define OUTVID(offset,val) OSWriteLittleInt32((mmio_base), offset, val)

#define Debug FALSE

#define LogPrefix "AmdCPUMonitor: "
#define DebugLog(string, args...)	do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)	do { IOLog (LogPrefix string "\n", ## args); } while(0)

class AmdCPUMonitor : public IOService {
    OSDeclareDefaultStructors(AmdCPUMonitor)    
private:
	IOService*			fakeSMC;
	OSDictionary*		sensors;
	volatile        UInt8* mmio_base;
	int             numCard;  //numCard=0 if only one Video, but may be any other value
	IOPCIDevice *		VCard;
	IOMemoryMap *		mmio;
	
	bool            addSensor(const char* key, const char* type, unsigned int size, int index);
	
public:
	virtual IOService*  probe(IOService *provider, SInt32 *score);
  virtual bool		start(IOService *provider);
	virtual bool		init(OSDictionary *properties=0);
	virtual void		free(void);
	virtual void		stop(IOService *provider);
	
	virtual IOReturn callPlatformFunction(const OSSymbol *functionName,
                                        bool waitForFunction,
                                        void *param1,
                                        void *param2,
                                        void *param3,
                                        void *param4); 
};
