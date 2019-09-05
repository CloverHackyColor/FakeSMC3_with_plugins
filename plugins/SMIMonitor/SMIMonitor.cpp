/*
 *  SMIMonitor.cpp
 *  HWSensors
 *
 *  Copyright 2014 Slice. All rights reserved.
 *
 */

#include "SMIMonitor.h"
#include "FakeSMC.h"
#include "utils.h"

#ifndef Debug
#define Debug FALSE
#endif

#define LogPrefix "SMIMonitor: "
#define DebugLog(string, args...)  do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)  do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super IOService
OSDefineMetaClassAndStructors(SMIMonitor, IOService)

bool SMIMonitor::addSensor(const char* key, const char* type, unsigned int size) {
  if (!key || !type) {
    return false;
  }
  if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false, (void *)key,
                                                        (void *)type,
                                                        (void *)(size_t)size,
                                                        (void *)this)) {
    WarningLog("Can't add key %s to fake SMC device, kext will not load", key);
    return false;
  }
  return true;
}

// for example addTachometer(0, "System Fan");
bool SMIMonitor::addTachometer(int index, const char* id) {
  UInt8 length = 0;
  void * data = 0;
  if (!id) {
    return false;
  }

  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                        true,
                                                        (void *)KEY_FAN_NUMBER,
                                                        (void *)&length,
                                                        (void *)&data,
                                                        0)) {
    char key[5];
    length = 0;

    bcopy(data, &length, 1);

    snprintf(key, 5, KEY_FORMAT_FAN_SPEED, length);

    if (addSensor(key, TYPE_FPE2, 2)) {
      if (id) {
        FanTypeDescStruct fds;
        snprintf(key, 5, KEY_FORMAT_FAN_ID, length);
        fds.type = FAN_PWM_TACH;
        fds.ui8Zone = 1;
        fds.location = LEFT_LOWER_FRONT;
        strncpy(fds.strFunction, id, DIAG_FUNCTION_STR_LEN);

        if (kIOReturnSuccess != fakeSMC->callPlatformFunction(kFakeSMCAddKeyValue,
                                                              false,
                                                              (void *)key,
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
                                                            (void *)&length,
                                                            0)) {
        WarningLog("error updating FNum value");
      }
      return true;
    }
  } else {
    WarningLog("error reading FNum value");
  }

  return false;
}


SMMRegisters *globalRegs;
static int gRc;
//static int fanMult;
//inline
static
int smm(SMMRegisters *regs)
{

  int rc;
  int eax = regs->eax;  //input value

#if __LP64__
  asm volatile("pushq %%rax\n\t"
               "movl 0(%%rax),%%edx\n\t"
               "pushq %%rdx\n\t"
               "movl 4(%%rax),%%ebx\n\t"
               "movl 8(%%rax),%%ecx\n\t"
               "movl 12(%%rax),%%edx\n\t"
               "movl 16(%%rax),%%esi\n\t"
               "movl 20(%%rax),%%edi\n\t"
               "popq %%rax\n\t"
               "out %%al,$0xb2\n\t"
               "out %%al,$0x84\n\t"
               "xchgq %%rax,(%%rsp)\n\t"
               "movl %%ebx,4(%%rax)\n\t"
               "movl %%ecx,8(%%rax)\n\t"
               "movl %%edx,12(%%rax)\n\t"
               "movl %%esi,16(%%rax)\n\t"
               "movl %%edi,20(%%rax)\n\t"
               "popq %%rdx\n\t"
               "movl %%edx,0(%%rax)\n\t"
               "pushfq\n\t"
               "popq %%rax\n\t"
               "andl $1,%%eax\n"
               : "=a"(rc)
               :    "a"(regs)
               :    "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");
#else
  asm volatile("pushl %%eax\n\t"
               "movl 0(%%eax),%%edx\n\t"
               "push %%edx\n\t"
               "movl 4(%%eax),%%ebx\n\t"
               "movl 8(%%eax),%%ecx\n\t"
               "movl 12(%%eax),%%edx\n\t"
               "movl 16(%%eax),%%esi\n\t"
               "movl 20(%%eax),%%edi\n\t"
               "popl %%eax\n\t"
               "out %%al,$0xb2\n\t"
               "out %%al,$0x84\n\t"
               "xchgl %%eax,(%%esp)\n\t"
               "movl %%ebx,4(%%eax)\n\t"
               "movl %%ecx,8(%%eax)\n\t"
               "movl %%edx,12(%%eax)\n\t"
               "movl %%esi,16(%%eax)\n\t"
               "movl %%edi,20(%%eax)\n\t"
               "popl %%edx\n\t"
               "movl %%edx,0(%%eax)\n\t"
               "lahf\n\t"
               "shrl $8,%%eax\n\t"
               "andl $1,%%eax\n"
               : "=a"(rc)
               :    "a"(regs)
               :    "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");
#endif

  if ((rc != 0) || ((regs->eax & 0xffff) == 0xffff) || (regs->eax == eax)) {
    return -1;
  }

  return 0;
}

