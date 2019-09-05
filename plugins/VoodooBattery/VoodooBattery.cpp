/*
 --- VoodooBattery ---
 (C) 2009 Superhai

 Changelog
 ----------------------------------------------
 1.2.0  19/1/09
 - Initial Release
 1.2.1  07/4/09
 - Bugfixes, minor mods
 ----------------------------------------------

 Contact  http://www.superhai.com/

 */
/*
 1.2 - 1.4 small improvements
 ...
 1.4.1 Slice 2017
 Make the kext is a plugin for FakeSMC to show battery state in HWMonitor
 */

#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/pwr_mgt/IOPM.h>

#include "Support.h"
#include "VoodooBattery.h"
#include "../../utils/definitions.h"

#pragma mark -
#pragma mark VoodooBattery Controller
#pragma mark -
#pragma mark IOService

#undef super
#define super IOHIKeyboard

OSDefineMetaClassAndStructors(Button, IOHIKeyboard)

bool Button::sendEvent(UInt8 keyEvent)
{
  AbsoluteTime now;
  clock_get_uptime((uint64_t*)&now);

  dispatchKeyboardEvent( keyEvent,
                        /*direction*/ true,
                        /*timeStamp*/ *((AbsoluteTime*)&now) );
  return true;
}

bool Button::attach(IOService *provider)
{
  if( !super::attach(provider) )  return false;
  _controller = (ButtonController *)provider;
  _controller->retain();
  return true;
}

void Button::detach( IOService * provider )
{
  assert(_controller == provider);
  _controller->release();
  _controller = 0;

  super::detach(provider);
}



#undef super
#define super IOService

OSDefineMetaClassAndStructors(ButtonController, IOService)

bool ButtonController::init(OSDictionary * properties)
{
  //
  // Initialize this object's minimal state.  This is invoked right after this
  // object is instantiated.
  //

  if (!super::init(properties))  return false;

  return true;
}

ButtonController * ButtonController::probe(IOService * provider, SInt32 * score)
{
  if (!super::probe(provider, score))  return 0;
  return this;
}

bool ButtonController::start(IOService * provider)
{
  if (!super::start(provider))  return false;
  ACButton2 = new Button;
//  ACButton2->registerService();
  return true;
}

void ButtonController::stop(IOService * provider)
{
  if (ACButton2) ACButton2->release();
  super::stop(provider);
}

bool ButtonController::sendEvent(UInt8 keyEvent)
{
  return ACButton2->sendEvent(keyEvent);
}

OSDefineMetaClassAndStructors(VoodooBattery, IOService)

#define kPowerStateCount 3
static IOPMPowerState myStates[2] = {
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};
/*static IOPMPowerState myStates[ kPowerStateCount ] =
 {
 { 1,0,0,0,0,0,0,0,0,0,0,0 },
 { 1,0,kIOPMDoze,kIOPMDoze,0,0,0,0,0,0,0,0 },
 { 1,kIOPMDeviceUsable,kIOPMPowerOn,kIOPMPowerOn,0,0,0,0,0,0,0,0 }
 };
 */

bool VoodooBattery::addSensor(const char* key, const char* type, unsigned int size, int index) {
  if (kIOReturnSuccess == fakeSMC->callPlatformFunction(kFakeSMCAddKeyHandler,
                                                        false,
                                                        (void *)key,
                                                        (void *)type,
                                                        (void *)(long long)size,
                                                        (void *)this)) {
    if (sensors) {
      return sensors->setObject(key, OSNumber::withNumber(index, 32));
    }
  }
  return false;
}

bool VoodooBattery::init(OSDictionary *properties) {
  if (!super::init(properties)) {
    return false;
  }
  firstTime = true;
  ExternalPowerConnected = false;
  if (!(sensors = OSDictionary::withCapacity(0))) {
    return false;
  }
  brightnessLevels = NULL;
  brightnessCount = 0;
  return true;
}

IOService * VoodooBattery::probe(IOService * provider, SInt32 * score) {

  // Call superclass
  if (IOService::probe(provider, score) != this) { return 0; }

  IORegistryIterator * iterator;
  IORegistryEntry * entry;
  OSString * pnp;

  // We need to look for batteries and if not found we unload
  BatteryCount = 0;
  iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
  pnp = OSString::withCString(PnpDeviceIdBattery);

  if (iterator) {
    while ((entry = iterator->getNextObject())) {
      if (entry->compareName(pnp)) {
        DebugLog("Found acpi pnp battery");
        BatteryDevice[BatteryCount++] = OSDynamicCast(IOACPIPlatformDevice, entry);
        if (BatteryCount >= MaxBatteriesSupported) break;
      }
    }
    iterator->release();
    iterator = 0;
  }

  IOLog("Found %u batteries\n", BatteryCount);
  if (BatteryCount == 0) return 0;

  // We will also try to find an A/C adapter in acpi space
  AcAdapterCount = 0;
  iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
  pnp = OSString::withCString(PnpDeviceIdAcAdapter);

  if (iterator) {
    while ((entry = iterator->getNextObject())) {
      if (entry->compareName(pnp)) {
        DebugLog("Found acpi pnp ac adapter");
        AcAdapterDevice[AcAdapterCount++] = OSDynamicCast(IOACPIPlatformDevice, entry);
        if (AcAdapterCount >= MaxAcAdaptersSupported) break;
      }
    }
    iterator->release();
    iterator = 0;
  }
  IOLog("Found %u ac adapters\n", AcAdapterCount);

  // look for LID device if any
  iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
  pnp = OSString::withCString(PnpDeviceIdLid);
  if (iterator) {
    while ((entry = iterator->getNextObject())) {
      if (entry->compareName(pnp)) {
        DebugLog("Found Lid device");
        LidDevice = OSDynamicCast(IOACPIPlatformDevice, entry);
        break;
      }
    }
    iterator->release();
    iterator = 0;
  }
//PNLFDevice APP0002 PnpDeviceIdPnlf
  iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
  pnp = OSString::withCString(PnpDeviceIdPnlf);
  if (iterator) {
    while ((entry = iterator->getNextObject())) {
      if (entry->compareName(pnp)) {
        DebugLog("Found PNLF device");
        PNLFDevice = OSDynamicCast(IOACPIPlatformDevice, entry);
        break;
      }
    }
    iterator->release();
    iterator = 0;
  }


  return this;
}

