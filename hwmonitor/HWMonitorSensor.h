//
//  NSSensor.h
//  HWSensors
//
//  Created by mozo,Navi on 22.10.11.
//  Copyright (c) 2011 mozo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ISPSmartController.h"

enum {
  TemperatureSensorGroup  =   1,
  VoltageSensorGroup      =   2,
  TachometerSensorGroup   =   3,
  FrequencySensorGroup    =   4,
  MultiplierSensorGroup   =   5,
  HDSmartTempSensorGroup  =   6,
  BatterySensorsGroup     =   7,
  HDSmartLifeSensorGroup  =   8,
  
};
typedef NSUInteger SensorGroup;

@interface HWMonitorSensor : NSObject {
  NSString *    key;
  NSString *    type;
  SensorGroup   group;
  NSString *    caption;
  id            object;
  BOOL          favorite;
  
  // instance vars for the below @property
  NSString *    _key;
  NSString *    _type;
  SensorGroup   _group;
  NSString *    _caption;
  id            _object;
  BOOL          _favorite;
}

@property (readwrite, retain) NSString *    key;
@property (readwrite, retain) NSString *    type;
@property (readwrite, assign) SensorGroup   group;
@property (readwrite, retain) NSString *    caption;
@property (readwrite, retain) id            object;
@property (readwrite, assign) BOOL          favorite;



+ (unsigned int) swapBytes:(unsigned int) value;

+ (NSData *)readValueForKey:(NSString *)key;
+ (NSString* )getTypeOfKey:(NSString*)key;

- (HWMonitorSensor *)initWithKey:(NSString *)aKey
                         andType: aType
                        andGroup:(NSUInteger)aGroup
                     withCaption:(NSString *)aCaption;

- (NSString *)formatedValue:(NSData *)value;

@end

