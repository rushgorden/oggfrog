// ZPCMQueue.cpp

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

#include "ZPCMQueue.h"
#include "ZDebug.h"

// I had a hard time getting this right...
#if 1
#undef OFAssert
#define OFAssert( x )
#endif

using std::exception;

ZPCMQueue::ZPCMQueue( int inBufferSizeInBytes )
	: mBufferSize( inBufferSizeInBytes / sizeof( float ) ),
		mBuffer( new float[ mBufferSize ] ),
		mHead(),
		mTail(),
		mSpaceNeeded(),
		mSpaceAvailable( 0, "ZPCMQueue Space Available Signal" ),
		mStop(),
		mFramesWritten(),
		mFramesRead(),
		mSilentFrames(),
		mWriteWaits()
{
	mHead.fValue = 0;
	mTail.fValue = 0;
	
	mSpaceNeeded.fValue = 0;
	mStop.fValue = kRun;
	
	mFramesWritten.fValue = 0;
	mFramesRead.fValue = 0;
	mSilentFrames.fValue = 0;
	mWriteWaits.fValue = 0;

	return;
}

ZPCMQueue::~ZPCMQueue()
{
	OFAssert( mTail.fValue >= 0 );
	OFAssert( mTail.fValue < mBufferSize);
	
	OFAssert( mHead.fValue >= 0 );
	OFAssert( mHead.fValue < mBufferSize );

	delete [] mBuffer;
	
	return;
}

void ZPCMQueue::Stop()
{
	ZAtomic_Set( &mStop, kTerminate );
}

void ZPCMQueue::Write( float *inPCM, int inFrames )
{			
	// We have to get the tail atomically because it can be changed by the reader running in a different
	// thread.  He'll have to get the head atomically
	
	int tail = ZAtomic_Get( &mTail );
	
	OFAssert( tail >= 0 );
	OFAssert( tail < mBufferSize);
	
	OFAssert( mHead.fValue >= 0 );
	OFAssert( mHead.fValue < mBufferSize );
	
	int available = SpaceAvailable( mHead.fValue, tail );
	
	OFAssert( available >= 0 );
	
	if ( available < inFrames ){

		// If there's not enough room, we'll wait until there is.
		
		ZAtomic_Inc( &mWriteWaits );
		
		ZAtomic_Set( &mSpaceNeeded, inFrames );
		
		ZThread::Error err;
		
		do{
			err = mSpaceAvailable.Wait( 1, 50000 );
			
			if ( err == ZThread::errorTimeout ){
			
				if ( ZAtomic_Get( &mStop ) == kTerminate )
					throw Terminated();
					
			}else if ( err == ZThread::errorDisposed ){
			
				throw Terminated();
			}
		}while ( err != ZThread::errorNone );
		
		tail = ZAtomic_Get( &mTail );
		
		available = SpaceAvailable( mHead.fValue, tail );
		
		ZAssert( available >= inFrames );
					
	}

	if ( mHead.fValue >= tail ){
	
		int endSpace = mBufferSize - mHead.fValue;
		
		OFAssert( endSpace >= 0 );

		size_t firstChunk = endSpace > inFrames ? inFrames : endSpace;
		
		OFAssert( firstChunk >= 0 );
		
		memcpy( mBuffer + mHead.fValue, inPCM, firstChunk * sizeof( float ) );
		
		ZAtomic_Add( &mFramesWritten, firstChunk );

		if ( firstChunk == inFrames ){
			
			ZAtomic_Add( &mHead, inFrames );
			
			OFAssert( mHead.fValue <= mBufferSize );
			
			if ( mHead.fValue == mBufferSize ){
				ZAtomic_Set( &mHead, 0 );
			}else{
				OFAssert( mHead.fValue > 0 );
			}

			OFAssert( mHead.fValue < mBufferSize );
		}else{
		
			// We've wrapped around, so we write the rest at the beginning
			
			int secondChunk = inFrames - firstChunk;
			
			OFAssert( secondChunk > 0 );
			
			memcpy( mBuffer, inPCM + firstChunk, secondChunk * sizeof( float ) );

			ZAtomic_Add( &mFramesWritten, secondChunk );
			
			ZAtomic_Set( &mHead, secondChunk );
			
			OFAssert( mHead.fValue > 0 );
			OFAssert( mHead.fValue < mBufferSize );
		}
	}else{
		
		// We already know we have enough room
		OFAssert( ( mHead.fValue + inFrames ) < tail );
		
		memcpy( mBuffer + mHead.fValue, inPCM, inFrames * sizeof( float ) );

		ZAtomic_Add( &mFramesWritten, inFrames );

		ZAtomic_Add( &mHead, inFrames );
		
		OFAssert( mHead.fValue <= mBufferSize );
		
		if ( mHead.fValue == mBufferSize ){
			ZAtomic_Set( &mHead, 0 );
		}else{
			OFAssert( mHead.fValue > 0 );
		}
		
		OFAssert( mHead.fValue < mBufferSize );
	}

	return;
}

