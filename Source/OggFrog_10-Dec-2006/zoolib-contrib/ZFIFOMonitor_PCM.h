// ZFIFOMonitor_PCM.h

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

#ifndef __ZFIFOMONITOR_PCM__
#define __ZFIFOMONITOR_PCM__

#include <vector>
#include <string>

#include "ZMessage.h"
#include "ZRefCount.h"

#include "OFApp.h"

class ZPCMQueue;

class ZFIFOMonitor_PCM: public ZRefCounted
{
	public:
	
		ZFIFOMonitor_PCM( FIFOVector &inFIFOs );
		ZFIFOMonitor_PCM();
		~ZFIFOMonitor_PCM();
		
		ZMutexBase &GetLock(){ return mLock; }
		
		void Add( ZMessenger inMessenger, int inFIFOIndex );
		void Remove( ZMessenger inMessenger, int inFIFOIndex );

	private:

		struct MonitorSpec{
			MonitorSpec( ZMessenger inClient, int inFIFOIndex, ZPCMQueue *inFIFO );
			MonitorSpec( MonitorSpec const &inOrig );
			MonitorSpec();
			
			MonitorSpec const &operator=( MonitorSpec const &inRHS );
			
			ZMessenger	mClient;
			int			mFIFOIndex;
			ZPCMQueue	*mFIFO;
		};
		
		enum{
			kRun,
			kTerminate,
			kDone
		};
		
		typedef std::vector< MonitorSpec > MonitorVec;
		
		ZMutex		mLock;
		MonitorVec	mSpecs;
		size_t		mSpecSize;		// For debugging
		ZAtomic_t	mStop;
		
		static void Monitor( ZFIFOMonitor_PCM &me );
		void Monitor();
		
};
		
#endif // __ZFIFOMONITOR_PCM__
