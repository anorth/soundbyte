//
//  SBTAppDelegate.m
//  Soundbyte
//
//  Created by Alex on 6/04/13.
//  Copyright (c) 2013 Alex. All rights reserved.
//

#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioSession.h>
#import <AVFoundation/AVAudioSession.h>
#import <AudioUnit/AudioUnit.h>

#import "CADebugMacros.h"
#import "CAStreamBasicDescription.h"
#import "CAXException.h"

#import "SBTAppDelegate.h"
#import "SBTEngine.h"
#import "SBTNativeEngine.h"
#import "SBTTetheredEngine.h"

static void onRenderError(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID,
                          AudioUnitScope inScope, AudioUnitElement inElement);
static void rioInterruptionListener(void *inClientData, UInt32 inInterruption);
static OSStatus render(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                            AudioBufferList *ioData);
static void silenceData(AudioBufferList *inData);

static id<SBTEngine> engine;

static const float SAMPLE_RATE = 44100;

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
  
@implementation SBTAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  [self configureAudio];

  engine = [[SBTNativeEngine alloc] init];
  [engine start];

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
//  XThrowIfError(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, self), "couldn't set property listener");

  // Default buffer size is 1024 frames (23 ms). Low latency allows it down to 256 (5ms) but this
  // seems to drastically affect quality, especially on device.
  // 10ms is still good for simulator.
  Float32 preferredBufferSize = .023; // Seconds
  NSLog(@"Setting hardware buffer duration %f", preferredBufferSize);
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                                        sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");

  Float64 hwSampleRate = SAMPLE_RATE;
  NSLog(@"Setting hardware sample rate %f", hwSampleRate);
  XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate,
                                        sizeof(hwSampleRate), &hwSampleRate), "Couldn't set HW sample rate");

  NSLog(@"Activating audio session");
  XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");

  UInt32 numChannelsSize;
  AudioSessionGetPropertySize(kAudioSessionProperty_CurrentHardwareInputNumberChannels, &numChannelsSize);
  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareInputNumberChannels, &numChannelsSize, &numChannels);
  NSLog(@"Num channels: %d", numChannels);

  UInt32 hardwareBufferSizeSize;
  AudioSessionGetPropertySize(kAudioSessionProperty_CurrentHardwareIOBufferDuration, &(hardwareBufferSizeSize));
  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareIOBufferDuration, &hardwareBufferSizeSize, &hardwareBufferDuration);
  NSLog(@"Hardware buffer duration %f", hardwareBufferDuration);

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

  self.unitHasBeenCreated = true;
}

- (void)startAudio {
  XThrowIfError(AudioOutputUnitStart(rioUnit), "couldn't start remote i/o unit");
}

@end

// Remote I/O render callback
OSStatus render(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                     const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
                     AudioBufferList *ioData) {
  SBTAppDelegate *THIS = (__bridge SBTAppDelegate *)inRefCon;
//  NSLog(@"render callback: unit %x, actionFlags %ld, for bus %d, frames %d, data %x", THIS->rioUnit,
//        *ioActionFlags, (unsigned int)inBusNumber, (unsigned int)inNumberFrames, ioData);
  int busToRender = kInputElement;
  OSStatus err = AudioUnitRender(THIS->rioUnit, ioActionFlags, inTimeStamp, busToRender, inNumberFrames, ioData);
  if (err) { printf("AudioUnitRender: error %d (-50 means bad parameters)\n", (int)err); return err; }

//  NSLog(@"Rendering %d frames in %d buffers, bus %d, %d channels, ts %.1f, flags %lx", (unsigned)inNumberFrames,
//        (unsigned) ioData->mNumberBuffers, (unsigned)inBusNumber,
//        (unsigned int)ioData->mBuffers[0].mNumberChannels, inTimeStamp->mSampleTime, *ioActionFlags);
  for (int i = 0; i < ioData->mNumberBuffers; ++i) {
    // FIXME avoid this copy
    int dataSize = ioData->mBuffers[i].mDataByteSize;;
    NSData *data = [NSData dataWithBytes:ioData->mBuffers[i].mData length:dataSize];
    [engine receiveAudio:data];
  }
  
  if ([engine messageAvailable]) {
    NSLog(@"Holy shit, got a message!");
//    NSData *msg = [engine takeMessage];
//    char buffer[1000];
//    int messageBytes = takeMessage(buffer, sizeof(buffer) - 1);
//    buffer[messageBytes] = '\0';
//    NSLog(@"%s", buffer);
  }

  *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
  silenceData(ioData);

  return err;
}

void rioInterruptionListener(void *inClientData, UInt32 inInterruption) {
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

void onRenderError(void *inRefCon, AudioUnit inUnit, AudioUnitPropertyID inID,
                   AudioUnitScope inScope, AudioUnitElement inElement) {
  SBTAppDelegate *THIS = (__bridge SBTAppDelegate *)inRefCon;
  OSStatus renderError;
  UInt32 size = sizeof(renderError);
  AudioUnitGetProperty(THIS->rioUnit, kAudioUnitProperty_LastRenderError, inScope, inElement, &renderError, &size);
  NSLog(@"Render error: %ld", renderError);
}

void silenceData(AudioBufferList *inData) {
  for (UInt32 i=0; i < inData->mNumberBuffers; i++)
    memset(inData->mBuffers[i].mData, 0, inData->mBuffers[i].mDataByteSize);
}