void ZPCMQueue::Read( float *outPCM, int inFramesNeeded )
{
	int head = ZAtomic_Get( &mHead );
	
	OFAssert( head >= 0 );
	OFAssert( head < mBufferSize );
	OFAssert( mTail.fValue >= 0 );
	OFAssert( mTail.fValue < mBufferSize );
	
	int howFull = HowFull( head, mTail.fValue );

	int available = min( inFramesNeeded, howFull );
	
	OFAssert( ( available != 0 ) || ( head == mTail.fValue ) );
	
	OFAssert( available >= 0 );

	if ( mTail.fValue > head ){
	
		int endSpace = mBufferSize - mTail.fValue;
		
		OFAssert( endSpace >= 0 );
				
		int firstChunk = endSpace > available ? available : endSpace;
		
		memcpy( outPCM, mBuffer + mTail.fValue, firstChunk * sizeof( float ) );

		ZAtomic_Add( &mFramesRead, firstChunk );
		
		if ( firstChunk == available ){
		
			ZAtomic_Add( &mTail, available );
			
			if ( mTail.fValue == mBufferSize ){
				ZAtomic_Set( &mTail, 0 );
			}else{
				OFAssert( mTail.fValue > 0 );
			}
				
			OFAssert( mTail.fValue < mBufferSize );
		}else{
		
			int secondChunk = available - firstChunk;
			
			OFAssert( secondChunk > 0 );
			OFAssert( secondChunk < inFramesNeeded );
			
			memcpy( outPCM + firstChunk, mBuffer, secondChunk * sizeof( float ) );
			
			ZAtomic_Add( &mFramesRead, secondChunk );

			ZAtomic_Set( &mTail, secondChunk );
			
			OFAssert( mTail.fValue < mBufferSize );
			OFAssert( mTail.fValue > 0 );

		}
	}else{
	
		// We already know we have enough room
		OFAssert( ( mTail.fValue + available ) <= head );
		
		memcpy( outPCM, mBuffer + mTail.fValue, available * sizeof( float ) );
		
		ZAtomic_Add( &mFramesRead, available );

		ZAtomic_Add( &mTail, available );
		
		OFAssert( mTail.fValue < mBufferSize );
		OFAssert( mTail.fValue >= 0 );
	}	
	
	if ( available < inFramesNeeded ){
	
		int silentFrames = inFramesNeeded - available;
		
		memset( outPCM + available, 0, silentFrames * sizeof( float ) );
		
		ZAtomic_Add( &mSilentFrames, silentFrames );
	}
	
	int spaceNeeded = ZAtomic_Get( &mSpaceNeeded );
	
	ZAssert( spaceNeeded >= 0 );
	
	if ( spaceNeeded != 0 ){
	
		if ( FillLevel() < 0.5 ){
	
			available = SpaceAvailable( ZAtomic_Get( &mHead ), mTail.fValue );
			
			if ( available >= spaceNeeded ){
			
				ZAtomic_Set( &mSpaceNeeded, 0 );
				
				mSpaceAvailable.Signal( 1 );
			}
		}
	}
	
	

	
	return;
}

ZPCMQueue::Terminated::Terminated() throw()
	: exception()
{
	return;
}

ZPCMQueue::Terminated::Terminated( Terminated const &inOriginal ) throw()
	: exception( inOriginal )
{
	return;
}

ZPCMQueue::Terminated::~Terminated() throw()
{
	return;
}
			
ZPCMQueue::Terminated &ZPCMQueue::Terminated::operator=( Terminated const &inRHS ) 
	throw()
{
	exception::operator=( inRHS );

	return *this; 
}
			
const char* ZPCMQueue::Terminated::what() const throw()
{	
	return "ZPCMQueue::Terminated";
}

