//
//  SASAppDelegate.m
//  AudioServer
//
//  Created by Alex on 31/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import "SASAppDelegate.h"

#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioSession.h>
#import <AVFoundation/AVAudioSession.h>
#import <AudioUnit/AudioUnit.h>

#import "CADebugMacros.h"
#import "CAStreamBasicDescription.h"
#import "CAXException.h"

#import "SASAudioSocket.h"
#import "SASViewController.h"


static Float64 SAMPLE_RATE = 44100;

static void onRenderError(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID,
                          AudioUnitScope inScope, AudioUnitElement inElement);
static void rioInterruptionListener(void *inClientData, UInt32 inInterruption);
static OSStatus render(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                       AudioBufferList *ioData);
static void silenceData(AudioBufferList *inData);

static AudioComponentDescription RIO_UNIT_DESC = {
  .componentType = kAudioUnitType_Output,
  .componentSubType = kAudioUnitSubType_RemoteIO,
  .componentManufacturer = kAudioUnitManufacturer_Apple,
  .componentFlags = 0,
  .componentFlagsMask = 0
};

// (LPCM non-interleaved 16 bit int) on the inward-facing scopes of
// the input/output busses (from aurioTouch).
static AudioStreamBasicDescription STREAM_DESC_1 = {
  .mSampleRate = SAMPLE_RATE,
  .mFormatID = kAudioFormatLinearPCM,
  .mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked |
  kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsNonInterleaved,
  .mBytesPerPacket = sizeof(AudioSampleType),
  .mFramesPerPacket = 1,
  .mBytesPerFrame = sizeof(AudioSampleType),
  .mChannelsPerFrame = 1,
  .mBitsPerChannel = 8 * sizeof(AudioSampleType),
  .mReserved = 0,
};


enum : AudioUnitElement {
  kOutputElement = 0,
  kInputElement = 1
};

@interface SASAppDelegate() {
@public
  AudioUnit rioUnit;
  SASAudioSocket *audioSocket;
}

@end;

@implementation SASAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  self.viewController = [[SASViewController alloc] initWithNibName:@"SASViewController" bundle:nil];
  self.window.rootViewController = self.viewController;
  [self.window makeKeyAndVisible];

  [self openSocket];

  [self configureAudio];
  [self startAudio];

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

- (void)openSocket {
  audioSocket = [[SASAudioSocket alloc] init];
  [audioSocket startListening];
}

- (void)configureAudio {
  NSLog(@"ConfigureAudio");

  AURenderCallbackStruct inputProc;
  inputProc.inputProc = render;
  inputProc.inputProcRefCon = (__bridge void *)self;

  // Initialize and configure the audio session
  NSLog(@"Initialising audio session");
  XThrowIfError(AudioSessionInitialize(NULL, NULL, rioInterruptionListener, (__bridge void *)self), "couldn't initialize audio session");

  UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category");

  // Default buffer size is 1024 frames (23 ms). Low latency allows it down to 256 (5ms) but this
  // seems to drastically affect quality, especially on device.
  // 10ms is still good for simulator.
//  Float32 preferredBufferSize = .023; // Seconds
//  NSLog(@"Setting hardware buffer duration %f", preferredBufferSize);
//  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
//                                        sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");

  Float64 hwSampleRate = SAMPLE_RATE;
  NSLog(@"Setting hardware sample rate %f", hwSampleRate);
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate,
                                        sizeof(hwSampleRate), &hwSampleRate), "Couldn't set HW sample rate");

  NSLog(@"Activating audio session");
  XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");

  // Open the input/output unit
  NSLog(@"Creating RemoteIO unit");

  AudioComponent comp = AudioComponentFindNext(NULL, &RIO_UNIT_DESC);
  XThrowIfError(AudioComponentInstanceNew(comp, &rioUnit), "Couldn't open remote I/O");

  // Enable input
  UInt32 one = 1;
  XThrowIfError(AudioUnitSetProperty(rioUnit, kAudioOutputUnitProperty_EnableIO,
                                     kAudioUnitScope_Input, kInputElement, &one, sizeof(one)),
                "couldn't enable input on the remote I/O unit");

  XThrowIfError(AudioUnitSetProperty(rioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, kOutputElement, &inputProc,
                                     sizeof(inputProc)),
                "couldn't set remote i/o render callback");

  // Set our required format on inward sides of both busses.
  NSLog(@"Configuring RemoteIO");
  CAStreamBasicDescription streamFormat = CAStreamBasicDescription(STREAM_DESC_1);

  XThrowIfError(AudioUnitSetProperty(rioUnit, kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Input, kOutputElement, &streamFormat, sizeof(streamFormat)),
                "couldn't set the remote I/O unit's output client format");
  XThrowIfError(AudioUnitSetProperty(rioUnit, kAudioUnitProperty_StreamFormat,
                                     kAudioUnitScope_Output, kInputElement, &streamFormat, sizeof(streamFormat)),
                "couldn't set the remote I/O unit's input client format");

  XThrowIfError(AudioUnitAddPropertyListener(rioUnit, kAudioUnitProperty_LastRenderError, onRenderError, (__bridge void *)self),
                "Couldn't set error listener");


  NSLog(@"Initialising RemoteIO");
  XThrowIfError(AudioUnitInitialize(rioUnit), "Couldn't initialize the remote I/O unit");

  NSLog(@"RemoteIO initialised");
}

