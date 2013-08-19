//
//  SBTAppDelegate.h
//  Soundbyte Listener
//
//  Created by Alex on 19/08/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <SoundbyteSDK/Soundbyte.h>


@class SBTViewController;

@interface SBTAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) SBTViewController *viewController;
@property (strong, nonatomic) SBTAudioUnit *soundbyte;

@end
