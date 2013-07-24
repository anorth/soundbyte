//
//  SBTAppDelegate.m
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioSession.h>
#import <AudioUnit/AudioUnit.h>

#import "CADebugMacros.h"
#import "CAStreamBasicDescription.h"
#import "CAXException.h"

#import "SBTAppDelegate.h"
#import "SBTEngine.h"
#import "SBTNativeEngine.h"
#import "SBTTetheredEngine.h"

static void rioInterruptionListener(void *inClientData, UInt32 inInterruption);
static int setupRemoteIO(AudioUnit& inRemoteIOUnit, AURenderCallbackStruct inRenderProc,
                         AudioStreamBasicDescription& outFormat);
static OSStatus performThru(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                            AudioBufferList *ioData);
static void silenceData(AudioBufferList *inData);

static id<SBTEngine> engine;

static const float SAMPLE_RATE = 44100;

@implementation SBTAppDelegate

@synthesize rioUnit;
@synthesize inputProc;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

  inputProc.inputProc = performThru;
  inputProc.inputProcRefCon = (__bridge void *)self;
  
  // Initialize and configure the audio session
  NSLog(@"Initialising audio session");
  XThrowIfError(AudioSessionInitialize(NULL, NULL, rioInterruptionListener, (__bridge void *)self), "couldn't initialize audio session");

  UInt32 audioCategory = kAudioSessionCategory_RecordAudio; // RecordAudio?
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category");
//  XThrowIfError(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, self), "couldn't set property listener");

  Float32 preferredBufferSize = .005; // Seconds
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                                        sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");

  Float64 hwSampleRate = SAMPLE_RATE;
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate,
                                        sizeof(hwSampleRate), &hwSampleRate), "Couldn't set HW sample rate");

  XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");

  CAStreamBasicDescription streamFormat;
  OSStatus rioStatus = setupRemoteIO(rioUnit, inputProc, streamFormat);
  if (rioStatus != kAudioSessionNoError) {
    NSLog(@"Couldn't set up remote IO: %ld", rioStatus);
  }
  self.unitHasBeenCreated = true;
  NSLog(@"Audio session initialised");

  XThrowIfError(AudioOutputUnitStart(rioUnit), "couldn't start remote i/o unit");

//  engine = [[SBTTetheredEngine alloc] init];
  engine = [[SBTNativeEngine alloc] init];
  [engine start];

  return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
  // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
  // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
  // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end


int setupRemoteIO(AudioUnit& inRemoteIOUnit, AURenderCallbackStruct inRenderProc, AudioStreamBasicDescription& outFormat)
{
  // Open the input/output unit
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_RemoteIO;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext(NULL, &desc);
  XThrowIfError(AudioComponentInstanceNew(comp, &inRemoteIOUnit), "Couldn't open remote I/O");

  // Enable input
  UInt32 one = 1;
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO,
                                              kAudioUnitScope_Input, 1, &one, sizeof(one)),
                "couldn't enable input on the remote I/O unit");

  // Set callback
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_SetRenderCallback,
                                                 kAudioUnitScope_Input, 0, &inRenderProc,
                                                 sizeof(inRenderProc)),
                "couldn't set remote i/o render callback");

  // Set our required format (LPCM non-interleaved 16 bit int) on the inward-facing scopes of
  // the input/output busses.
  UInt32 inFormatID = kAudioFormatLinearPCM;
  UInt32 bytesPerPacket = 2;
  UInt32 framesPerPacket = 1;
  UInt32 bytesPerFrame = 2;
  UInt32 channelsPerFrame = 1; // 2?
  UInt32 bitsPerChannel = 16;
  UInt32 inFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked |
      kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsNonInterleaved;

  outFormat = CAStreamBasicDescription(SAMPLE_RATE, inFormatID, bytesPerPacket, framesPerPacket,
                                       bytesPerFrame, channelsPerFrame, bitsPerChannel, inFormatFlags);

  int outputBus = 0;
  int inputBus = 1;
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat,
                                               kAudioUnitScope_Input, outputBus, &outFormat, sizeof(outFormat)),
                "couldn't set the remote I/O unit's output client format");
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output, inputBus, &outFormat, sizeof(outFormat)),
                "couldn't set the remote I/O unit's input client format");

  XThrowIfError(AudioUnitInitialize(inRemoteIOUnit), "Couldn't initialize the remote I/O unit");
  
  NSLog(@"RemoteIO initialised");
  return 0;
}

// Remote I/O render callback
OSStatus performThru(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                     const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                     AudioBufferList *ioData) {
  SBTAppDelegate *THIS = (__bridge SBTAppDelegate *)inRefCon;
  OSStatus err = AudioUnitRender(THIS->rioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
  if (err) { printf("PerformThru: error %d\n", (int)err); return err; }

//  NSLog(@"Received %d frames in %d buffers, bus %d", (unsigned)inNumberFrames,
//        (unsigned) ioData->mNumberBuffers, (unsigned)inBusNumber);
  for (int i = 0; i < ioData->mNumberBuffers; ++i) {
    // FIXME avoid this copy
    NSData *data = [NSData dataWithBytes:ioData->mBuffers[i].mData length:ioData->mBuffers[i].mDataByteSize];
    [engine receiveAudio:data];
  }
  
  if ([engine messageAvailable]) {
    NSLog(@"Holy shit, got a message!");
    NSData *msg = [engine takeMessage];
//    char buffer[1000];
//    int messageBytes = takeMessage(buffer, sizeof(buffer) - 1);
//    buffer[messageBytes] = '\0';
//    NSLog(@"%s", buffer);
  }

  silenceData(ioData);

  return err;
}

void rioInterruptionListener(void *inClientData, UInt32 inInterruption)
{
  try {
    NSLog(@"Session interrupted! --- %s ---",
          (inInterruption == kAudioSessionBeginInterruption) ? "Begin Interruption" : "End Interruption");

    SBTAppDelegate *THIS = (__bridge SBTAppDelegate *)inClientData;

    if (inInterruption == kAudioSessionEndInterruption) {
      XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active");
      XThrowIfError(AudioOutputUnitStart(THIS->rioUnit), "couldn't start unit");
    }

    if (inInterruption == kAudioSessionBeginInterruption) {
      XThrowIfError(AudioOutputUnitStop(THIS->rioUnit), "couldn't stop unit");
    }
  } catch (CAXException e) {
    char buf[256];
    fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
  }
}

void silenceData(AudioBufferList *inData) {
  for (UInt32 i=0; i < inData->mNumberBuffers; i++)
    memset(inData->mBuffers[i].mData, 0, inData->mBuffers[i].mDataByteSize);
}


