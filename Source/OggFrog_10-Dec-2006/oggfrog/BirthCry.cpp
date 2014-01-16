// BirthCry.cpp - The very first sound Ogg Frog made!

/* Ogg Frog: Free Music Ripping, Encoding and Backup Program
 * http://www.oggfrog.com
 *
 * Copyright (C) 2006 Michael David Crawford. 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <iostream>
#include <math.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnitProperties.h>

int OpenOutput( AudioUnit &output );
int CloseOutput( AudioUnit output );

OSStatus PCMData( void *							inRefCon,
						AudioUnitRenderActionFlags *	ioActionFlags,
						const AudioTimeStamp *			inTimeStamp,
						UInt32							inBusNumber,
						UInt32							inNumberFrames,
						AudioBufferList *				ioData);

using std::cerr;
using std::cout;

int main( int argc, char **argv )
{	
	AudioUnit output;
	
	if ( OpenOutput( output ) != 0 ){
		cerr << "OpenOutput failed\n";
		return -1;
	}
	
	ComponentResult cResult;
	
	if ( noErr != ( cResult = AudioOutputUnitStart( output ) ) ){
		cerr << "AudioOutputUnitStart failed\n";
		return -1;
	}
	
	usleep( 10 * 1000 * 1000 );
	
	if ( noErr != ( cResult = AudioOutputUnitStop( output ) ) ){
		cerr << "AudioOutputUnitStop failed\n";
		return -1;
	}
	
	if ( CloseOutput( output ) != 0 ){
		cerr << "CloseOutput failed\n";
		return -1;
	}
	
	return 0;
}

int OpenOutput( AudioUnit &output )
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
		
	AURenderCallbackStruct callbackData;
	callbackData.inputProc = PCMData;
	callbackData.inputProcRefCon = NULL;
	
	if ( ( result = AudioUnitSetProperty( output,
								kAudioUnitProperty_SetRenderCallback,
								kAudioUnitScope_Global,
								0,
								&callbackData,
								sizeof( callbackData ) ) ) != noErr ){
		return -1;
	}
	
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

int CloseOutput( AudioUnit output )
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

OSStatus PCMData( void *inRefCon,
						AudioUnitRenderActionFlags *	ioActionFlags,
						const AudioTimeStamp *			inTimeStamp,
						UInt32							inBusNumber,
						UInt32							inNumberFrames,
						AudioBufferList *				ioData)
{
	Float64 now = inTimeStamp->mSampleTime;
	
	Float64 twoPi = 2 * 3.1415926535;
		
	Float32 *sample = (Float32*)( ioData->mBuffers[ 0 ].mData );
	
	for ( int i = 0; i < inNumberFrames; ++i ){
	
		*sample++ = sin( twoPi * ( now / 100.0 ) );
		
		now += 1.0;
	}

	return noErr;
}

	