bool VoodooBattery::start(IOService * provider) {
  bool isBattery = false;
  char key[5];
  // Call superclass
  if (!IOService::start(provider)) { return false; }

  if (!(fakeSMC = waitForService(serviceMatching(kFakeSMCDeviceService)))) {
    WarningLog("Can't locate fake SMC device, kext will not load");
    return false;
  }

  // Printout banner
  InfoLog("%s %s (%s) %s %s",
          KextProductName,
          KextVersion,
          KextConfig,
          KextBuildDate,
          KextBuildTime /*,
                         KextOSX */);
  InfoLog("(C) 2009 Superhai, All Rights Reserved, 2017 Slice");

  WorkLoop = getWorkLoop();
  Poller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action,
                                                                           this,
                                                                           &VoodooBattery::Update));
  if (!Poller || !WorkLoop) { return false; }
  if (WorkLoop->addEventSource(Poller) != kIOReturnSuccess) { return false; }


  for (UInt8 i = 0; i < BatteryCount; i++) {
    DebugLog("Attaching and starting battery %s", BatteryDevice[i]->getName());
    BatteryPowerSource[i] = AppleSmartBattery::NewBattery();
    fBatteryGate[i] = IOCommandGate::commandGate(BatteryPowerSource[i]);
    WorkLoop->addEventSource(fBatteryGate[i]);

    if (BatteryPowerSource[i]) {
      if (BatteryPowerSource[i]->attach(BatteryDevice[i]) && BatteryPowerSource[i]->start(this)) {
        BatteryPowerSource[i]->registerService(0);
        BatteryPowerSource[i]->ParentService = this;
      } else {
        ErrorLog("Error on battery attach %u", i);
        return false;
      }
      isBattery = true;
    }
  }

  for (UInt8 i = 0; i < AcAdapterCount; i++) {
//    if (attach(AcAdapterDevice[i])) {
      InfoLog("A/C adapter %s available", AcAdapterDevice[i]->getName());
 //   }
  }

//  if (PNLFDevice) {
//    attach(PNLFDevice);
//  }

///  ACButtonController = new ButtonController;
//  InfoLog("new");
///  ACButtonController->start(provider);
//  InfoLog("start");
//  ACButtonController->registerService();
//  InfoLog("register");
//  ACButtonController->attach(this);
//  ACButtonController->IOService::start(this);

/*  ACButton = new Button;
  ACButton->attach(this);
  ACButton->registerService(); */
/*  ACButtonController = new ButtonController;
  if (attach(ACButtonController)) {
    InfoLog("ACButton available");
    ACButtonController->start(provider);
//    ACButtonController->registerService(0);
  }
*/

  if (isBattery) {
    PMinit();                      // Powermanagement
    //  registerPowerDriver(this, myStates, kPowerStateCount);
    registerPowerDriver(this, myStates, 2);
    provider->joinPMtree(this);
  }

  for (UInt32 i = 0; i < 5; i++) {
    IOSleep(1000);
    CheckDevices();
  }

  if (isBattery) {
    for (int Index = 0; Index < BatteryCount; Index++) {
      if (BatteryConnected[Index]) {
        snprintf(key, 5, KEY_FORMAT_BAT_VOLTAGE, Index);
        DebugLog("Adding key %s", key);
        addSensor(key, TYPE_UI16, 2, Index);
        snprintf(key, 5, KEY_FORMAT_BAT_AMPERAGE, Index);
        DebugLog("Adding key %s", key);
        addSensor(key, TYPE_UI16, 2, Index);
        snprintf(key, 5, KEY_FORMAT_BAT_STATUS, Index);
        DebugLog("Adding key %s", key);
        addSensor(key, TYPE_UI16, 2, Index);
        snprintf(key, 5, KEY_FORMAT_BAT_REMAINING_CAPACITY, Index);
        DebugLog("Adding key %s", key);
        addSensor(key, TYPE_UI16, 2, Index);
      }
    }
    snprintf(key, 5, KEY_BAT_POWERED);
    addSensor(key, TYPE_FLAG, 1, 0);
    snprintf(key, 5, KEY_NUMBER_OF_BATTERIES);
    addSensor(key, TYPE_UI8, 1, 0);
    snprintf(key, 5, KEY_BAT_INSERTED);
    addSensor(key, TYPE_UI8, 1, 0);
    snprintf(key, 5, KEY_BAT_CHARGE_CODE);
    addSensor(key, TYPE_UI8, 1, 0);
    snprintf(key, 5, KEY_NUMBER_OF_ADAPTERS);
    addSensor(key, TYPE_UI8, 1, 0);
    snprintf(key, 5, KEY_CONNECTED_ADAPTER);
    addSensor(key, TYPE_UI8, 1, 0);
    snprintf(key, 5, KEY_ADAPTER_AMPERAGE);
    addSensor(key, TYPE_UI16, 2, 0);
  }
  if (LidDevice && attach(LidDevice)) {
    snprintf(key, 5, KEY_LID_CLOSED);
    addSensor(key, TYPE_UI8, 1, 0);
  }

  while (PNLFDevice) {
    OSObject* result = 0;
    // check for brightness methods
    if (kIOReturnSuccess != PNLFDevice->validateObject(MethodBCL) || kIOReturnSuccess != PNLFDevice->validateObject(MethodBCM) || kIOReturnSuccess != PNLFDevice->validateObject(MethodBQC)) {
      WarningLog("no methods in PNLF");
      break;
    }
    // methods are there, so now try to collect brightness levels
    if (kIOReturnSuccess != PNLFDevice->evaluateObject(MethodBCL, &result)) {
      WarningLog("no _BCL in PNLF");
      break;
    }
    OSArray* array = OSDynamicCast(OSArray, result);
    if (!array) {
      WarningLog("_BCL returned non-array package");
      break;
    }
    int count = array->getCount();
    if (count < 4) {
      WarningLog("_BCL returned invalid package, <4");
      break;
    }
/*    brightnessCount = count;
    brightnessLevels = new int[brightnessCount];
    if (!brightnessLevels) {
      WarningLog("brightnessLevels new int[] failed");
      break;
    }
    for (int i = 0; i < brightnessCount; i++) {
      OSNumber* num = OSDynamicCast(OSNumber, array->getObject(i));
      int brightness = num ? num->unsigned32BitValue() : 0;
      brightnessLevels[i] = brightness;
      DebugLog("brightness[%d]=%x", i, brightness);
    } */
    break;
  }

  return true;
}

