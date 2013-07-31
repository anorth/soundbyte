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
  NSAssert(!started, @"Engine not started!");
  NSLog(@"NativeEngine stopping");
  started = NO;
}

- (NSData *)encodeMessage:(NSData *)payload {
  int nbytes = SAMPLE_RATE * BYTES_PER_SAMPLE * SEND_BUFFER_DURATION_SECS;
  char *buf = (char *)malloc(nbytes);
  int bytesUsed = encodeMessage((char *)[payload bytes], [payload length], buf, nbytes);
  NSAssert(bytesUsed > 0, @"Message payload too long");
  return [NSData dataWithBytesNoCopy:buf length:bytesUsed];
}

- (void)receiveAudio:(NSData *)audio {
  progress = decodeAudio((char*)[audio bytes], [audio length]);
}

- (BOOL)messageAvailable {
  return messageAvailable();
}

- (NSInteger)messageProgress {
  return progress;
}

- (NSData *)takeMessage {
  void *buffer = malloc(RECV_MESSAGE_BUFFER_SIZE);
  int messageBytes = takeMessage((char *)buffer, RECV_MESSAGE_BUFFER_SIZE - 1);
  return [NSData dataWithBytesNoCopy:buffer length:messageBytes];
}

@end
