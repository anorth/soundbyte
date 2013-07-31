//
//  SBTNativeEngine.m
//  Soundbyte
//
//  Created by Alex on 18/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import "SBTNativeEngine.h"

#import <Scom/scom.h>
#import <Scom/log.h>


@interface SBTNativeEngine()

@property BOOL started;

@end


@implementation SBTNativeEngine

- (void)start {
  NSAssert(!self.started, @"Already started!");
  NSLog(@"NativeEngine starting");
  setPriority(LOG_INFO);
  scomInit(18000, 50, 2, 8);
  NSLog(@"Scom initialised");
  self.started = YES;
}

- (void)stop {
  NSAssert(!self.started, @"Engine not started!");
  NSLog(@"NativeEngine stopping");
  self.started = NO;
}

- (NSData *)encodeMessage:(NSData *)payload {
  return nil;
}

- (void)receiveAudio:(NSData *)audio {
  decodeAudio((char*)[audio bytes], [audio length]);
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