void VoodooBattery::stop(IOService * provider)
{
  sensors->flushCollection();
  Poller->cancelTimeout();
  PMstop();
  IOWorkLoop *wl = getWorkLoop();
  for (UInt8 i = 0; i < BatteryCount; i++) {
    BatteryPowerSource[i]->ParentService = 0;
    BatteryPowerSource[i]->detach(BatteryDevice[i]);
    BatteryPowerSource[i]->stop(this);

    if (wl) {
      wl->removeEventSource(fBatteryGate[i]);
    }
    fBatteryGate[i]->free();
    fBatteryGate[i] = NULL;

  }
/*  ACButton->detach(this);
  ACButton->release(); */
//  ACButtonController->detach(this);
  ACButtonController->stop(this);

  if (brightnessLevels)
  {
    delete[] brightnessLevels;
    brightnessLevels = 0;
  }

  IOService::stop(provider);
}

void VoodooBattery::free() {
  sensors->release();
  IOService::free();
}

IOReturn VoodooBattery::setPowerState(unsigned long state, IOService * device) {
  if (state) {
    DebugLog("%s We are waking up", device->getName());
    QuickPoll = QuickPollCount;
    CheckDevices();
  } else {
    DebugLog("%s We are sleeping", device->getName());
    QuickPoll = 0;
  }
  return kIOPMAckImplied;
}

IOReturn VoodooBattery::message(UInt32 type, IOService * provider, void * argument) {
  if (type == kIOACPIMessageDeviceNotification) {
    DebugLog("%s kIOACPIMessageDeviceNotification", provider->getName());
    QuickPoll = QuickPollCount;
    CheckDevices();
  } else {
    DebugLog("%s %08X", provider->getName(), type);
  }

  return kIOReturnSuccess;
}

#pragma mark Own

void VoodooBattery::Update(void) {
  DebugLog("ExternalPowerConnected %x BatteriesAreFull %x", ExternalPowerConnected, BatteriesAreFull);
  if (ExternalPowerConnected && BatteriesAreFull) {
    DebugLog("NoPoll");
  } else {
    if (QuickPoll) {
      DebugLog("QuickPoll %u", QuickPoll);
      Poller->setTimeoutMS(QuickPollInterval);
      QuickPoll--;
    } else {
      DebugLog("NormalPoll");
      Poller->setTimeoutMS(NormalPollInterval);
    }
  }
  BatteriesAreFull = true;
  for (UInt8 i = 0; i < BatteryCount; i++) {
    if (BatteryConnected[i]) { BatteryStatus(i); }
    if (!AcAdapterCount) { ExternalPowerConnected |= CalculatedAcAdapterConnected[i]; }
  }
  if (!AcAdapterCount) { ExternalPower(ExternalPowerConnected); }
}