- (void)startAudio {
  XThrowIfError(AudioOutputUnitStart(rioUnit), "couldn't start remote i/o unit");
}

@end

// Remote I/O render callback
OSStatus render(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                AudioBufferList *ioData) {
  SASAppDelegate *THIS = (__bridge SASAppDelegate *)inRefCon;
  //  NSLog(@"render callback: unit %x, actionFlags %ld, for bus %d, frames %d, data %x", THIS->rioUnit,
  //        *ioActionFlags, (unsigned int)inBusNumber, (unsigned int)inNumberFrames, ioData);
  OSStatus err = AudioUnitRender(THIS->rioUnit, ioActionFlags, inTimeStamp, kInputElement, inNumberFrames, ioData);
  if (err) {
    NSLog(@"AudioUnitRender: error %d (-50 means bad parameters)\n", (int)err);
    return err;
  }

//  NSLog(@"Rendering %d frames in %d buffers, bus %d, %d channels, ts %.1f, flags %lx", (unsigned)inNumberFrames,
//        (unsigned) ioData->mNumberBuffers, (unsigned)inBusNumber,
//        (unsigned int)ioData->mBuffers[0].mNumberChannels, inTimeStamp->mSampleTime, *ioActionFlags);
  for (int i = 0; i < ioData->mNumberBuffers; ++i) {
    AudioBuffer &buf = ioData->mBuffers[i];
    // Send recorded audio
    [THIS->audioSocket write:(const uint8_t *)buf.mData maxLength:buf.mDataByteSize];

    // Play received audio
    memset(buf.mData, 0, buf.mDataByteSize);
    NSUInteger bytesPlayed = [THIS->audioSocket read:(uint8_t *)buf.mData maxLength:buf.mDataByteSize];
    if (bytesPlayed == 0) {
      *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
    }
  }

  return err;
}

void rioInterruptionListener(void *inClientData, UInt32 inInterruption) {
  try {
    NSLog(@"Session interrupted! --- %s ---",
          (inInterruption == kAudioSessionBeginInterruption) ? "Begin Interruption" : "End Interruption");

    SASAppDelegate *THIS = (__bridge SASAppDelegate *)inClientData;

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

void onRenderError(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID,
                   AudioUnitScope inScope, AudioUnitElement inElement) {
  SASAppDelegate *THIS = (__bridge SASAppDelegate *)inRefCon;
  OSStatus renderError;
  UInt32 size = sizeof(renderError);
  AudioUnitGetProperty(THIS->rioUnit, kAudioUnitProperty_LastRenderError, inScope, inElement, &renderError, &size);
  NSLog(@"Render error: %ld", renderError);
}

void silenceData(AudioBufferList *inData) {
  for (UInt32 i=0; i < inData->mNumberBuffers; i++)
    memset(inData->mBuffers[i].mData, 0, inData->mBuffers[i].mDataByteSize);
}
