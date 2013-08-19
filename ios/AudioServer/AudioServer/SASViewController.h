//
//  SASViewController.h
//  AudioServer
//
//  Created by Alex on 31/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SASAudioSocket.h"

@interface SASViewController : UIViewController <SASAudioSocketDelegate>

- (void)setIpAddress:(NSString *)ipAddr port:(NSString *)port;

@end
