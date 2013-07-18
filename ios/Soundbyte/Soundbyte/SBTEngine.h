//
//  SBTEngine.h
//  Soundbyte
//
//  Created by Alex on 18/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol SBTEngine <NSObject>

- (void)start;

- (void)stop;

- (NSData *)encodeMessage:(NSData *)payload;

- (void)receiveAudio:(NSData *)audio;

- (BOOL)messageAvailable;

- (NSInteger)messageProgress;

- (NSData *)takeMessage;

@end
