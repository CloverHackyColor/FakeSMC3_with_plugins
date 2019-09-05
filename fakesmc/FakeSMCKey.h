/*
 *  FakeSMCKey.h
 *  FakeSMC
 *
 *  Created by mozo on 03/10/10.
 *  Copyright 2010 mozodojo. All rights reserved.
 *
 */

#ifndef _FAKESMCKEY_H
#define _FAKESMCKEY_H

#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSSymbol.h>
#include <libkern/c++/OSNumber.h>
#include <libkern/c++/OSData.h>
#include <IOKit/IOService.h>

class FakeSMCKey : public OSObject {
  OSDeclareDefaultStructors(FakeSMCKey)

protected:
	static void		copySymbol(const char *from, char* to);

  char *        name;
  char *        type;
	unsigned char	size;
	void *        value;
	IOService *		handler;

public:
	static FakeSMCKey *withValue(const char *aName,
                               const char *aType,
                               unsigned char aSize,
                               const void *aValue);
  
	static FakeSMCKey *withHandler(const char *aName,
                                 const char *aType,
                                 unsigned char aSize,
                                 IOService *aHandler);

	// Not for general use. Use withCallback or withValue instance creation method
	virtual bool init(const char * aName,
                    const char * aType,
                    unsigned char aSize,
                    const void *aValue,
                    IOService *aHandler = 0);

	virtual void  free();

	const char    *getName();
	const char    *getType();
	unsigned char getSize();
	const void    *getValue();

	bool          setValueFromBuffer(const void *aBuffer, unsigned char aSize);
	bool          setHandler(IOService *aHandler);
  IOService     *getHandler();

	bool          isEqualTo(const char *aKey);
	bool          isEqualTo(FakeSMCKey *aKey);
	bool          isEqualTo(const OSMetaClassBase *anObject);
};


#endif
