// ZPCMSink_CoreAudio.cpp

/* ------------------------------------------------------------
Copyright (c) 2006 Michael David Crawford.
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

#ifndef __ZPCMSINK_COREAUDIO__
#define __ZPCMSINK_COREAUDIO__ 1


#include <memory>
#include <vector>

#include <ZThreadSimple.h>

#if ZCONFIG( OS, Carbon ) || ZCONFIG( OS, MacOSX )

#include <AudioUnit/AudioUnitProperties.h>

#include "OggFrogConfig.h"
#include "ZPCMSink.h"
#include "ZFIFOVector.h"

class ZPCMQueue;
 
class ZPCMSink_CoreAudio: public ZPCMSink
{
	public:
	
		ZPCMSink_CoreAudio( FIFOVector &inFIFOs );
		virtual ~ZPCMSink_CoreAudio();
		
		void Start();
		void Stop();
		
		virtual bool Stopped() const;

	private:
	
		AudioUnit mOutput;
		bool		mStarted;
		
		int OpenOutput( AudioUnit &output );
		int CloseOutput( AudioUnit output );

		static OSStatus PCMData( void *inRefCon,
									AudioUnitRenderActionFlags *	ioActionFlags,
									const AudioTimeStamp *			inTimeStamp,
									UInt32							inBusNumber,
									UInt32							inNumberFrames,
									AudioBufferList *				ioData);
									
};

#endif // ZCONFIG( OS, Carbon )

#endif // __ZPCMSINK_COREAUDIO__
