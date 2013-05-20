//
//  SBTAppDelegate.h
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import <AudioUnit/AudioUnit.h>
#import <UIKit/UIKit.h>

@interface SBTAppDelegate : UIResponder <UIApplicationDelegate> {
@public
  AudioUnit rioUnit;
  AURenderCallbackStruct inputProc;
}

@property (strong, nonatomic) UIWindow *window;

@property (nonatomic, assign) AudioUnit rioUnit;
@property (nonatomic, assign) BOOL unitIsRunning;
@property (nonatomic, assign) BOOL unitHasBeenCreated;
@property (nonatomic, assign) AURenderCallbackStruct inputProc;

@end
