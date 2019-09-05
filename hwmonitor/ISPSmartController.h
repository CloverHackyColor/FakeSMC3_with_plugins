//
//  ISPSmartController.h
//  iStatPro
//
//  Created by Buffy on 11/06/07.
//  Copyright 2007 . All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <sys/param.h>
#include <sys/mount.h>


#define kATADefaultSectorSize 512
#define kWindowSMARTsDriveTempAttribute     194
#define kWindowSMARTsDriveTempAttribute2    190
#define kSMARTsDriveWearLevelingCount       177
#define kSMARTAttributeCount 30

@interface ISPSmartController : NSObject {
  NSMutableArray *diskData;
  NSMutableArray *latestData;
  NSArray *temps;
  NSArray *disksStatus;
  NSMutableDictionary *partitionData;
  NSNumber *temp;
  NSNumber *life;
}
- (void)getPartitions;
- (void)update;
- (NSDictionary *)getDataSet /*:(int)degrees*/;
- (NSDictionary *)getSSDLife;
@end