void read_smi(void * magic) {
  SMMRegisters *regs = (SMMRegisters *)magic;
  volatile UInt32 i = cpu_number();
  if (i == 0) { /* SMM requires CPU 0 */
    gRc = smm(regs);
  } else gRc = -1;
}

int SMIMonitor::i8k_smm(SMMRegisters *regs)
{
  mp_rendezvous_no_intrs(read_smi, (void *)regs);
  return gRc;
}

//not works
/*
int SMIMonitor::i8k_get_bios_version(void) {
  INIT_REGS;
  int rc;

  regs.eax = I8K_SMM_BIOS_VERSION;
  if ((rc = i8k_smm(&regs)) < 0) {
    return rc;
  }
  return regs.eax;
} */

/*
 * Read the CPU temperature in Celcius.
 */
int SMIMonitor::i8k_get_temp(int sensor) {
  INIT_REGS;
  int rc;
  int temp;

  regs.eax = I8K_SMM_GET_TEMP;
  regs.ebx = sensor & 0xFF;
  if ((rc=i8k_smm(&regs)) < 0) {
    return rc;
  }

  temp = regs.eax & 0xff;
  if (temp == 0x99) {
    IOSleep(100);
    regs.eax = I8K_SMM_GET_TEMP;
    regs.ebx = sensor & 0xFF;
    if ((rc=i8k_smm(&regs)) < 0) {
      return rc;
    }
    temp = regs.eax & 0xff;
  }
  return temp;
}

int SMIMonitor::i8k_get_temp_type(int sensor) {
  INIT_REGS;
  int rc;
  int type;

  regs.eax = I8K_SMM_GET_TEMP_TYPE;
  regs.ebx = sensor & 0xFF;
  if ((rc=i8k_smm(&regs)) < 0) {
    return rc;
  }

  type = regs.eax & 0xff;
  return type;
}

const char * SMIMonitor::getKeyForTemp(int type) {
//  int type = i8k_get_temp_type(sensor);
  switch (type) {
    case I8K_SMM_TEMP_CPU:
      return KEY_CPU_PROXIMITY_TEMPERATURE;
    case I8K_SMM_TEMP_GPU:
      return KEY_GPU_PROXIMITY_TEMPERATURE;
    case I8K_SMM_TEMP_MEMORY:
      return KEY_DIMM_TEMPERATURE;
    case I8K_SMM_TEMP_MISC:
      return KEY_NORTHBRIDGE_TEMPERATURE;
    case I8K_SMM_TEMP_AMBIENT:
      return KEY_AMBIENT_TEMPERATURE; //
    case I8K_SMM_TEMP_OTHER:
      return KEY_AIRPORT_TEMPERATURE;
    default:
      break;
  }
  return NULL;
}

bool SMIMonitor::i8k_get_dell_sig_aux(int fn) {
  INIT_REGS;
  int rc;

  regs.eax = fn;
  if ((rc=i8k_smm(&regs)) < 0) {
    WarningLog("No function 0x%x", fn);
    return false;
  }
  InfoLog("Got sigs %x and %x", regs.eax, regs.edx);
  return ((regs.eax == 0x44494147 /*DIAG*/) &&
          (regs.edx == 0x44454C4C /*DELL*/));
}

bool SMIMonitor::i8k_get_dell_signature(void) {

  return (i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG1) ||
          i8k_get_dell_sig_aux(I8K_SMM_GET_DELL_SIG2));
}


/*
 * Read the power status.
 */
int SMIMonitor::i8k_get_power_status(void) {
  INIT_REGS;
  int rc;
  /*
   regs.eax = I8K_SMM_GET_POWER_TYPE;
   i8k_smm(&regs);
   InfoLog("Got power type=%d", (regs.eax & 0xff));
   */
  regs.eax = I8K_SMM_GET_POWER_TYPE; //I8K_SMM_GET_POWER_STATUS;
  if ((rc=i8k_smm(&regs)) < 0) {
    WarningLog("No power status");
    return rc;
  }
  InfoLog("Got power status=%d", (regs.eax & 0xff));
  return regs.eax & 0xff; // 0 = No Batt, 3 = No AC, 1 = Charging, 5 = Full.
}

