// ZPCMSink_CoreAudio.cpp

/* ------------------------------------------------------------
Copyright (c) 2006 Michael D. Crawford.
http://www.oggfrog.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------ */

/* This is presently hardwired for 44.1 khz stereo linear PCM, encoded as 32-bit floating point, which
 * should be the default sound output on recent versions of Mac OS X; however, if you set your sound output
 * differently, the audio will be corrupted.  For production use, we'll need to inquire about the output format
 * and sample rate, and convert if necessary.
 */

#include "ZPCMSink_CoreAudio.h"
#include <new>
#include <cstring>
#include <AudioToolbox/AudioToolbox.h>
#include "ZDebug.h"
#include "OFApp.h"

using std::bad_alloc;
using std::vector;

ZPCMSink_CoreAudio::ZPCMSink_CoreAudio( FIFOVector &inFIFOs )
	: ZPCMSink( inFIFOs ),
		mStarted( false )
{		
	if ( OpenOutput( mOutput ) != 0 ){
		throw bad_alloc();
	}

	return;
}

ZPCMSink_CoreAudio::~ZPCMSink_CoreAudio()
{
	Stop();
	
	if ( CloseOutput( mOutput ) != 0 ){
		ZAssert( false );
	}
	
	return;
}

void ZPCMSink_CoreAudio::Start()
{	
	AURenderCallbackStruct callbackData;
	callbackData.inputProc = PCMData;
	callbackData.inputProcRefCon = this;
	
	OSStatus result;

	if ( ( result = AudioUnitSetProperty( mOutput,
								kAudioUnitProperty_SetRenderCallback,
								kAudioUnitScope_Global,
								0,
								&callbackData,
								sizeof( callbackData ) ) ) != noErr ){
		return;
	}

	ComponentResult cResult;

	if ( noErr != ( cResult = AudioOutputUnitStart( mOutput ) ) ){
		throw bad_alloc();
	}

	mStarted = true;
	
	return;
}

void ZPCMSink_CoreAudio::Stop()
{
	ComponentResult cResult;

	if ( noErr != ( cResult = AudioOutputUnitStop( mOutput ) ) ){
		ZAssert( false );
	}

	mStarted = false;
	
	return;
}

bool ZPCMSink_CoreAudio::Stopped() const
{
	return !mStarted;
}

int ZPCMSink_CoreAudio::OpenOutput( AudioUnit &output )
{
	ComponentDescription cd;
	
	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;
	
	Component comp = FindNextComponent( NULL, &cd );
	if ( comp == NULL )
		return -1;
	
	OSStatus result;
	
	result = OpenAComponent( comp, &output );
	if ( noErr != result )
		return -1;
		
	result = AudioUnitInitialize( output );
	if ( noErr != result )
		return -1;

	AudioStreamBasicDescription streamDesc;
	
	UInt32 streamDescSize = sizeof( streamDesc );
	
	if ( noErr != ( result = AudioUnitGetProperty( output,
													kAudioUnitProperty_StreamFormat,
													kAudioUnitScope_Global,
													0,
													&streamDesc,
													&streamDescSize ) ) ){
		return -1;
	}
													
	return 0;
}		

int ZPCMSink_CoreAudio::CloseOutput( AudioUnit output )
{
	ComponentResult cResult;
	OSErr err;
	
	if ( noErr != ( cResult = AudioUnitUninitialize( output ) ) ){
		return -1;
	}
	
	if ( noErr != ( err = CloseComponent( output ) ) ){
		return -1;
	}

	return 0;
}

OSStatus ZPCMSink_CoreAudio::PCMData( void *inRefCon,
										AudioUnitRenderActionFlags *	ioActionFlags,
										const AudioTimeStamp *			inTimeStamp,
										UInt32							inBusNumber,
										UInt32							inNumberFrames,
										AudioBufferList *				ioData)
{
	ZPCMSink_CoreAudio &me = *static_cast< ZPCMSink_CoreAudio* >( inRefCon );

	InputFIFOVector &inputs = me.GetInputs();
	
	inputs[ 0 ]->Read( static_cast< float* >( ioData->mBuffers[ 0 ].mData ), inNumberFrames );
	inputs[ 1 ]->Read( static_cast< float* >( ioData->mBuffers[ 1 ].mData ), inNumberFrames );
	
	return noErr;
}

	