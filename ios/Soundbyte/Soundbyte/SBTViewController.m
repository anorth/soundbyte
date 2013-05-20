//
//  SBTViewController.m
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import "SBTViewController.h"

@interface SBTViewController ()

@end

static void initialiseAudio();

@implementation SBTViewController

- (void)viewDidLoad
{
  [super viewDidLoad];


}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
}

@end

void initialiseAudio() {
//  AVAudioSession *session = [AVAudioSession sharedInstance];
//  NSError *activationError = nil;
//  BOOL success = [session setActive:YES error:&activationError];
//  if (!success) {
//    NSLog(@"AudioSession failed to activate: %@", activationError);
//    return;
//  }
//
//  NSError *setCategoryError = nil;
//  success = [session setCategory:AVAudioSessionCategoryRecord error:&setCategoryError];
//
//  if (!success) {
//    NSLog(@"AudioSession failed to set category for recording: %@", setCategoryError);
//    [session setActive:NO error:&activationError];
//    return;
//  }
}