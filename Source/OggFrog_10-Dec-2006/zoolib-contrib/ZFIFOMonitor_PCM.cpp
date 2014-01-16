// ZFIFOMonitor_PCM.cpp

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

#include "ZFIFOMonitor_PCM.h"
#include "ZThreadSimple.h"
#include "ZPCMQueue.h"

ZFIFOMonitor_PCM::ZFIFOMonitor_PCM( FIFOVector &inFIFOs )
	: mLock( "FIFO Monitor", true ),
		mSpecs(),
		mSpecSize( mSpecs.size() ),
		mStop()
{
	ZAssert( mSpecSize == 0 );
	
	int i = 0;
	for( FIFOVector::iterator it( inFIFOs.begin() );
			it != inFIFOs.end();
			++it ){
		mSpecs.push_back( MonitorSpec( ZMessenger(), i++, it->GetObject() ) );
		++mSpecSize;
	}

	mStop.fValue = kTerminate;

	ZThreadSimple< ZFIFOMonitor_PCM& > *thread = new ZThreadSimple< ZFIFOMonitor_PCM& >( Monitor, *this, "FIFO Monitor" );
	
	ZAtomic_Set( &mStop, kRun );
	
	ZAssert( mSpecs.size() == mSpecSize );

	mLock.Release();

	thread->Start();

	return;
}

#if 0
ZFIFOMonitor_PCM::ZFIFOMonitor_PCM()
	: mLock( "FIFO Monitor", true ),
		mSpecs(),
		mSpecSize( mSpecs.size() ),
		mStop()
{
	ZAssert( mSpecSize == 0 );

	mStop.fValue = kTerminate;

#if 0	
	ZThreadSimple< ZFIFOMonitor_PCM& > *thread = new ZThreadSimple< ZFIFOMonitor_PCM& >( Monitor, *this, "FIFO Monitor" );
	
	ZAtomic_Set( &mStop, kRun );
	
	ZAssert( mSpecs.size() == mSpecSize );
	
	mLock.Release();

	thread->Start();
#else
	mLock.Release();
#endif
	return;
}
#endif

ZFIFOMonitor_PCM::~ZFIFOMonitor_PCM()
{
	if ( ZAtomic_Swap( &mStop, kTerminate ) != kDone ){
	
		while ( kDone != ZAtomic_Get( &mStop ) )
			ZThread::sSleep( 10 );
	}
	
	return;
}

void ZFIFOMonitor_PCM::Add( ZMessenger inMessenger, int inFIFOIndex )
{	
	ZAssert( mLock.IsLocked() );

	ZAssert( inFIFOIndex >= 0 && inFIFOIndex < mSpecs.size() );

	mSpecs[ inFIFOIndex ].mClient = inMessenger;

	return;
}

void ZFIFOMonitor_PCM::Remove( ZMessenger inMessenger, int inFIFOIndex )
{
	ZAssert( mLock.IsLocked() );

	ZAssert( mSpecSize == mSpecs.size() );
	
	for ( MonitorVec::iterator it( mSpecs.begin() ); it != mSpecs.end(); ++it ){
	
		if ( ( it->mClient == inMessenger ) && ( it->mFIFOIndex == inFIFOIndex ) ){
		
			mSpecs.erase( it );
			
			--mSpecSize;

			ZAssert( mSpecSize == mSpecs.size() );
						
			break; 
		}
	
	}

	// We only allow one entry per combination
	
	for ( MonitorVec::iterator it( mSpecs.begin() ); it != mSpecs.end(); ++it ){
	
		ZAssert( !( ( it->mClient == inMessenger ) && ( it->mFIFOIndex == inFIFOIndex ) ) );
	}
	
	return;
}

void ZFIFOMonitor_PCM::Monitor( ZFIFOMonitor_PCM &me )
{
#if ZCONFIG_Debug > 0
	{
		ZLocker locker( me.mLock );
		ZAssert( me.mSpecSize == me.mSpecs.size() );
	}
#endif

	me.Monitor();
	
	return;
}

void ZFIFOMonitor_PCM::Monitor()
{	
	do{
	
		int item = 0;
		
		do{
		
			if ( kTerminate == ZAtomic_Get( &mStop ) ){
			
				ZAtomic_Set( &mStop, kDone );
				
				return;
			}
		
			ZLocker locker( GetLock() );
			
			ZAssert( mSpecSize == mSpecs.size() );
			ZAssert( mSpecSize < 64 );
			
			if ( item >= mSpecs.size() )
				break;

			ZPCMQueue &fifo = *mSpecs[ item ].mFIFO;
			
			ZMessage msg;

			msg.SetString( "what", "OggFrog:ZPCMQueue:report" );
			
			msg.SetPointer( "FIFO", &( fifo ) );
			
			msg.SetInt32( "channel-index", item );
			msg.SetInt32( "howFull", fifo.HowFull() );
			msg.SetInt32( "capacity", fifo.Capacity() );
			msg.SetInt32( "framesWritten", fifo.FramesWritten() );
			msg.SetInt32( "framesRead", fifo.FramesRead() );
			msg.SetInt32( "silentFrames", fifo.SilentFrames() );
			msg.SetInt32( "writeWaits", fifo.WriteWaits() );
			
			ZAssert( mSpecSize == mSpecs.size() );
			ZAssert( item < mSpecSize );
		
			mSpecs[ item ].mClient.PostMessage(msg);

			++item;
					
		}while( true );
		
		ZThread::sSleep( 100 );

	}while( true );
}

ZFIFOMonitor_PCM::MonitorSpec::MonitorSpec( ZMessenger inClient, int inFIFOIndex, ZPCMQueue *inFIFO )
	: mClient( inClient ),
		mFIFOIndex( inFIFOIndex ),
		mFIFO( inFIFO )
{
	return;
}

ZFIFOMonitor_PCM::MonitorSpec::MonitorSpec( MonitorSpec const &inOrig )
	: mClient( inOrig.mClient ),
		mFIFOIndex( inOrig.mFIFOIndex ),
		mFIFO( inOrig.mFIFO )
{
	return;
}


ZFIFOMonitor_PCM::MonitorSpec::MonitorSpec()
	: mClient(),
		mFIFOIndex( 0 ),
		mFIFO( NULL )
{
	return;
}
			
ZFIFOMonitor_PCM::MonitorSpec const &ZFIFOMonitor_PCM::MonitorSpec::operator=( MonitorSpec const &inRHS )
{
	mClient = inRHS.mClient;
	mFIFOIndex = inRHS.mFIFOIndex;
	mFIFO = inRHS.mFIFO;
	
	return *this;
}
