//
//  NSSensor.m
//  HWSensors
//
//  Created by mozo on 22.10.11.
//  Copyright (c) 2011 mozo. All rights reserved.
//

#import "HWMonitorSensor.h"

#include "../utils/definitions.h"
#include "smc.h"

//#define SMC_ACCESS
#define BIT(x) (1 << (x))
#define bit_get(x, y) ((x) & (y))
#define bit_clear(x, y) ((x) &= (~y))


int getIndexOfHexChar(char);
float decodeNumericValue(NSData*, NSString*);

int getIndexOfHexChar(char c) {
  return c > 96 && c < 103 ? c - 87 : c > 47 && c < 58 ? c - 48 : 0;
}

float decodeNumericValue(NSData* _data, NSString*_type)
{
  if (_type && _data && [_type length] >= 3) {
    if (([_type characterAtIndex:0] == 'u' ||
         [_type characterAtIndex:0] == 's')
        && [_type characterAtIndex:1] == 'i') {
      BOOL signd = [_type characterAtIndex:0] == 's';
      
      switch ([_type characterAtIndex:2]) {
        case '8':
          if ([_data length] == 1) {
            UInt8 encoded = 0;
            
            bcopy([_data bytes], &encoded, 1);
            
            if (signd && bit_get(encoded, BIT(7))) {
              bit_clear(encoded, BIT(7));
              return -encoded;
            }
            
            return encoded;
          }
          break;
          
        case '1':
          if ([_type characterAtIndex:3] == '6' && [_data length] == 2) {
            UInt16 encoded = 0;
            
            bcopy([_data bytes], &encoded, 2);
            
            encoded = OSSwapBigToHostInt16(encoded);
            
            if (signd && bit_get(encoded, BIT(15))) {
              bit_clear(encoded, BIT(15));
              return -encoded;
            }
            
            return encoded;
          }
          break;
          
        case '3':
          if ([_type characterAtIndex:3] == '2' && [_data length] == 4) {
            UInt32 encoded = 0;
            
            bcopy([_data bytes], &encoded, 4);
            
            encoded = OSSwapBigToHostInt32(encoded);
            
            if (signd && bit_get(encoded, BIT(31))) {
              bit_clear(encoded, BIT(31));
              return -encoded;
            }
            
            return encoded;
          }
          break;
      }
    }
    else if (([_type characterAtIndex:0] == 'f' ||
              [_type characterAtIndex:0] == 's') &&
             [_type characterAtIndex:1] == 'p' && [_data length] == 2) {
      UInt16 encoded = 0;
      
      bcopy([_data bytes], &encoded, 2);
      
      UInt8 i = getIndexOfHexChar([_type characterAtIndex:2]);
      UInt8 f = getIndexOfHexChar([_type characterAtIndex:3]);
      
      if ((i + f) != (([_type characterAtIndex:0] == 's') ? 15 : 16) )
        return 0;
      
      UInt16 swapped = OSSwapBigToHostInt16(encoded);
      
      BOOL signd = [_type characterAtIndex:0] == 's';
      BOOL minus = !!(bit_get(swapped, BIT(15)));
      
      if (signd && minus) bit_clear(swapped, BIT(15));
      
      return ((float)swapped / (float)BIT(f)) * (signd && minus ? -1 : 1);
    }
  }
  
  return 0;
}

@implementation HWMonitorSensor

@synthesize key;
@synthesize type;
@synthesize group;
@synthesize caption;
@synthesize object;
@synthesize favorite;

+ (unsigned int)swapBytes:(unsigned int)value {
  return ((value & 0xff00) >> 8) | ((value & 0xff) << 8);
}


+ (NSData *) readValueForKey:(NSString *)key {
  SMCOpen(&conn);
  
  UInt32Char_t  readkey = "\0";
  strncpy(readkey,
          [key cStringUsingEncoding:NSASCIIStringEncoding]==NULL
          ? ""
          : [key cStringUsingEncoding:NSASCIIStringEncoding],4);
  
  readkey[4]=0;
  SMCVal_t      val;
  
  kern_return_t result = SMCReadKey(readkey, &val);
  if (result != kIOReturnSuccess) {
    return NULL;
  }
  SMCClose(conn);
  if (val.dataSize > 0) {
    return [NSData dataWithBytes:val.bytes length:val.dataSize];
  }
  return nil;
  
}

+ (NSString *) getTypeOfKey:(NSString *)key {
  SMCOpen(&conn);
  
  UInt32Char_t  readkey = "\0";
  strncpy(readkey,[key cStringUsingEncoding:NSASCIIStringEncoding]==NULL
          ? ""
          : [key cStringUsingEncoding:NSASCIIStringEncoding],4);
  
  readkey[4]=0;
  SMCVal_t      val;
  
  kern_return_t result = SMCReadKey(readkey, &val);
  if (result != kIOReturnSuccess)
    return NULL;
  SMCClose(conn);
  if (val.dataSize > 0)
    return [NSString stringWithFormat:@"%.4s", val.dataType];
  return nil;
}


- (HWMonitorSensor *)initWithKey:(NSString *)aKey
                         andType: aType
                        andGroup:(NSUInteger)aGroup
                     withCaption:(NSString *)aCaption {
  self.type = aType;
  self.key = aKey;
  self.group = aGroup;
  self.caption = aCaption;
  
  return self;
}

- (NSString *) formatedValue:(NSData *)value {
  if (value != nil) {
    float v = decodeNumericValue(value, type);
    switch (self.group) {
      case TemperatureSensorGroup:
        return [[NSString alloc] initWithFormat:@"%2d°",(int)v];
        
      case HDSmartTempSensorGroup: {
        unsigned int t = 0;
        bcopy([value bytes], &t, 2);
        //t = [NSSensor swapBytes:t] >> 8;
        return [[NSString alloc] initWithFormat:@"%d°",t];
      }
        
      case BatterySensorsGroup: {
        NSInteger * t;
        t = (NSInteger*)[value bytes];
        return [[NSString alloc] initWithFormat:@"%ld",*t];
      }
        
      case HDSmartLifeSensorGroup: {
        NSInteger * l;
        l = (NSInteger*)[value bytes];
        return [[NSString alloc] initWithFormat:@"%ld%%",*l];
      }
        
      case VoltageSensorGroup:
        return [[NSString alloc] initWithFormat:@"%2.3fV", v];
        
      case TachometerSensorGroup:
        return [[NSString alloc] initWithFormat:@"%drpm",(int)v];
        
      case FrequencySensorGroup: {
        unsigned int MHZ = 0;
        bcopy([value bytes], &MHZ, 2);
        MHZ = [HWMonitorSensor swapBytes:MHZ];
        return [[NSString alloc] initWithFormat:@"%dMHz",MHZ];
      }
        
      case MultiplierSensorGroup: {
        unsigned int mlt = 0;
        bcopy([value bytes], &mlt, 2);
        return [[NSString alloc] initWithFormat:@"x%1.1f",(float)mlt / 10.0];
      } break;
    }
  }
  
  return @"-";
}

@end

