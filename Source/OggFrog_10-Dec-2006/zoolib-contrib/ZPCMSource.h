// ZPCMSource.h

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

#ifndef __ZPCMSource__
#define __ZPCMSource__ 1

#include <memory>

#include "ZRefCount.h"
#include "ZFile.h"
#include "ZMessage.h"

#include "OggFrogConfig.h"

class ZPCMQueue;

class ZPCMSource: public ZRefCounted
{
	public:
		ZPCMSource( ZFileSpec const &inFile, ZMessenger inClient );
		virtual ~ZPCMSource();
		
		static ZRef< ZPCMSource > Create( ZFileSpec const &inFile, ZMessenger inClient );
		
		virtual void Play();
		virtual void Pause();
		virtual void Stop();
		
		virtual void SetTime( float newTime );
		
		virtual bool Playing();
		
		virtual float TotalTime() const;
			
		virtual float CurTime() const;

		static inline float Clip( float inSample );
		
	protected:

		typedef std::vector< ZPCMQueue* > OutputFIFOVector;

		OutputFIFOVector			mOutputs;
		ZRef< ZStreamerRPos >		mRawStream;
		ZMessenger					mClient;
		ZSemaphore					mPlayLock;
		ZAtomic_t					mStop;
		float						mTotalTime;

		enum{
			kRun,
			kPause,
			kStop,
			kTerminate,
			kDone
		};

		class DecoderDone: public std::exception
		{
			public:
				DecoderDone() throw();
				~DecoderDone() throw();
		};
		
		class DecoderPaused: public std::exception
		{
			public:
				DecoderPaused() throw();
				~DecoderPaused() throw();
		};
		
		class DecoderTerminated: public std::exception
		{
			public:
				DecoderTerminated() throw();
				~DecoderTerminated() throw();
		};

		void SendFinishedMessage();
		void Terminate();

	private:
	
		ZRef< ZStreamerRPos > OpenInput( ZFileSpec const &inFile );

		
};

inline float ZPCMSource::Clip( float inSample )
{
	if ( inSample > 1.0 )
		return 1.0;
		
	if ( inSample < -1.0 )
		return -1.0;
		
	return inSample;
}


#endif