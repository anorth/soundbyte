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

#import "SBTConstants.h"

@interface SBTNativeEngine() {
  BOOL started;
  SInt32 progress;
}

@end

static int RECV_MESSAGE_BUFFER_SIZE = 1000;
static int SEND_BUFFER_DURATION_SECS = 10;

@implementation SBTNativeEngine

- (void)start {
  NSAssert(!started, @"Already started!");
  NSLog(@"NativeEngine starting");
  setPriority(LOG_INFO);
  scomInit(18000, 50, 2, 8);
  NSLog(@"Scom initialised");
  started = YES;
}

- (void)stop {
  [self checkStarted];
  NSLog(@"NativeEngine stopping");
  started = NO;
}

- (NSData *)encodeMessage:(NSData *)payload {
  [self checkStarted];
  int nbytes = SBT_SAMPLE_RATE * SBT_BYTES_PER_SAMPLE * SEND_BUFFER_DURATION_SECS;
  char *buf = (char *)malloc(nbytes);
  int bytesUsed = encodeMessage((char *)[payload bytes], [payload length], buf, nbytes);
  NSAssert(bytesUsed > 0, @"Message payload too long");
  return [NSData dataWithBytesNoCopy:buf length:bytesUsed];
}

- (void)receiveAudio:(NSData *)audio {
  [self checkStarted];
  progress = decodeAudio((char*)[audio bytes], [audio length]);
}

- (BOOL)messageAvailable {
  [self checkStarted];
  return messageAvailable();
}

- (NSInteger)messageProgress {
  [self checkStarted];
  return progress;
}

- (NSData *)takeMessage {
  [self checkStarted];
  NSAssert(messageAvailable(), @"No message available");
  void *buffer = malloc(RECV_MESSAGE_BUFFER_SIZE);
  int messageBytes = takeMessage((char *)buffer, RECV_MESSAGE_BUFFER_SIZE - 1);
  return [NSData dataWithBytesNoCopy:buffer length:messageBytes];
}

- (void)checkStarted {
  NSAssert(started, @"Engine not started");
}

@end
