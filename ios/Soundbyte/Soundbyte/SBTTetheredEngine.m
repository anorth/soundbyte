//
//  SBTTetheredEngine.m
//  Soundbyte
//
//  Created by Alex on 18/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import "SBTTetheredEngine.h"

@interface SBTTetheredEngine()

@property BOOL started;

@end


@implementation SBTTetheredEngine 

- (void)start {
  NSAssert(!self.started, @"Already started!");
  NSLog(@"TetheredEngine starting");
  self.started = YES;
}

- (void)stop {
  NSAssert(!self.started, @"Engine not started!");
  NSLog(@"TetheredEngine stopping");
  self.started = NO;
}

- (NSData *)encodeMessage:(NSData *)payload {
  return nil;
}

- (void)receiveAudio:(NSData *)audio {

}

- (BOOL)messageAvailable {
  return NO;
}

- (NSInteger)messageProgress {
  return 0;
}

- (NSData *)takeMessage {
  return nil;
}


@end
