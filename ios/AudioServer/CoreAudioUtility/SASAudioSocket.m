//
//  SASAudioSocket.m
//  AudioServer
//
//  Created by Alex on 1/08/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import "SASAudioSocket.h"

#import <CoreFoundation/CoreFoundation.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <ifaddrs.h>

static void socketCallback (CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef address,
                            const void *data, void *info);

static const int PORT = 16002;

@interface SASAudioSocket() <NSStreamDelegate> {
@public
  NSInputStream *inputStream;
  NSOutputStream *outputStream;
@private
  BOOL inputStreamHasData;
  BOOL outputStreamHasSpace;
}


@end

@implementation SASAudioSocket

- (id)init {
  self = [super init];
  if (self) {
    inputStreamHasData = NO;
    outputStreamHasSpace = NO;
  }
  return self;
}

- (void)startListening {
  // Create socket
  CFSocketContext ctx = {
    .version = 0,
    .info = (__bridge void *)(self),
    .retain = NULL,
    .release = NULL,
    .copyDescription = NULL
  };
  CFSocketRef sockRef = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP,
                                            kCFSocketAcceptCallBack, socketCallback, &ctx);

  // Bind
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_len = sizeof(sin);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);
  sin.sin_addr.s_addr= INADDR_ANY;

  CFDataRef sinData = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&sin, sizeof(sin));
  CFSocketSetAddress(sockRef, sinData);
  CFRelease(sinData);
  NSLog(@"Socket listening on %@ %d", [self getIPAddress], PORT);

  // Add to run loop
  CFRunLoopSourceRef socketSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, sockRef, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), socketSource, kCFRunLoopDefaultMode);
}

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len {
  if (inputStreamHasData) {
    NSUInteger bytesRead = [inputStream read:buffer maxLength:len];
    if (bytesRead < len) {
      inputStreamHasData = NO;
    }
    return bytesRead;
  }
  return 0;
}

- (NSInteger)write:(const uint8_t *)buffer maxLength:(NSUInteger)len {
  if (outputStreamHasSpace) {
    NSUInteger bytesWritten = [outputStream write:buffer maxLength:len];
    if (bytesWritten < len) {
      outputStreamHasSpace = NO;
    }
  }
  return 0;
}

- (void)stream:(NSStream *)theStream handleEvent:(NSStreamEvent)streamEvent {
  if (streamEvent == NSStreamEventOpenCompleted) {
    NSLog(@"Stream %@ opened", theStream);
  } else if (streamEvent == NSStreamEventHasBytesAvailable) {
//    NSLog(@"Input stream has data");
    inputStreamHasData = YES;
  } else if (streamEvent == NSStreamEventHasSpaceAvailable) {
//    NSLog(@"Output stream has space");
    outputStreamHasSpace = YES;
  } else if (streamEvent == NSStreamEventEndEncountered) {
    NSLog(@"Stream %@ ended", theStream);
  } else if (streamEvent == NSStreamEventErrorOccurred) {
    NSLog(@"Stream %@ erred", theStream);
  } else {
    NSLog(@"Stream event %x on stream %@", streamEvent, theStream);
  }
}

- (NSString *)getIPAddress {
  NSString *address = @"error";
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  int success = 0;
  // retrieve the current interfaces - returns 0 on success
  success = getifaddrs(&interfaces);
  if (success == 0) {
    // Loop through linked list of interfaces
    temp_addr = interfaces;
    while(temp_addr != NULL) {
      if(temp_addr->ifa_addr->sa_family == AF_INET) {
        // Check if interface is en0 which is the wifi connection on the iPhone
        if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"]) {
          // Get NSString from C String
          address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
        }
      }
      temp_addr = temp_addr->ifa_next;
    }
  }
  // Free memory
  freeifaddrs(interfaces);
  return address;
}

@end

void socketCallback (CFSocketRef sock, CFSocketCallBackType callbackType, CFDataRef address,
                     const void *data, void *info) {
  if (callbackType == kCFSocketAcceptCallBack) {
    NSLog(@"Socket accepted");
    SASAudioSocket *THIS = (__bridge SASAudioSocket *)(info);
    const CFSocketNativeHandle *handle = (CFSocketNativeHandle *)data;

    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;
    CFStreamCreatePairWithSocket(NULL, *handle, &readStream, &writeStream);

    THIS->inputStream = (__bridge NSInputStream *)readStream;
    THIS->inputStream.delegate = THIS;
    [THIS->inputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [THIS->inputStream open];
    
    THIS->outputStream = (__bridge NSOutputStream *)writeStream;
    THIS->outputStream.delegate = THIS;
    [THIS->outputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [THIS->outputStream open];
  } else {
    NSLog(@"Unexpected callback type %ld", callbackType);
  }
}