void VoodooBattery::CheckDevices(void) {
  UInt32 acpi;
  bool change;
  bool wasConnected = ExternalPowerConnected;
  Poller->cancelTimeout();
  BatteriesConnected = false;
  DebugLog("CheckDevices");
  for (UInt8 i = 0; i < BatteryCount; i++) {
    acpi = 0;
    if (kIOReturnSuccess == BatteryDevice[i]->evaluateInteger(AcpiStatus, &acpi)) {  //_STA
      DebugLog("Return status %x\n", acpi);
      change = BatteryConnected[i];
      BatteryConnected[i] = (acpi & 0x10) ? true : false;
      if (BatteryConnected[i] != change) { BatteryInformation(i); }
      BatteriesConnected |= BatteryConnected[i];
    }
  }
  ExternalPowerConnected = false;
  for (UInt8 i = 0; i < AcAdapterCount; i++) {
    acpi = 0;
    if (kIOReturnSuccess == AcAdapterDevice[i]->evaluateInteger(AcpiPowerSource, &acpi)) {  //_PSR
      DebugLog("Return power source %x\n", acpi);
      AcAdapterConnected[i] = acpi ? true : false;
      ExternalPowerConnected |= AcAdapterConnected[i];
    }
  }
  if (BatteriesConnected) {
    Update();
  } else {
    ExternalPowerConnected = true;    // Safe to assume without batteries you need ac
  }

  if (wasConnected && !ExternalPowerConnected) {
    //reduce brightness
    DebugLog("reduce brightness ");
//    ACButtonController->sendEvent(0x90); //0x90  //0x02
//    ChangeBrightness(-4);
  } else if (!wasConnected && ExternalPowerConnected){
    //increase brightness
    DebugLog("increase brightness ");
//    ChangeBrightness(4);
//    ACButtonController->sendEvent(0x91);  //0x91  //0x03
  } //else nothing to do
  ExternalPower(ExternalPowerConnected);
  DebugLog("BatteriesConnected %x ExternalPowerConnected %x", BatteriesConnected, ExternalPowerConnected);
}

void VoodooBattery::ChangeBrightness(int Shift)
{
  UInt32 BrightBefore = 0;
  int index = 2;
  if (PNLFDevice && (brightnessCount > 2)) {
    PNLFDevice->evaluateInteger(MethodBQC, &BrightBefore);
    DebugLog("BrightBefore = %x", BrightBefore);
    while (index < brightnessCount) {
      if (brightnessLevels[index++] >= BrightBefore)
        break;
    }
    DebugLog("at index = %d", index);
    index += Shift - 1;
    index = (index < brightnessCount)? index : brightnessCount - 1;
    index = (index > 2)? index: 2;

    OSNumber* num = OSNumber::withNumber(brightnessLevels[index], 32);
    if (kIOReturnSuccess != PNLFDevice->evaluateObject(MethodBCM, NULL, (OSObject**)&num, 1)) {
      DebugLog("_BCM returned error\n");
    }
    num->release();
  }
}

void VoodooBattery::GetBatteryInfoEx(UInt8 battery, OSObject * acpi) {
  OSArray * info = OSDynamicCast(OSArray, acpi);
  if (GetValueFromArray(info, 1) == 0x00000000) PowerUnitIsWatt = true;
  Battery[battery].DesignCapacity = GetValueFromArray(info, 2);
  Battery[battery].LastFullChargeCapacity = GetValueFromArray(info, 3);
  Battery[battery].Technology = GetValueFromArray(info, 4);
  Battery[battery].DesignVoltage = GetValueFromArray(info, 5);
  Battery[battery].DesignCapacityWarning = GetValueFromArray(info, 6);
  Battery[battery].DesignCapacityLow = GetValueFromArray(info, 7);
  if (!Battery[battery].DesignVoltage) { Battery[battery].DesignVoltage = DummyVoltage; }

  if (PowerUnitIsWatt) {
    UInt32 volt = Battery[battery].DesignVoltage / 1000;
    InfoLog("Battery voltage %d,%03d", volt, Battery[battery].DesignVoltage % 1000);
    if ((Battery[battery].DesignCapacity / volt) < 900) {
      WarningLog("Battery reports mWh but uses mAh (%u)",
                 Battery[battery].DesignCapacity);
      PowerUnitIsWatt = false;
    } else {
      Battery[battery].DesignCapacity /= volt;
      Battery[battery].LastFullChargeCapacity /= volt;
      Battery[battery].DesignCapacityWarning /= volt;
      Battery[battery].DesignCapacityLow /= volt;
    }
  }

  if (Battery[battery].DesignCapacity < Battery[battery].LastFullChargeCapacity) {
    WarningLog("Battery reports lower design capacity than maximum charged (%u/%u)",
               Battery[battery].DesignCapacity, Battery[battery].LastFullChargeCapacity);
    if (Battery[battery].LastFullChargeCapacity < AcpiMax) {
      UInt32 temp = Battery[battery].DesignCapacity;
      Battery[battery].DesignCapacity = Battery[battery].LastFullChargeCapacity;
      Battery[battery].LastFullChargeCapacity = temp;
    }
  }
  Battery[battery].Cycle  = GetValueFromArray(info, 8);
}

