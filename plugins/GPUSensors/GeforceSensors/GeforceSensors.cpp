/*
 *  GeforceSensors.cpp
 *  HWSensors
 *
 *  Created by kozlek on 19/04/12.
 *  Copyright 2010 Natan Zalkin <natan.zalkin@me.com>. All rights reserved.
 *
 */

/*
 * Copyright 2007-2008 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "GeforceSensors.h"

#include "../../../fakesmc/FakeSMC.h"
#include "../../../utils/definitions.h"
#include "../../../utils/utils.h"

#include "nouveau.h"
//#include "nvclock_i2c.h"
bool is_digit(char c);

#define Debug FALSE

#define LogPrefix "GeForceSensors: "
#define DebugLog(string, args...)  do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)  do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define kNouveauPWMSensor               1000
#define kNouveauCoreTemperatureSensor   1001
#define kNouveauBoardTemperatureSensor  1002

#define kGenericPCIDevice "IOPCIDevice"
#define kTimeoutMSecs     1000
#define fVendor           "vendor-id"
#define fDevice           "device-id"
#define fClass            "class-code"
#define kIOPCIConfigBaseAddress0 0x10


#define super IOService
OSDefineMetaClassAndStructors(GeforceSensors, IOService)


bool is_digit(char c) {
  if (((c>='0')&&(c<='9'))||((c>='a')&&(c<='f'))||((c>='A')&&(c<='F'))) {
    return true;
  }

  return false;
}

bool GeforceSensors::addSensor(const char* key, const char* type, unsigned int size, int index) {
  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        true,
                                                        (void *)key,
                                                        (void *)type,
                                                        (void *)(long long)size,
                                                        (void *)this)) {
    if (sensors->setObject(key, OSNumber::withNumber(index, 32))) {
      return true;
    } else {
      WarningLog("%s key sensor not set", key);
      return 0;
    }
  }

  WarningLog("%s key sensor not added", key);

  return 0;
}

int GeforceSensors::addTachometer(int index, const char* id) {
  UInt8 length = 0;
  void * data = 0;

  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                        false,
                                                        (void *)KEY_FAN_NUMBER,
                                                        (void *)&length,
                                                        (void *)&data, 0)) {
    char name[5];
    char key[5];

    bcopy(data, &length, 1);
    snprintf(name, 5, KEY_FORMAT_FAN_SPEED, length);

    if (addSensor(name, TYPE_FPE2, 2, index)) {
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
                                                            false,
                                                            (void *)KEY_FAN_NUMBER,
                                                            (void *)1,
                                                            (void *)&length,
                                                            0)) {
        WarningLog("error updating FNum value");
      }

      return length-1;
    }
  } else {
    WarningLog("error reading FNum value");
  }

  return -1;
}

//float GeforceSensors::getSensorValue(FakeSMCSensor *sensor)
//{
//    switch (sensor->getGroup()) {
//        case kFakeSMCTemperatureSensor:
//            return card.temp_get(&card);
//
//        case kNouveauCoreTemperatureSensor:
//            return card.core_temp_get(&card);
//
//        case kNouveauBoardTemperatureSensor:
//            return card.board_temp_get(&card);
//
//        case kFakeSMCFrequencySensor:
//            return card.clocks_get(&card, sensor->getIndex()) / 1000.0f;
//
//        case kNouveauPWMSensor:
//            return card.fan_pwm_get(&card);
//
//        case kFakeSMCTachometerSensor:
//            return card.fan_rpm_get(&card); // count ticks for 500ms
//
//        case kFakeSMCVoltageSensor:
//            return (float)card.voltage_get(&card) / 1000000.0f;
//    }
//
//    return 0;
//}

bool GeforceSensors::init(OSDictionary *properties) {
  //  DebugLog("Initialising...");

  if (!super::init(properties)) {
    return false;
  }

  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }

  return true;
}

IOService* GeforceSensors::probe(IOService *provider, SInt32 *score) {
  UInt32 vendor_id, device_id, class_id;
  DebugLog("Probing...");

  if (super::probe(provider, score) != this) { return 0; }

  s8 ret = 0;
  if (OSDictionary * dictionary = serviceMatching(kGenericPCIDevice)) {
    if (OSIterator * iterator = getMatchingServices(dictionary)) {
      //      ret = 1;
      IOPCIDevice* device = 0;
      do {
        device = OSDynamicCast(IOPCIDevice, iterator->getNextObject());
        if (!device) {
          break;
        }
        OSData *data = OSDynamicCast(OSData, device->getProperty(fVendor));
        vendor_id = 0;
        if (data) {
          vendor_id = *(UInt32*)data->getBytesNoCopy();
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

        if ((vendor_id==0x10de) && (class_id == 0x030000)) {
          InfoLog("found %x Nvidia chip", (unsigned int)device_id);
          card.pcidev = device;
          card.device_id = device_id;
          ret = 1; //TODO - count a number of cards
          card.card_index = ret;
          break;
        }
      } while (device);
    }
  }

  if (ret) {
    return this;
  } else {
    return 0;
  }

  return this;
}

SInt8 GeforceSensors::getVacantGPUIndex() {
  //Find card number
  char key[5];
  UInt8 length = 0;
  void * data = 0;


  for (UInt8 i = 0; i <= 0xf; i++) {

    snprintf(key, 5, KEY_FORMAT_GPU_DIODE_TEMPERATURE, i);
    //        if (isKeyHandled(key)) continue;
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                          true,
                                                          (void *)key,
                                                          (void *)&length,
                                                          (void *)&data, 0)) {
      continue;
    }

    snprintf(key, 5, KEY_FORMAT_GPU_HEATSINK_TEMPERATURE, i);
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                          true,
                                                          (void *)key,
                                                          (void *)&length,
                                                          (void *)&data,
                                                          0)) {
      continue;
    }

    snprintf(key, 5, KEY_FORMAT_GPU_PROXIMITY_TEMPERATURE, i);
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                          true,
                                                          (void *)key,
                                                          (void *)&length,
                                                          (void *)&data,
                                                          0)) {
      continue;
    }

    snprintf(key, 5, KEY_FORMAT_GPU_VOLTAGE, i);
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                          true,
                                                          (void *)key,
                                                          (void *)&length,
                                                          (void *)&data,
                                                          0)) {
      continue;
    }

    snprintf(key, 5, KEY_FAKESMC_FORMAT_GPU_FREQUENCY, i);
    if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCGetKeyValue,
                                                          true,
                                                          (void *)key,
                                                          (void *)&length,
                                                          (void *)&data,
                                                          0)) {
      continue;
    }

    return i;
  }

  return false;
}

bool GeforceSensors::start(IOService * provider) {
  DebugLog("Starting...");

  if (!super::start(provider)) {
    return false;
  }

  if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
    WarningLog("Can't locate fake SMC device, kext will not load");
    return false;
  }

  InfoLog("GeforceSensors by kozlek (C) 2012");

  struct nouveau_device *device = &card;

  //Find card number
  card.card_index = getVacantGPUIndex();

  if (card.card_index < 0) {
    nv_error(device, "failed to obtain vacant GPU index\n");
    return true;
  }

  // map device memory
  //  device->pcidev = (IOPCIDevice*)provider;
  if (device->pcidev) {
    device->pcidev->setMemoryEnable(true);

    if ((device->mmio = device->pcidev->mapDeviceMemoryWithIndex(0))) {
      nv_debug(device, "memory mapped successfully\n");
    } else {
      nv_error(device, "failed to map memory\n");
      return true;
    }
  } else {
    nv_error(device, "failed to assign PCI device\n");
    return true;
  }

  // identify chipset
  if (!nouveau_identify(device)) {
    return true;
  }
  // shadow and parse bios

  //try to load bios from registry first from "vbios" property created by  bootloader
  if (OSData *vbios = OSDynamicCast(OSData, provider->getProperty("vbios"))) {
    device->bios.size = vbios->getLength();
    device->bios.data = (u8*)IOMalloc(card.bios.size);
    memcpy(device->bios.data, vbios->getBytesNoCopy(), device->bios.size);
  }

  if (nouveau_bios_score(device, true) < 1) {
    if (!nouveau_bios_shadow(device)) {
      if (device->bios.data && device->bios.size) {
        IOFree(card.bios.data, card.bios.size);
        device->bios.data = NULL;
        device->bios.size = 0;
      }
      nv_error(device, "unable to shadow VBIOS\n");
      return true;
    }
  }

  nouveau_vbios_init(device);
  nouveau_bios_parse(device);

  // initialize funcs and variables
  if (!nouveau_init(device)) {
    nv_error(device, "unable to initialize monitoring driver\n");
    return false;
  }

  nv_info(device, "chipset: %s (NV%02X) bios: %02x.%02x.%02x.%02x\n",
          device->cname,
          (unsigned int)device->chipset,
          device->bios.version.major,
          device->bios.version.chip,
          device->bios.version.minor,
          device->bios.version.micro);

  if (device->card_type < NV_C0) {
    // init i2c structures
    nouveau_i2c_create(device);

    // setup nouveau i2c sensors
    nouveau_i2c_probe(device);
  }

  // Register sensors
  char key[5];

  if (card.core_temp_get || card.board_temp_get) {
    nv_debug(device, "registering i2c temperature sensors...\n");

    if (card.core_temp_get && card.board_temp_get) {
      snprintf(key, 5, KEY_FORMAT_GPU_DIODE_TEMPERATURE, card.card_index);
      this->addSensor(key, TYPE_SP78, 2, 0);

      snprintf(key, 5, KEY_FORMAT_GPU_HEATSINK_TEMPERATURE, card.card_index);
      addSensor(key, TYPE_SP78, 2, 0);
    } else if (card.core_temp_get) {
      snprintf(key, 5, KEY_FORMAT_GPU_PROXIMITY_TEMPERATURE, card.card_index);
      addSensor(key, TYPE_SP78, 2, 0);
    } else if (card.board_temp_get) {
      snprintf(key, 5, KEY_FORMAT_GPU_PROXIMITY_TEMPERATURE, card.card_index);
      addSensor(key, TYPE_SP78, 2, 0);
    }
  } else if (card.temp_get) {
    nv_debug(device, "registering temperature sensors...\n");

    snprintf(key, 5, KEY_FORMAT_GPU_PROXIMITY_TEMPERATURE, card.card_index);
    addSensor(key, TYPE_SP78, 2, 0);
  }

  if (card.clocks_get) {
    nv_debug(device, "registering clocks sensors...\n");

    if (card.clocks_get(&card, nouveau_clock_core) > 0) {
      snprintf(key, 5, KEY_FAKESMC_FORMAT_GPU_FREQUENCY, card.card_index);
      addSensor(key, TYPE_FREQ, TYPE_UI32_SIZE, nouveau_clock_core);
    }

    if (card.clocks_get(&card, nouveau_clock_shader) > 0) {
      snprintf(key, 5, KEY_FAKESMC_FORMAT_GPU_SHADER_FREQUENCY, card.card_index);
      addSensor(key, TYPE_FREQ, TYPE_UI32_SIZE, nouveau_clock_shader);
    }

    if (card.clocks_get(&card, nouveau_clock_rop) > 0) {
      snprintf(key, 5, KEY_FAKESMC_FORMAT_GPU_ROP_FREQUENCY, card.card_index);
      addSensor(key, TYPE_FREQ, TYPE_UI32_SIZE, nouveau_clock_rop);
    }

    if (card.clocks_get(&card, nouveau_clock_memory) > 0) {
      snprintf(key, 5, KEY_FAKESMC_FORMAT_GPU_MEMORY_FREQUENCY, card.card_index);
      addSensor(key, TYPE_FREQ, TYPE_UI32_SIZE, nouveau_clock_memory);
    }
  }

  if (card.fan_pwm_get || card.fan_rpm_get) {
    nv_debug(device, "registering PWM sensors...\n");

    if (card.fan_rpm_get && card.fan_rpm_get(device) > 0) {
      char title[6];
      snprintf (title, 6, "GPU %X", card.card_index + 1);

      UInt8 fanIndex = 0;

      if (addTachometer( fanIndex, title)) {
        if (card.fan_pwm_get && card.fan_pwm_get(device) > 0) {
          snprintf(key, 5, KEY_FAKESMC_FORMAT_GPUPWM, fanIndex);
          addSensor(key, TYPE_UI8, TYPE_UI8_SIZE, 0);
        }
      }
    }
  }

  if (card.voltage_get && card.voltage.supported) {
    nv_debug(device, "registering voltage sensors...\n");
    snprintf(key, 5, KEY_FORMAT_GPU_VOLTAGE, card.card_index);
    addSensor(key, TYPE_FP2E, TYPE_FPXX_SIZE, 0);
  }

  nv_info(device, "started\n");

  return true;
}

void GeforceSensors::stop (IOService* provider) {
  DebugLog("Stoping...");

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

void GeforceSensors::free(void) {
  if (card.mmio) {
    OSSafeReleaseNULL(card.mmio);
  }
  if (card.bios.data) {
    IOFree(card.bios.data, card.bios.size);
    card.bios.data = 0;
  }

  sensors->release();

  super::free();
}

IOReturn GeforceSensors::callPlatformFunction(const OSSymbol *functionName,
                                              bool waitForFunction,
                                              void *param1,
                                              void *param2,
                                              void *param3,
                                              void *param4) {
  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    const char* key = (const char*)param1;
    char * data = (char*)param2;
    //    UInt32 size = (UInt64)param3;

    if (key && data) {
      OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(key));
      if (number) {

        //        UInt32 index = number->unsigned16BitValue();
        //
        //        if (index < nvclock.num_cards) {
        //
        //          if (!set_card(index)){
        //            char buf[80];
        //            WarningLog("%s", get_error(buf, 80));
        //            return kIOReturnSuccess;
        //          }

        UInt32 value = 0;

        switch (key[0]) {
          case 'T':
            switch (key[3]) {
              case 'D':
              case 'P':
                value = card.temp_get(&card);
                break;
              case 'H':
                value = card.core_temp_get(&card);
                break;
            }
            //bcopy(&value, data, 2);
            memcpy(data, &value, 2);
            break;
          case 'C':
            switch (key[3]) {
              case 'C':
                value=swap_value((UInt16)(card.clocks_get(&card, nouveau_clock_core) / 1000.0f));
                //bcopy(&value, data, 2);
                memcpy(data, &value, 4);
                break;
              case 'S':
                value=swap_value((UInt16)(card.clocks_get(&card, nouveau_clock_shader) / 1000.0f));
                //bcopy(&value, data, 2);
                memcpy(data, &value, 4);
                break;
              case 'M':
                value=swap_value((UInt16)(card.clocks_get(&card, nouveau_clock_memory) / 1000.0f));
                //bcopy(&value, data, 2);
                memcpy(data, &value, 4);
                break;
              case 'R':
                value=swap_value((UInt16)(card.clocks_get(&card, nouveau_clock_rop) / 1000.0f));
                //bcopy(&value, data, 2);
                memcpy(data, &value, 4);
                break;
            }
          case 'V':
            switch (key[3]) {
              case 'G':
                //  value = encode_fp2e( (float)card.voltage_get(&card) / 1000000.0f);
                //encode_fp2e has input as mV, output is V in fixed format
                value = encode_fp2e( (float)card.voltage_get(&card) / 1000.0f);  // Fix for fp2e encoding
                //bcopy(&value, data, 2);
                memcpy(data, &value, 2);
                break;
            }
        }
        return kIOReturnSuccess;
      }
    }
    return kIOReturnBadArgument;
  }

  return super::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);
}

