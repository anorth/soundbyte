//
//  SBTAppDelegate.m
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import "SBTAppDelegate.h"
#import "SBTAudioUnit.h"

  
@implementation SBTAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  self.soundbyte = [[SBTAudioUnit alloc] init];
  [self.soundbyte start];

  return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application {
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
}

- (void)applicationWillTerminate:(UIApplication *)application {
}

@end