void VoodooBattery::GetBatteryInfo(UInt8 battery, OSObject * acpi) {
  OSArray * info = OSDynamicCast(OSArray, acpi);
  if (GetValueFromArray(info, 0) == 0x00000000) PowerUnitIsWatt = true;
  Battery[battery].DesignCapacity = GetValueFromArray(info, 1);
  Battery[battery].LastFullChargeCapacity = GetValueFromArray(info, 2);
  Battery[battery].Technology = GetValueFromArray(info, 3);
  Battery[battery].DesignVoltage = GetValueFromArray(info, 4);
  Battery[battery].DesignCapacityWarning = GetValueFromArray(info, 5);
  Battery[battery].DesignCapacityLow = GetValueFromArray(info, 6);

  if (!Battery[battery].DesignVoltage) { Battery[battery].DesignVoltage = DummyVoltage; }

  if (PowerUnitIsWatt) {
    UInt32 volt = Battery[battery].DesignVoltage / 1000;
    InfoLog("Battery voltage %d,%03d", volt, Battery[battery].DesignVoltage % 1000);
    if ((Battery[battery].DesignCapacity / volt) < 900) {
      WarningLog("Battery reports mWh but uses mAh (%u)",
                 Battery[battery].DesignCapacity);
      PowerUnitIsWatt = false;
    } else {
      Battery[battery].DesignCapacity /= volt;
      Battery[battery].LastFullChargeCapacity /= volt;
      Battery[battery].DesignCapacityWarning /= volt;
      Battery[battery].DesignCapacityLow /= volt;
    }
  }

  if (Battery[battery].DesignCapacity < Battery[battery].LastFullChargeCapacity) {
    WarningLog("Battery reports lower design capacity than maximum charged (%u/%u)",
               Battery[battery].DesignCapacity, Battery[battery].LastFullChargeCapacity);
    if (Battery[battery].LastFullChargeCapacity < AcpiMax) {
      UInt32 temp = Battery[battery].DesignCapacity;
      Battery[battery].DesignCapacity = Battery[battery].LastFullChargeCapacity;
      Battery[battery].LastFullChargeCapacity = temp;
    }
  }

  if (Battery[battery].LastFullChargeCapacity && Battery[battery].DesignCapacity) {
    UInt32 last    = Battery[battery].LastFullChargeCapacity;
    UInt32 design  = Battery[battery].DesignCapacity;
    //UInt32 cycle  = 2 * (10 - (last * 10 / design)) / 3;
    Battery[battery].Cycle  = (design - last) * 1000 / design; //assume battery designed for 1000 cycles
  }
}

void VoodooBattery::PublishBatteryInfo(UInt8 battery, OSObject * acpi, int Ext)
{
  OSArray * info = OSDynamicCast(OSArray, acpi);
  // Publish to our IOKit powersource
  BatteryPowerSource[battery]->setMaxCapacity(Battery[battery].LastFullChargeCapacity);
  BatteryPowerSource[battery]->setDesignCapacity(Battery[battery].DesignCapacity);
  BatteryPowerSource[battery]->setExternalChargeCapable(true);
  BatteryPowerSource[battery]->setBatteryInstalled(true);
  BatteryPowerSource[battery]->setLocation(StartLocation + battery);
  BatteryPowerSource[battery]->setAdapterInfo(0);
  BatteryPowerSource[battery]->setDeviceName(GetSymbolFromArray(info, 9 + Ext));
  BatteryPowerSource[battery]->setSerial(GetSymbolFromArray(info, 10 + Ext));
  BatteryPowerSource[battery]->setBatteryType(GetSymbolFromArray(info, 11 + Ext));
  BatteryPowerSource[battery]->setSerialString(GetSymbolFromArray(info, 10 + Ext));
  BatteryPowerSource[battery]->setManufacturer(GetSymbolFromArray(info, 12 + Ext));
  BatteryPowerSource[battery]->setCycleCount(Battery[battery].Cycle);
}


void VoodooBattery::BatteryInformation(UInt8 battery) {
  PowerUnitIsWatt = false;
  if (BatteryConnected[battery]) {
    DebugLog("Battery %u Connected", battery);
    OSObject * acpi = NULL;

    if (kIOReturnSuccess == BatteryDevice[battery]->evaluateObject(AcpiBatteryInformationEx, &acpi)) { //_BIX
      if (acpi && (OSTypeIDInst(acpi) == OSTypeID(OSArray))) {
        GetBatteryInfoEx(battery, acpi);
        PublishBatteryInfo(battery, acpi, 7);

        BatteryPowerSource[battery]->updateStatus();
        acpi->release();
        return;
      } else {
        WarningLog("Error in ACPI data");
        //BatteryConnected[battery] = false;
        //try to get _BIF
      }
    }

    if (kIOReturnSuccess == BatteryDevice[battery]->evaluateObject(AcpiBatteryInformation, &acpi)) { //_BIF
      if (acpi && (OSTypeIDInst(acpi) == OSTypeID(OSArray))) {
        GetBatteryInfo(battery, acpi);
        PublishBatteryInfo(battery, acpi, 0);
        acpi->release();
      } else {
        WarningLog("Error in ACPI data");
        BatteryConnected[battery] = false;
      }
    }
  } else {
    DebugLog("Battery %u Disconnected", battery);
    BatteryPowerSource[battery]->BlankOutBattery();
    CalculatedAcAdapterConnected[battery] = false;
  }
  BatteryPowerSource[battery]->updateStatus();
}

