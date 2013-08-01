//
//  SASAudioSocket.h
//  AudioServer
//
//  Created by Alex on 1/08/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SASAudioSocket : NSObject

- (void)startListening;

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len;

- (NSInteger)write:(const uint8_t *)buffer maxLength:(NSUInteger)len;

@end