/*
 * Read the fan speed in RPM.
 */
int SMIMonitor::i8k_get_fan_speed(int fan) {
  INIT_REGS;
  int rc;
  int speed = 0;

  regs.eax = I8K_SMM_GET_SPEED;
  regs.ebx = fan & 0xff;
  if ((rc=i8k_smm(&regs)) < 0) {
    return rc;
  }
  speed = (regs.eax & 0xffff) * fanMult;
  return speed;
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_status(int fan) {
  INIT_REGS;
  int rc;

  regs.eax = I8K_SMM_GET_FAN;
  regs.ebx = fan & 0xff;
  if ((rc=i8k_smm(&regs)) < 0) {
    return rc;
  }

  return (regs.eax & 0xff);
}

/*
 * Read the fan status.
 */
int SMIMonitor::i8k_get_fan_type(int fan) {
  INIT_REGS;
  int rc;

  regs.eax = I8K_SMM_GET_FAN_TYPE;
  regs.ebx = fan & 0xff;
  if ((rc=i8k_smm(&regs)) < 0) {
    return rc;
  }

  return (regs.eax & 0xff);
}


/*
 * Read the fan nominal rpm for specific fan speed (0,1,2) or zero
 */
int SMIMonitor::i8k_get_fan_nominal_speed(int fan, int speed)
{
  INIT_REGS;
  regs.eax = I8K_SMM_GET_NOM_SPEED;
  regs.ebx = (fan & 0xff) | (speed << 8);
  return i8k_smm(&regs) ? 0 : (regs.eax & 0xffff) * fanMult;
}

/*
 * Set the fan speed (off, low, high). Returns the new fan status.
 */
int SMIMonitor::i8k_set_fan(int fan, int speed)
{
  INIT_REGS;
  regs.eax = I8K_SMM_SET_FAN;

  speed = (speed < 0) ? 0 : ((speed > I8K_FAN_MAX) ? I8K_FAN_MAX : speed);
  regs.ebx = (fan & 0xff) | (speed << 8);

  return i8k_smm(&regs) ? -1 : i8k_get_fan_status(fan);
}

int  SMIMonitor::i8k_set_fan_control_manual(int fan)
{
  INIT_REGS;
  regs.eax = I8K_SMM_IO_DISABLE_FAN_CTL1;
  regs.ebx = (fan & 0xff);
  return i8k_smm(&regs);
}

int  SMIMonitor::i8k_set_fan_control_auto(int fan)
{
  INIT_REGS;
  regs.eax = I8K_SMM_IO_ENABLE_FAN_CTL1;
  regs.ebx = (fan & 0xff);
  return i8k_smm(&regs);
}


IOService* SMIMonitor::probe(IOService *provider, SInt32 *score) {
  if (super::probe(provider, score) != this) { return 0; }

  if (!i8k_get_dell_signature()) {
    WarningLog("Unable to get Dell SMM signature!");
    return NULL;
  }

  InfoLog("Based on I8kfan project adopted to HWSensors by Slice 2014");
#if TEST
   InfoLog("Dell BIOS version=%x", i8k_get_bios_version());
   InfoLog("Dump SMI ------------");
   INIT_REGS;
   int rc;

   regs.eax = I8K_SMM_POWER_STATUS;
   rc=i8k_smm(&regs);
   InfoLog("POWER_STATUS: rc=0x%x eax=0x%x", rc, regs.eax);

   for (int i=0; i<6; i++) {
     memset(&regs, 0, sizeof(regs));
     regs.eax = I8K_SMM_GET_FAN;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     InfoLog("GET_FAN %d: rc=0x%x eax=0x%x", i, rc, regs.eax);
     memset(&regs, 0, sizeof(regs));
     if (rc < 0) continue;
     regs.eax = I8K_SMM_FN_STATUS;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     InfoLog("FN_STATUS %d: rc=0x%x eax=0x%x", i, rc, regs.eax);
     if (rc < 0) continue;
     memset(&regs, 0, sizeof(regs));
     regs.eax = I8K_SMM_GET_FAN_TYPE;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     InfoLog("GET_FAN_TYPE: rc=0x%x eax=0x%x", rc, regs.eax);
     memset(&regs, 0, sizeof(regs));
     regs.eax = I8K_SMM_GET_SPEED;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     InfoLog("GET_SPEED: rc=0x%x eax=0x%x", rc, regs.eax);
     memset(&regs, 0, sizeof(regs));
     regs.eax = I8K_SMM_GET_NOM_SPEED;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     InfoLog("GET_NOM_SPEED: rc=0x%x eax=0x%x", rc, regs.eax);
   }

   for (int i=0; i<6; i++) {
     memset(&regs, 0, sizeof(regs));
     regs.eax = I8K_SMM_GET_TEMP;
     regs.ebx = i;
     rc=i8k_smm(&regs);
     if (rc < 0) continue;
     InfoLog("GET_TEMP %d: rc=0x%x eax=0x%x", i, rc, regs.eax);
   }
#endif

  i8k_get_power_status();

  return this;
}

bool SMIMonitor::start(IOService * provider)
{
  INIT_REGS;
  int rc = 0;
  if (!provider || !super::start(provider)) { return false; }

  if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
    WarningLog("Can't locate fake SMC device, kext will not load");
    return false;
  }

  char key[5];

  //Here are Fans in info.plist
  OSArray* fanNames = OSDynamicCast(OSArray, getProperty("FanNames"));
  fanNum = 0;
  fansStatus = 0;
  for (int fan = 0; fan < 6; fan++) {

    memset(&regs, 0, sizeof(regs));
    regs.eax = I8K_SMM_GET_FAN;
    regs.ebx = fan;
    rc = i8k_smm(&regs);
    if (rc < 0) continue; //laptop has one fan, but desktop 0 and 2; continue search
    i8k_set_fan_control_auto(fan); //force automatic control

    fanNum++;
    snprintf(key, 5, "FAN%X", fan);
    int fType = i8k_get_fan_type(fan);
    InfoLog("found fan %d type %d", fan, fType);
    if (fType < 0) {
      fType = fan;
    }
    InfoLog("   min speed %d", i8k_get_fan_nominal_speed(fan, 1));
    InfoLog("   max speed %d", i8k_get_fan_nominal_speed(fan, 2));

    OSString* name = NULL;
    if (fanNames) {
      name = OSDynamicCast(OSString, fanNames->getObject(fType));
    }

    if (!addTachometer(fan, name ? name->getCStringNoCopy() : 0)) {
      WarningLog("Can't add tachometer sensor, key %s", key);
    }
	  snprintf(key, 5, KEY_FORMAT_FAN_MIN_SPEED, fan);
	  addSensor(key, TYPE_FPE2, 2);  //F0Mn
	  snprintf(key, 5, KEY_FORMAT_FAN_MAX_SPEED, fan);
	  addSensor(key, TYPE_FPE2, 2);  //F0Mm

    //add special key for fan status control
    snprintf(key, 5, "F%XAs", fan);
    addSensor(key, TYPE_UI8, 1);  //F0As
    // individual fan control
    snprintf(key, 5, KEY_FORMAT_FAN_MANUAL_DRIVE, fan);
    addSensor(key, TYPE_UI8, 1);  //F0As
  }
  snprintf(key, 5, KEY_FAN_FORCE);
  addSensor(key, TYPE_UI16, 2);  //FS!

  for (int i=0; i<6; i++) {
    rc = i8k_get_temp(i);
    if (rc >= 0) {
      int type = i8k_get_temp_type(i);
      if ((type >= I8K_SMM_TEMP_CPU) && (type <= I8K_SMM_TEMP_OTHER)) {
        TempSensors[type] = i;
        InfoLog("sensor %d type %d", i, type);
        addSensor(getKeyForTemp(type), TYPE_SP78, 2);
      }
    }
  }

  registerService(0);
  return true;
}