void VoodooBattery::BatteryStatus(UInt8 battery) {
  OSObject * acpi = NULL;

  if (kIOReturnSuccess == BatteryDevice[battery]->evaluateObject(AcpiBatteryStatus, &acpi)) { //_BST
    if (acpi && (OSTypeIDInst(acpi) == OSTypeID(OSArray))) {
      OSArray * status = OSDynamicCast(OSArray, acpi);
      setProperty(BatteryDevice[battery]->getName(), status);

      UInt32 TimeRemaining = 0;
      UInt32 HighAverageBound, LowAverageBound;
      bool bogus = false;
      bool warning = false;
      bool critical = false;
      Battery[battery].State = GetValueFromArray(status, 0);
      Battery[battery].PresentRate = GetValueFromArray(status, 1);
      Battery[battery].RemainingCapacity = GetValueFromArray(status, 2);
      Battery[battery].PresentVoltage = GetValueFromArray(status, 3);

      if (PowerUnitIsWatt) {
        UInt32 volt = Battery[battery].DesignVoltage / 1000;
        Battery[battery].PresentRate /= volt;
        Battery[battery].RemainingCapacity /= volt;
      }
      // Average rate calculation
      if (!Battery[battery].PresentRate || (Battery[battery].PresentRate == AcpiUnknown)) {
        UInt32 delta = (Battery[battery].RemainingCapacity > Battery[battery].LastRemainingCapacity ?
                        Battery[battery].RemainingCapacity - Battery[battery].LastRemainingCapacity :
                        Battery[battery].LastRemainingCapacity - Battery[battery].RemainingCapacity);
        UInt32 interval = QuickPoll ? 3600 / (QuickPollInterval / 1000) : 3600 / (NormalPollInterval / 1000);
        Battery[battery].PresentRate = delta ? delta * interval : interval;
      }

      if (!Battery[battery].AverageRate) { Battery[battery].AverageRate = Battery[battery].PresentRate; }
      Battery[battery].AverageRate += Battery[battery].PresentRate;
      Battery[battery].AverageRate >>= 1;
      HighAverageBound = Battery[battery].PresentRate * (100 + AverageBoundPercent) / 100;
      LowAverageBound  = Battery[battery].PresentRate * (100 - AverageBoundPercent) / 100;

      if (Battery[battery].AverageRate > HighAverageBound) {
        Battery[battery].AverageRate = HighAverageBound;
      }

      if (Battery[battery].AverageRate < LowAverageBound) {
        Battery[battery].AverageRate = LowAverageBound;
      }
      // Remaining capacity
      if (!Battery[battery].RemainingCapacity || (Battery[battery].RemainingCapacity == AcpiUnknown)) {
        WarningLog("Battery %u has no remaining capacity reported", battery);
      } else {
        TimeRemaining = (Battery[battery].AverageRate ?
                         60 * Battery[battery].RemainingCapacity / Battery[battery].AverageRate :
                         60 * Battery[battery].RemainingCapacity);
        BatteryPowerSource[battery]->setTimeRemaining(TimeRemaining);
        TimeRemaining = (Battery[battery].PresentRate ?
                         60 * Battery[battery].RemainingCapacity / Battery[battery].PresentRate :
                         60 * Battery[battery].RemainingCapacity);
        BatteryPowerSource[battery]->setInstantaneousTimeToEmpty(TimeRemaining);
      }
      // Voltage
      if (!Battery[battery].PresentVoltage || (Battery[battery].PresentVoltage == AcpiUnknown)) {
        Battery[battery].PresentVoltage = Battery[battery].DesignVoltage;
      }
      // Check battery state
      switch (Battery[battery].State & 0x3) {
        case BatteryFullyCharged:
          DebugLog("Battery %u Full", battery);
          CalculatedAcAdapterConnected[battery] = true;
          BatteriesAreFull &= true;
          BatteryPowerSource[battery]->setIsCharging(false);
          BatteryPowerSource[battery]->setFullyCharged(true);

          if ((Battery[battery].LastRemainingCapacity >= Battery[battery].DesignCapacity) ||
              (Battery[battery].LastRemainingCapacity == 0)) {
            BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].LastFullChargeCapacity);
          } else {
            BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].LastRemainingCapacity);
          }

          BatteryPowerSource[battery]->setInstantAmperage((SInt32) Battery[battery].PresentRate);
          BatteryPowerSource[battery]->setAmperage((SInt32) Battery[battery].AverageRate);
          break;
        case BatteryDischarging:
          DebugLog("Battery %u Discharging", battery);
          CalculatedAcAdapterConnected[battery] = false;
          BatteriesAreFull = false;
          BatteryPowerSource[battery]->setIsCharging(false);
          BatteryPowerSource[battery]->setFullyCharged(false);
          BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
          BatteryPowerSource[battery]->setInstantAmperage((SInt32) Battery[battery].PresentRate * -1);
          BatteryPowerSource[battery]->setAmperage((SInt32) Battery[battery].AverageRate * -1);
          break;
        case BatteryCharging:
          DebugLog("Battery %u Charging", battery);
          CalculatedAcAdapterConnected[battery] = true;
          BatteriesAreFull = false;
          BatteryPowerSource[battery]->setIsCharging(true);
          BatteryPowerSource[battery]->setFullyCharged(false);
          BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
          BatteryPowerSource[battery]->setInstantAmperage((SInt32) Battery[battery].PresentRate);
          BatteryPowerSource[battery]->setAmperage((SInt32) Battery[battery].AverageRate);
          break;
        default:
          WarningLog("Bogus status data from battery %u (%x)", battery, Battery[battery].State);
          BatteriesAreFull = false;
          BatteryPowerSource[battery]->setIsCharging(false);
          BatteryPowerSource[battery]->setFullyCharged(false);
          BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
          bogus = true;
          break;
      }
      warning    = Battery[battery].RemainingCapacity <= Battery[battery].DesignCapacityWarning;
      critical  = Battery[battery].RemainingCapacity <= Battery[battery].DesignCapacityLow;

      if (Battery[battery].State & BatteryCritical) {
        DebugLog("Battery %u is critical", battery);
        critical = true;
      }

      if (!warning && TimeRemaining < 10) { warning = true; }
      if (!critical && TimeRemaining < 5) { critical = true; }

      BatteryPowerSource[battery]->setAtWarnLevel(warning);
      BatteryPowerSource[battery]->setAtCriticalLevel(critical);
      BatteryPowerSource[battery]->setVoltage(Battery[battery].PresentVoltage);

      if (critical && bogus) {
        BatteryPowerSource[battery]->setErrorCondition((OSSymbol *) permanentFailureKey);
      }

      BatteryPowerSource[battery]->rebuildLegacyIOBatteryInfo();
      BatteryPowerSource[battery]->settingsChangedSinceUpdate = true;
      BatteryPowerSource[battery]->updateStatus();
      acpi->release();
    } else {
      WarningLog("Error in ACPI data");
      BatteryConnected[battery] = false;
    }
  }
  Battery[battery].LastRemainingCapacity = Battery[battery].RemainingCapacity;
}

