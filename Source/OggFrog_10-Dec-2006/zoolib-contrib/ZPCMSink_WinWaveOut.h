// ZPCMSink_WinWaveOut.h

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

#ifndef __ZPCMSINK_WINWAVEOUT__
#define __ZPCMSINK_WINWAVEOUT__ 1

#include <vector>

#include "ZThread.h"

#include "ZPCMSink.h"

#if ZCONFIG( OS, Win32 )

#include "ZWinHeader.h"
 
class ZPCMSink_WinWaveOut: public ZPCMSink
{
	public:
	
		ZPCMSink_WinWaveOut( FIFOVector &inFIFOs );

		virtual ~ZPCMSink_WinWaveOut();
		
		virtual void Start();
		virtual void Stop();
		
		virtual bool Stopped() const;
		
	private:
	
		enum{
			kMaxBuffers = 8,
			kBufferSize = 1024
		};
		struct Buffer{
			WAVEHDR	mHeader;
			short	mSamples[ kBufferSize * 2 ];
		};
		
		
		typedef std::vector< Buffer > BufferVector;
		
		float			mSineWavePhase;		// for testing
		float			*mRawPCM;
		ZSemaphore		mStartSignal;
		ZAtomic_t		mStop;
		ZAtomic_t		mAvailableBuffers;
		ZAtomic_t		mTerminate;
		ZSemaphore		mBufferAvailable;
		HWAVEOUT		mDevice;
		BufferVector	mBuffers;
		BufferVector::iterator	mNextBuffer;
		
		static void CALLBACK Callback( HWAVEOUT device,      
  										UINT message,         
  										DWORD_PTR refCon,  
 										DWORD msgParam1,    
  										DWORD msgParam2 );
  										
  		static void OutputThread( ZPCMSink_WinWaveOut &me );
  		void OutputThread();
  		
  		void FreeBuffer();
		void WaitForInput();
  											
};

#endif // ZCONFIG( OS, Win32 )

#endif // __ZPCMSINK_WINWAVEOUT__
