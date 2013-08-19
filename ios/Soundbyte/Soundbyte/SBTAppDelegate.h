//
//  SBTAppDelegate.h
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import <AudioUnit/AudioUnit.h>
#import <UIKit/UIKit.h>

#import "SBTAudioUnit.h"

@interface SBTAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) SBTAudioUnit *soundbyte;

@end