void VoodooBattery::ExternalPower(bool status) {
  IOPMrootDomain * rd = getPMRootDomain();
  rd->receivePowerNotification(kIOPMSetACAdaptorConnected | (kIOPMSetValue * status));

  for (UInt8 i = 0; i < BatteryCount; i++) {
    BatteryPowerSource[i]->setExternalConnected(status);
  }
}

IOReturn VoodooBattery::callPlatformFunction(const OSSymbol *functionName,
                                             bool waitForFunction,
                                             void *param1,
                                             void *param2,
                                             void *param3,
                                             void *param4) {
  SInt32 value;
  UInt32 index = 0;
  int batNum = 0;
  //  OSString* key;


  if (functionName->isEqualTo(kFakeSMCGetValueCallback)) {
    const char* name = (const char*)param1;
    void* data = param2;

    if (name && data) {
      //      WarningLog("callPF for key %s", name);
      if ((name[0] == 'B') && (name[2] == 'A')) {
        batNum = name[1] - 0x30;
        if (OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(name))) {
          index = number->unsigned16BitValue();
          if (index >= MaxBatteriesSupported) {
            WarningLog("called battery # %d for A%c", index, name[3]);
            return kIOReturnBadArgument;
          }
        }

        BatteryStatus(batNum);
        switch (name[3]) {
          case 'C':
            value = Battery[batNum].AverageRate;
            break;
          case 'V':
            value = Battery[batNum].PresentVoltage;
            break;
          default:
            return kIOReturnBadArgument;
        }
        memcpy(data, &value, 2);
        return kIOReturnSuccess;
      } else if ((name[0] == 'B') && (name[2] == 'S') && (name[3] == 't')) {
        UInt32 state = 0;
        batNum = name[1] - 0x30;
        if (OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(name))) {
          index = number->unsigned16BitValue();
          if (index >= MaxBatteriesSupported) {
            WarningLog("called battery # %d for St", index);
            return kIOReturnBadArgument;
          }
        }
        BatteryStatus(batNum);
        state = Battery[batNum].State;
        value = kBInitializedStatusBit;
        if (state & BatteryDischarging)
          value |= kBDischargingStatusBit;
        if (state & BatteryCritical)
          value |= kBFullyDischargedStatusBit;
        if ((state & 3) == BatteryFullyCharged)
          value |= kBFullyChargedStatusBit | kBTerminateChargeAlarmBit;
        memcpy(data, &value, 2);
        return kIOReturnSuccess;
      } else if ((name[0] == 'B') && (name[2] == 'R') && (name[3] == 'M')) {
        batNum = name[1] - 0x30;
        if (OSNumber *number = OSDynamicCast(OSNumber, sensors->getObject(name))) {
          index = number->unsigned16BitValue();
          if (index >= MaxBatteriesSupported) {
            WarningLog("called battery # %d for RM", index);
            return kIOReturnBadArgument;
          }
        }
        BatteryStatus(batNum);
        value = Battery[batNum].RemainingCapacity;
        memcpy(data, &value, 2);
        return kIOReturnSuccess;
      } else if ((name[0] == 'B') && (name[1] == 'R') &&
                 (name[2] == 'S') && (name[3] == 'C')) {
        UInt32 batrc = 0;
        value = 0;
        for (UInt8 i = 0; i < BatteryCount; i++) {
          batNum = i;
          if (BatteryConnected[batNum]) {
            BatteryStatus(batNum);
            batrc = Battery[batNum].RemainingCapacity * 100 / Battery[batNum].LastFullChargeCapacity;
            value = (value << 8) | batrc;
          }
        }
        memcpy(data, &value, 2); //assumes only two battery possible
        return kIOReturnSuccess;
      } else if ((name[0] == 'C') && (name[1] == 'H') && (name[2] == 'L') && (name[3] == 'C')) {
        UInt32 state = 0;
        value = 1;
        for (UInt8 i = 0; i < BatteryCount; i++) {
          batNum = i;
          if (BatteryConnected[batNum]) {
            BatteryStatus(batNum);
            state = Battery[batNum].State;
            if ((state & 3) == BatteryFullyCharged) {
              value = 2; //if one battery is fully charged
              break;
            }
          }
        }
      }
      else if ((name[0] == 'M') && (name[1] == 'S') &&
               (name[2] == 'L') && (name[3] == 'D')) {
        if (!LidDevice) {
          return kIOReturnBadArgument;
        }
        UInt32 acpi = 0;
        LidDevice->evaluateInteger(LidStatus, &acpi);
        value = acpi ? 0 : 1;
      } else if ((name[0] == 'B') && (name[1] == 'A') &&
                 (name[2] == 'T') && (name[3] == 'P')) {
        if (firstTime) {
          value = 1;
          firstTime = false;
        }
        else {
          value = ExternalPowerConnected ? 0 : 1;
        }
      } else if ((name[0] == 'B') && (name[1] == 'B') &&
                 (name[2] == 'I') && (name[3] == 'N')) {
        value = BatteriesConnected;
      } else if ((name[0] == 'B') && (name[1] == 'N') &&
                 (name[2] == 'u') && (name[3] == 'm')) {
        value = BatteryCount;
      } else if ((name[0] == 'A') && (name[1] == 'C') &&
                 (name[2] == '-') && (name[3] == 'N')) {
        value = AcAdapterCount;
        memcpy(data, &value, 2);
        return kIOReturnSuccess;
      } else if ((name[0] == 'A') && (name[1] == 'C') &&
                 (name[2] == '-') && (name[3] == 'W')) {
        value = 1;  //numeration from 1
      } else if ((name[0] == 'A') && (name[1] == 'C') &&
                 (name[2] == 'I') && (name[3] == 'C')) {
        value = 0x0020; //big endian will be 0x2000 = 8192mA = 96W Is it enough?
        memcpy(data, &value, 2);
        return kIOReturnSuccess;
      } else {
        return kIOReturnBadArgument;
      }
      memcpy(data, &value, 1);
      return kIOReturnSuccess;
    }
    return kIOReturnBadArgument;
  }

  return IOService::callPlatformFunction(functionName, waitForFunction, param1, param2, param3, param4);

}