bool SMIMonitor::init(OSDictionary *properties) {
  if (!super::init(properties)) {
    return false;
  }
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  fanMult = 1; //linux proposed to get nominal speed and if it high then change multiplier
  OSNumber * Multiplier = OSDynamicCast(OSNumber, properties->getObject("FanMultiplier"));
  if (Multiplier)
    fanMult = Multiplier->unsigned32BitValue();
  return true;
}

void SMIMonitor::stop(IOService* provider) {
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

void SMIMonitor::free() {
  sensors->release();
  super::free();
}

#define MEGA10 10000000ull
IOReturn SMIMonitor::callPlatformFunction(const OSSymbol *functionName,
                                          bool waitForFunction,
                                          void *param1,
                                          void *param2,
                                          void *param3,
                                          void *param4) {
  const char* name = (const char*)param1;
  UInt8 * data = (UInt8*)param2;
  //  UInt64 size = (UInt64)param3;

  size_t value;
  UInt16 val;

  if (functionName->isEqualTo(kFakeSMCSetValueCallback)) {
    if (name && data) {
      InfoLog("Writing key=%s value=%x", name, *(UInt8*)data);
      //OSObject * params[1];
      if ((name[0] == 'F') && (name[2] == 'A') && (name[3] == 's')) {  //set fan status {off, low, high}
        val = *(UInt8*)data & 3; //restrict possible values to 0,1,2,3
        int fan = (int)(name[1] - '0');
        int ret = i8k_set_fan(fan, val); //return new status, should we check it?
        if (ret == val) {
          return kIOReturnSuccess;
        }
        else {
          return kIOReturnError;
        }
      }
      else if ((name[0] == 'F') && (name[1] == 'S') && (name[2] == '!')) {
        val = (data[0] << 8) + data[1]; //big endian data
        int rc = 0;
        for (int i = 0; i < fanNum; i++) {
          if ((val & (1 << i)) != (fansStatus & (1 << i))) {
            rc |= (val & (1 << i)) ? i8k_set_fan_control_manual(i) : i8k_set_fan_control_auto(i);
          }
        }
        if (!rc) {
          fansStatus = val;
          return kIOReturnSuccess;
        }
        else {
          return kIOReturnError;
        }
      }
      else if ((name[0] == 'F') && (name[2] == 'M') && (name[3] == 'd')) {
        val = data[0];
        int fan = (int)(name[1] - '0') & 0x7; //restrict to 7 fans
        int rc = 0;
        if (val != (fansStatus & (1 << fan))>>fan) {
          rc |= val ? i8k_set_fan_control_manual(fan) : i8k_set_fan_control_auto(fan);
        }
        if (!rc) {
          fansStatus = val ? (fansStatus | (1 << fan)): (fansStatus & ~(1 << fan));
          return kIOReturnSuccess;
        }
        else {
          return kIOReturnError;
        }
      }    }
    return kIOReturnBadArgument;
  }

  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    if (name && data) {
      val = 0;
      if (name[0] == 'F') {
        if ((name[2] == 'A') && (name[3] == 'c')) {
          int fan = (int)(name[1] - '0');
          value = i8k_get_fan_speed(fan);
          val = encode_fpe2(value);
          bcopy(&val, data, 2);
          return kIOReturnSuccess;
        }
        else if ((name[2] == 'A') && (name[3] == 's')) {
          int fan = (int)(name[1] - '0');
          val = i8k_get_fan_status(fan);
          bcopy(&val, data, 1);
          return kIOReturnSuccess;
        }
        else if ((name[1] == 'S') && (name[2] == '!')) {
          val = fansStatus;
          data[0] = val >> 8;
          data[1] = val & 0xFF;
          return kIOReturnSuccess;
        }
        else if ((name[2] == 'M') && (name[3] == 'd')) {
          int fan = (int)(name[1] - '0') & 0x7;
          val = (fansStatus & (1 << fan)) >> fan;
          bcopy(&val, data, 1);
          return kIOReturnSuccess;
        }
		else if ((name[2] == 'M') && (name[3] == 'n')) {
			int fan = (int)(name[1] - '0') & 0x7;
			value = i8k_get_fan_nominal_speed(fan, 1);
			val = encode_fpe2(value);
			bcopy(&val, data, 2);
			return kIOReturnSuccess;
		}
		else if ((name[2] == 'M') && (name[3] == 'm')) {
			int fan = (int)(name[1] - '0') & 0x7;
			value = i8k_get_fan_nominal_speed(fan, 2);
			val = encode_fpe2(value);
			bcopy(&val, data, 2);
			return kIOReturnSuccess;
		}
	  } else if ((name[0] == 'T') && (name[2] == '0') && (name[3] == 'P')) {
        if (name[1] == 'C') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_CPU]);
        } else if (name[1]  == 'G') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_GPU]);
        } else if (name[1]  == 'm') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_MEMORY]);
        } else if (name[1]  == 'N') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_MISC]);
        } else if (name[1]  == 'A') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_AMBIENT]);
        } else if (name[1]  == 'W') {
          val = i8k_get_temp(TempSensors[I8K_SMM_TEMP_OTHER]);
        }
        bcopy(&val, data, 1);
        return kIOReturnSuccess;
      }
      return kIOReturnBadArgument; //no key or no pointer to data
    }
    //DebugLog("bad argument key name or data");
    return kIOReturnBadArgument;
  }

  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}

