//
//  SASAudioSocket.h
//  AudioServer
//
//  Created by Alex on 1/08/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol SASAudioSocketDelegate <NSObject>

- (void)statusChanged:(NSString *)status;

@end

@interface SASAudioSocket : NSObject {
@public
  __weak id<SASAudioSocketDelegate> delegate;

}
- (void)startListening;

- (NSString *)getIPAddress;
- (NSString *)getPort;

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len;

- (NSInteger)write:(const uint8_t *)buffer maxLength:(NSUInteger)len;

@end
