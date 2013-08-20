//
//  SBTAudioUnit.h
//  Soundbyte
//
//  Created by Alex on 19/08/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SBTAudioUnit : NSObject

- (void)start;

- (BOOL)messageAvailable;

- (NSData *)takeMessage;

- (void)sendMessage:(NSData *)msgData;

@end
