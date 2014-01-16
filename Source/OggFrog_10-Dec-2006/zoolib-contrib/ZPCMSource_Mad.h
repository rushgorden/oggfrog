// ZPCMSource_Mad.h

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

#ifndef __ZPCMSource_Mad__
#define __ZPCMSource_Mad__ 1

#include <vector>

#include "ZPCMSource.h"

#include "mad.h"

class ZPCMSource_Mad: public ZPCMSource
{
	public:
		ZPCMSource_Mad( ZFileSpec const &inFile, ZMessenger inClient );
		virtual ~ZPCMSource_Mad();

		virtual void SetTime( float newTime );
				
		virtual float TotalTime() const;
			
		virtual float CurTime() const;
		
	private:
	
		struct TimeOffset {
			TimeOffset( size_t inOffset, mad_timer_t const &inTime );
			TimeOffset( TimeOffset const &inOrig );
			
			size_t	mOffset;
			mad_timer_t	mTime;
		};
		
		typedef std::vector< TimeOffset > TimeVec;
		
		enum
		{
			kInputBufferSize = 8 * 8192,
			kBufferGuardSize = 8,
			kOutputBufferSize = 8192
		};
				
		ZStreamRPos	const	&mReader;
		mad_stream			mStream;
		mad_frame			mFrame;
		mad_synth			mSynth;
		mad_timer_t			mTimer;
		unsigned char		mInputBuffer[ kInputBufferSize ];
		float				mOutputBuffer[ 2 ][ kOutputBufferSize ];
		const float			*mOutputBufferEnd;
		unsigned char		*mGuardPtr;
		unsigned long		mFrameCount;
		TimeVec				mTimeVec;
		mad_timer_t			mOffsetInterval;
		ZAtomic_t			mTotalTime;
		ZAtomic_t			mCurTime;
		ZAtomic_t			mNewTime;

		static void DecoderProc( ZPCMSource_Mad &inPlayer );
		void DecoderProc();

		float MadFixedToFloat( mad_fixed_t fixed );
		
		void SkipTags();
		void RecordOffset( size_t offset, mad_timer_t const &inTime );
		void EstimateTotalTime();
		float TimerToFloat( mad_timer_t const &inTimer );
		void FloatToTimer( mad_timer_t &outTimer, float inTime );
		void SetTime( mad_timer_t const &inNewTime );
		void Read();
};

#endif