#pragma mark -
#pragma mark VoodooBattery PowerSource Device
#pragma mark -
#pragma mark IOPMPowerSource

OSDefineMetaClassAndStructors(AppleSmartBattery, IOPMPowerSource)

IOReturn AppleSmartBattery::message(UInt32 type, IOService * provider, void * argument) {

  if (ParentService) {
    ParentService->message(type, provider, argument);
  }
  return kIOReturnSuccess;
}

#pragma mark Own

AppleSmartBattery * AppleSmartBattery::NewBattery(void) {
  // Create and initialize our powersource
  AppleSmartBattery * battery = new AppleSmartBattery;
  if (battery) {
    if (battery->init()) { return battery; }
    battery->release();
  }
  return 0;
}

IOReturn AppleSmartBattery::setPowerState(unsigned long which, IOService *whom)
{
  return IOPMAckImplied;
}

void AppleSmartBattery::BlankOutBattery(void) {
  setBatteryInstalled(false);
  setCycleCount(0);
  setAdapterInfo(0);
  setIsCharging(false);
  setCurrentCapacity(0);
  setMaxCapacity(0);
  setTimeRemaining(0);
  setAmperage(0);
  setVoltage(0);
  properties->removeObject(manufacturerKey);
  removeProperty(manufacturerKey);
  properties->removeObject(serialKey);
  removeProperty(serialKey);
  properties->removeObject(batteryInfoKey);
  removeProperty(batteryInfoKey);
  properties->removeObject(errorConditionKey);
  removeProperty(errorConditionKey);
  properties->removeObject(chargeStatusKey);
  removeProperty(chargeStatusKey);
  rebuildLegacyIOBatteryInfo();
}

void AppleSmartBattery::setDesignCapacity(unsigned int val) {
  OSNumber *n = OSNumber::withNumber(val, 32);
  setPSProperty(designCapacityKey, n);
  n->release();
}

void AppleSmartBattery::setDeviceName(OSSymbol * sym) {
  if (sym) {
    setPSProperty(deviceNameKey, (OSObject *) sym);
  }
}

void AppleSmartBattery::setBatteryType(OSSymbol * sym) {
  if (sym) {
    setPSProperty(batteryTypeKey, (OSObject *) sym);
  }
}

void AppleSmartBattery::setFullyCharged(bool charged) {
  setPSProperty(fullyChargedKey,
                (charged ? kOSBooleanTrue:kOSBooleanFalse));
}

void AppleSmartBattery::setInstantAmperage(int mA) {
  OSNumber *n = OSNumber::withNumber(mA, 32);
  if (n) {
    setPSProperty(instantAmperageKey, n);
    n->release();
  }
}

void AppleSmartBattery::setInstantaneousTimeToEmpty(int seconds) {
  OSNumber *n = OSNumber::withNumber(seconds, 32);
  if (n) {
    setPSProperty(instantTimeToEmptyKey, n);
    n->release();
  }
}

void AppleSmartBattery::setSerialString(OSSymbol * sym) {
  if (sym) {
    setPSProperty(softwareSerialKey, (OSObject *) sym);
  }
}

void AppleSmartBattery::rebuildLegacyIOBatteryInfo(void) {
  OSDictionary        *legacyDict = OSDictionary::withCapacity(5);
  uint32_t            flags = 0;
  OSNumber            *flags_num = NULL;

  if(externalConnected()) {
    flags |= kIOPMACInstalled;
  }

  if(batteryInstalled()) {
    flags |= kIOPMBatteryInstalled;
  }

  if(isCharging()) {
    flags |= kIOPMBatteryCharging;
  }

  flags_num = OSNumber::withNumber((unsigned long long)flags, 32);
  legacyDict->setObject(kIOBatteryFlagsKey, flags_num);
  flags_num->release();

  legacyDict->setObject(kIOBatteryCurrentChargeKey, properties->getObject(kIOPMPSCurrentCapacityKey));
  legacyDict->setObject(kIOBatteryCapacityKey, properties->getObject(kIOPMPSMaxCapacityKey));
  legacyDict->setObject(kIOBatteryVoltageKey, properties->getObject(kIOPMPSVoltageKey));
  legacyDict->setObject(kIOBatteryAmperageKey, properties->getObject(kIOPMPSAmperageKey));
  legacyDict->setObject(kIOBatteryCycleCountKey, properties->getObject(kIOPMPSCycleCountKey));

  setLegacyIOBatteryInfo(legacyDict);

  legacyDict->release();
}

