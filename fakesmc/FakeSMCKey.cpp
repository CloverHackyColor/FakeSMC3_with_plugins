/*
 *  FakeSMCKey.cpp
 *  FakeSMC
 *
 *  Created by mozo on 03/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#include "FakeSMC.h"

#include <IOKit/IOLib.h>
#include <libkern/c++/OSSerialize.h>

#ifndef Debug
#define Debug FALSE
#endif

#define LogPrefix "FakeSMCKey: "
#define DebugLog(string, args...)	do { if (Debug) { IOLog (LogPrefix "[Debug] " string "\n", ## args); } } while(0)
#define WarningLog(string, args...) do { IOLog (LogPrefix "[Warning] " string "\n", ## args); } while(0)
#define InfoLog(string, args...)	do { IOLog (LogPrefix string "\n", ## args); } while(0)

#define super OSObject
OSDefineMetaClassAndStructors(FakeSMCKey, OSObject)

IOService *FakeSMCKey::getHandler() {
  return handler;
};

void FakeSMCKey::copySymbol(const char *from, char* to) {
	bzero(to, 5);
	size_t len = strlen(from);
	bcopy(from, to, len > 4 ? 4 : len);
}

FakeSMCKey *FakeSMCKey::withValue(const char *aName,
                                  const char *aType,
                                  unsigned char aSize,
                                  const void *aValue) {
  FakeSMCKey *me = new FakeSMCKey;
	
  if (me && !me->init(aName, aType, aSize, aValue)) {
    me->release();
    return 0;
  }
	
  return me;
}

FakeSMCKey *FakeSMCKey::withHandler(const char *aName,
                                    const char *aType,
                                    unsigned char aSize,
                                    IOService *aHandler) {
  FakeSMCKey *me = new FakeSMCKey;
	
  if (me && !me->init(aName, aType, aSize, 0, aHandler)) {
    me->release();
    return 0;
  }
	
  return me;
}

bool FakeSMCKey::init(const char * aName,
                      const char * aType,
                      unsigned char aSize,
                      const void *aValue,
                      IOService * aHandler) {
  if (!super::init()) {
    return false;
  }
  
  if (!aName || strlen(aName) == 0 || !(name = (char *)IOMalloc(5))) {
		return false;
  }
  
	copySymbol(aName, name);
	size = aSize;
	
  if (!(type = (char *)IOMalloc(5))) {
		return false;
  }
  
  if (!aType || strlen(aType) == 0) {
    switch (size) {
      case 1:
        copySymbol("ui8", type);
        break;
      case 2:
        copySymbol("ui16", type);
        break;
      case 4:
        copySymbol("ui32", type);
        break;
      default:
        copySymbol("ch8*", type);
        break;
    }
  } else {
    copySymbol(aType, type);
  }
	
  if (size == 0) {
		size++;
  }
  
  if (!(value = IOMalloc(size))) {
		return false;
  }
  
  if (aValue) {
		bcopy(aValue, value, size);
  } else {
		bzero(value, size);
  }
  
	handler = aHandler;
  return true;
}

void FakeSMCKey::free() {
  if (name) {
		IOFree(name, 5);
  }
  
  if (type) {
		IOFree(type, 5);
  }
  
  if (value) {
		IOFree(value, size);
  }
	super::free();
}

const char *FakeSMCKey::getName() {
  return name;
};

const char *FakeSMCKey::getType() {
  return type;
};

unsigned char FakeSMCKey::getSize() {
  return size;
};

const void *FakeSMCKey::getValue() {
  if (handler) {
    IOReturn result = handler->callPlatformFunction(kFakeSMCGetValueCallback,
                                                    false, (void *)name,
                                                    (void *)value,
                                                    (void *)(long long)size,
                                                    0);
    
    if (kIOReturnSuccess != result) {
      WarningLog("value update request callback error for key %s, return 0x%x", name, result);
    }
  }
  
  return value;
};

bool FakeSMCKey::setValueFromBuffer(const void *aBuffer, unsigned char aSize) {
  if (!aBuffer || aSize == 0) {
		return false;
  }
  
	if (aSize != size) {
    if (value) {
			IOFree(value, size);
    }
    
		size = aSize;
		
    if (!(value = IOMalloc(size))) {
			return false;
    }
	}
	
	bcopy(aBuffer, value, size);
	
	if (handler) {
		IOReturn result = handler->callPlatformFunction(kFakeSMCSetValueCallback,
                                                    false,
                                                    (void *)name,
                                                    (void *)value,
                                                    (void *)(long long)size,
                                                    0);
		
    if (kIOReturnSuccess != result) {
			WarningLog("value changed event callback error for key %s, return 0x%x", name, result);
    }
	}
	
	return true;
}

bool FakeSMCKey::setHandler(IOService *aHandler) {
  if (!aHandler) {
		return false;
  }
  
	handler = aHandler;
	return true;
}

/*
 void FakeSMCKey::removeHandler() {
 handler = NULL;
 return;
 }
*/

bool FakeSMCKey::isEqualTo(const char *aKey) {
	return strncmp(name, aKey, 4) == 0;
}

bool FakeSMCKey::isEqualTo(FakeSMCKey *aKey) {
	return (aKey && aKey->isEqualTo(name));
}

bool FakeSMCKey::isEqualTo(const OSMetaClassBase *anObject) {
  if (FakeSMCKey *aKey = OSDynamicCast(FakeSMCKey, anObject)) {
    return isEqualTo(aKey);
  }
  return false;
}
