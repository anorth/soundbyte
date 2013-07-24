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
  NSLog(@"Setting hardware buffer duration %f", preferredBufferSize);
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                                        sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");

  Float64 hwSampleRate = SAMPLE_RATE;
  NSLog(@"Setting hardware sample rate %f", hwSampleRate);
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate,
                                        sizeof(hwSampleRate), &hwSampleRate), "Couldn't set HW sample rate");

  NSLog(@"Activating audio session");
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
							
- (void)applicationWillResignActive:(UIApplication *)application {
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
}

- (void)applicationWillTerminate:(UIApplication *)application {
}

@end


int setupRemoteIO(AudioUnit& inRemoteIOUnit, AURenderCallbackStruct inRenderProc, AudioStreamBasicDescription& outFormat)
{
  // Open the input/output unit
  NSLog(@"Creating RemoteIO unit");
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_RemoteIO;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext(NULL, &desc);
  XThrowIfError(AudioComponentInstanceNew(comp, &inRemoteIOUnit), "Couldn't open remote I/O");

  enum : AudioUnitElement {
    kOutputElement = 0,
    kInputElement = 1
  };

  // Enable input
  UInt32 one = 1;
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO,
                                              kAudioUnitScope_Input, kInputElement, &one, sizeof(one)),
                "couldn't enable input on the remote I/O unit");

  // Disable output (necessary with kAudioSessionCategory_RecordAudio)
//  UInt32 zero = 0;
//  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioOutputUnitProperty_EnableIO,
//                                     kAudioUnitScope_Output, kOutputElement, &zero, sizeof(zero)),
//                "Error disabling output for remote I/O unit");

  // Set callback
  // kAudioOutputUnitProperty_SetInputCallback, kAudioUnitProperty_SetRenderCallback
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_SetRenderCallback,                                                 kAudioUnitScope_Input, kOutputElement, &inRenderProc,
                                                 sizeof(inRenderProc)),
                "couldn't set remote i/o render callback");

  // Set our required format (LPCM non-interleaved 16 bit int) on the inward-facing scopes of
  // the input/output busses.
  NSLog(@"Configuring RemoteIO");
  UInt32 inFormatID = kAudioFormatLinearPCM;
  UInt32 bytesPerPacket = sizeof(AudioSampleType);
  UInt32 framesPerPacket = 1;
  UInt32 bytesPerFrame = sizeof(AudioSampleType);
  UInt32 channelsPerFrame = 1; // 2?
  UInt32 bitsPerChannel = 8 * sizeof(AudioSampleType);
  UInt32 inFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked |
      kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsNonInterleaved;

  outFormat = CAStreamBasicDescription(SAMPLE_RATE, inFormatID, bytesPerPacket, framesPerPacket,
                                       bytesPerFrame, channelsPerFrame, bitsPerChannel, inFormatFlags);

  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat,
                                               kAudioUnitScope_Input, kOutputElement, &outFormat, sizeof(outFormat)),
                "couldn't set the remote I/O unit's output client format");
  XThrowIfError(AudioUnitSetProperty(inRemoteIOUnit, kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output, kInputElement, &outFormat, sizeof(outFormat)),
                "couldn't set the remote I/O unit's input client format");

  NSLog(@"Initialising RemoteIO");
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
  if (err) { printf("AudioUnitRender: error %d\n", (int)err); return err; }

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


