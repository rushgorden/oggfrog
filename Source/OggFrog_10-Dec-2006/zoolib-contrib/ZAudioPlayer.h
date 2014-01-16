// ZAudioPlayer.h

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

#ifndef __ZAUDIOPLAYER__
#define __ZAUDIOPLAYER__

#include <vector>
#include "ZThread.h"
#include "ZRefCount.h"
#include "ZFIFOMonitor_PCM.h"
#include "ZPCMSink.h"
#include "ZPCMSource.h"

class ZAudioPlayer
{
		ZAudioPlayer();
		ZAudioPlayer( ZAudioPlayer const &inOrig );
		~ZAudioPlayer();
		
		ZAudioPlayer const &operator=( ZAudioPlayer const &inRHS );
		
	public:
	
		static ZAudioPlayer &sGet();
		
		ZMutexBase &GetLock()
		{ return mLock; }
		
		ZFIFOMonitor_PCM &GetFIFOMonitor() const;
		
		static const int kLeft;
		static const int kRight;

		// in non-const FIFO, if the index is beyond the end of our vector of FIFOs, 
		// new ones will be created to accomodate.
		// For thread safety, you must therefore lock the application object.
		
		//ZPCMQueue &FIFO( unsigned int inWhichChannel );
		
		// const FIFO requires that the indexed FIFO already exist.  No need to lock.
		ZPCMQueue &FIFO( unsigned int inWhichChannel ) const;

		void SetTrack( ZFileSpec const &inTrack, ZMessenger inClient );

		void Play();
		
		void Pause();
		
		void Stop();
		
		bool Playing() const;
		
		bool TrackSelected() const;

		float TotalTime() const;			
		float CurTime() const; 

		void SetPosition( float newTime );

		void PlayerFinished();

	private:
	
		static ZAudioPlayer *sSingleton;
	
		enum{
			kOutputFIFOSize = 44100 * sizeof( float )
		};
		
		
		ZMutex					mLock;
		FIFOVector				mFIFOs;
		ZRef< ZFIFOMonitor_PCM >		mFIFOMonitor;
		ZFileSpec				mMusicFile;
		ZRef< ZPCMSink >		mOutput;
		ZRef< ZPCMSource >		mPlayer;
		ZMessenger				mClient;

		int CountFIFOChannels() { return mFIFOs.size(); }

		static FIFOVector MakeFIFOs();

		void DrainFIFO();
};		
	
#endif
