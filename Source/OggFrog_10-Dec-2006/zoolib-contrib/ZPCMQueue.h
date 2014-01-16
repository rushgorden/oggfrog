// ZPCMQueue.h

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

#ifndef __ZPCMQUEUE__
#define __ZPCMQUEUE__ 1

/* First-In First-Out buffer for Pulse Code Modulated audio.
 *
 * Internally it is a two-ended circular buffer.  The writer waits on a
 * semaphore when it's full.  The reader never blocks - it's not allowed to
 * on Mac OS X.  Instead it gets silence when the FIFO is empty.
 *
 * We can't use a Standard Template Library deque because audio pathways on
 * Classic Mac OS have to be run from interrupt tasks, which aren't allowed
 * to allocate memory.  Thus the buffer has a fixed size and does no allocation
 * during use.
 *
 * An important boundary condition is that mHead == mTail means the FIFO
 * is empty.  Thus we have to keep a minimum of one slot empty to distingush
 * being full from being empty.
 *
 * The source is littered with asserts because I had a hard time getting it
 * right.  The performance still seems to be OK even if they're all enabled.
 *
 * For now, I use a separate PCMQueue for each channel.  It's also hardwired
 * for floating point PCM.  In the next build I'll have a single multichannel 
 * PCMQueue that also harmonizes the input format with the output.  For 
 * example, libvorbis produces floating point samples, but the Windows 
 * waveOut device wants 16 or 24-bit integer.  With libmad MP3 and OS X CoreAudio,
 * it's just the opposite - integer input and floating point output.  Also,
 * CoreAudio wants the channels in different buffers, while waveOut wants them
 * interleaved.
 *
 * This code doesn't look thread-safe, but I think it is.  Because the output
 * can't wait on locks, there's no thread-safe way to atomically manipulate both
 * mHead and mTail at the same time.  We calculate how much room their is by 
 * fetching their values in a non-atomic way in several places.  The value 
 * calculated won't, in general, be accurate, but we can guarantee that 
 * there is always at least as much room as calculated; there might actually 
 * be more.
 *
 * This works because only the input can change mHead, and only the output can
 * change mTail.
 */

#include <exception>
#include "ZThread.h"
#include "ZRefCount.h"
#include "OggFrogConfig.h"

class ZPCMQueue: public ZRefCounted
{
	public:
	
		ZPCMQueue( int inBufferSizeInBytes = 0x10000 );
		virtual ~ZPCMQueue();
		
		// always succeeds; it will wait on a semaphore if there
		// isn't enough room.  If we're still waiting when the FIFO
		// is deleted or stopped, it will throw ZPCMQueue::Terminate.
			
		void Write( float *inPCM, int inFrames );
		
		void Read( float *outPCM, int inFramesNeeded );
		
		void Stop();
		
		int Capacity() const { return mBufferSize - 1; }	// Result returned in frames
		inline int HowFull() const;
		inline float FillLevel() const 
			{ return static_cast< float >( HowFull() ) / static_cast< float >( Capacity() ); }
		
		int FramesWritten() const { return ZAtomic_Get( &mFramesWritten ); };
		int FramesRead() const { return ZAtomic_Get( &mFramesRead ); };
		int SilentFrames() const { return ZAtomic_Get( &mSilentFrames ); };
		
		int WriteWaits() const { return ZAtomic_Get( &mWriteWaits ); };
		
		class Terminated: public std::exception
		{
			public:
			
			Terminated() throw();
			Terminated( Terminated const &inOriginal ) throw();
			virtual ~Terminated() throw();
			
			Terminated &operator=( Terminated const &inRHS ) throw();
			
			virtual const char* what() const throw();
		};
			
	private:
	
		enum{
			kRun,
			kTerminate
		};
		
		int				mBufferSize;
		float			*mBuffer;
		ZAtomic_t		mHead;			// The next empty slot into which a frame will be written
		ZAtomic_t		mTail;			// The first full slot from which a frame will be read
		ZAtomic_t		mSpaceNeeded;
		ZSemaphore		mSpaceAvailable;
		ZAtomic_t		mStop;
		ZAtomic_t		mFramesWritten;
		ZAtomic_t		mFramesRead;
		ZAtomic_t		mSilentFrames;
		ZAtomic_t		mWriteWaits;
		
		
		inline int HowFull( int inHead, int inTail ) const; // inHead == inTail means the queue is empty
		inline int SpaceAvailable( int inHead, int inTail ) const;
};

inline int ZPCMQueue::SpaceAvailable( int inHead, int inTail ) const
{
	int result;
	int howFull = HowFull( inHead, inTail );
	
	// we have to keep at least one free frame so that filling up doesn't make head == tail.
	// Otherwise we wouldn't be able to distinguish the full from the empty case.
		
	result = ( mBufferSize - 1 ) - howFull;
	
	OFAssert( result >= 0 );
	OFAssert( result < mBufferSize );
	
	return result;
}

inline int ZPCMQueue::HowFull() const
{
	// This isn't completely reliable, as head can change before fetching the tail
	return HowFull( ZAtomic_Get( &mHead ), ZAtomic_Get( &mTail ) );
}

inline int ZPCMQueue::HowFull( int inHead, int inTail ) const
{
	int result;
	
	OFAssert( inHead < mBufferSize );
	OFAssert( inTail < mBufferSize );
	
	if ( inHead > inTail ){
	
		result = inHead - inTail;

	}else if ( inHead == inTail ){
		
		result = 0;
	}else{
	
		result = inHead + mBufferSize - inTail;
	}
	
	OFAssert( result >= 0 );
	OFAssert( result < mBufferSize );
	
	return result;
}


#endif // __ZPCMQUEUE__
