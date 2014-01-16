// ZAudioPlayer.cpp

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

#include "ZAudioPlayer.h"
#include "ZPCMSink_Null.h"		// For debugging

#define ASSERTLOCKED 	const_cast< ZMutex* >( &mLock )->IsLocked()

const int ZAudioPlayer::kLeft = 0;
const int ZAudioPlayer::kRight = 1;

ZAudioPlayer *ZAudioPlayer::sSingleton = nil;

ZAudioPlayer::ZAudioPlayer()
	: mLock( "ZAudioPlayer lock", true ),
		mFIFOs( MakeFIFOs() ),
		mFIFOMonitor( new ZFIFOMonitor_PCM( mFIFOs ) ),
		mMusicFile(),
		mOutput( ZPCMSink::sCreateOutput( mFIFOs ) ),
		//mOutput( new ZPCMSink_Null( mFIFOs ) ),
		mPlayer(),
		mClient()
{
	mLock.Release();
	
	return;
}
	
ZAudioPlayer::~ZAudioPlayer()
{
	for ( FIFOVector::iterator it( mFIFOs.begin() );
			it != mFIFOs.end();
			++it ){
		(*it)->Stop();
	}
	
	return;
}

ZAudioPlayer &ZAudioPlayer::sGet()
{
	if ( sSingleton == nil )
		sSingleton = new ZAudioPlayer;
		
	return *sSingleton;
}

void ZAudioPlayer::SetTrack( ZFileSpec const &inTrack,
								ZMessenger inClient )
{
	ASSERTLOCKED;
	
	mClient = inClient;
	
	mMusicFile = inTrack;
	
	mPlayer = ZPCMSource::Create( mMusicFile, mClient );
	
	return;
}

void ZAudioPlayer::Play()
{
	ASSERTLOCKED;
	
	if ( mPlayer != nil ){
		mPlayer->Play();

		ZThread::sSleep( 200 );

		mOutput->Start();
	}
		
	return;
}

bool ZAudioPlayer::Playing() const
{
	ASSERTLOCKED;
	
	if ( mPlayer == nil || !mPlayer->Playing() )
		return false;
		
	return true;
}

float ZAudioPlayer::TotalTime() const 
{
	ASSERTLOCKED;
	
	if ( mPlayer == nil )
		return 0.0;
		
	return mPlayer->TotalTime();
}
			
float ZAudioPlayer::CurTime() const
{
	ASSERTLOCKED;
	
	if ( mPlayer == nil )
		return 0.0;
		
	return mPlayer->CurTime();
}

bool ZAudioPlayer::TrackSelected() const
{
	ASSERTLOCKED;
		
	return mPlayer != nil;
}

ZFIFOMonitor_PCM &ZAudioPlayer::GetFIFOMonitor() const
{
	ZAssert( nil != mFIFOMonitor.GetObject() );
	
	return *mFIFOMonitor.GetObject();
}

void ZAudioPlayer::Pause()
{
	ASSERTLOCKED;
	
	mOutput->Stop();
	
	if ( mPlayer != nil ){
		mPlayer->Pause();
	}

	return;
}

void ZAudioPlayer::SetPosition( float newTime )
{
	ASSERTLOCKED;
	
	if ( mPlayer == nil )
		return;

	mPlayer->SetTime( newTime );
	
	return;
}

void ZAudioPlayer::Stop()
{
	ASSERTLOCKED;
	
	if ( mPlayer != nil ){
		mPlayer->Stop();
	}

	return;
}

void ZAudioPlayer::PlayerFinished()
{
	ASSERTLOCKED;
	
	DrainFIFO();
	
	mOutput->Stop();

	mPlayer = ZPCMSource::Create( mMusicFile, mClient );
	
	return;
}

void ZAudioPlayer::DrainFIFO()
{	
	if ( mFIFOs.size() == 0 )
		return;
		
	bool stopped = mOutput->Stopped();
	
	try{
		if ( stopped )
			mOutput->Start();
				
		while ( mFIFOs[ 0 ]->HowFull() != 0 )
			ZThread::sSleep( 100 );
	}catch(...){
	
		if ( stopped )
			mOutput->Stop();
			
		throw;
	}

	if ( stopped )
		mOutput->Stop();

	return;
}

ZPCMQueue &ZAudioPlayer::FIFO( unsigned int inWhichChannel ) const
{
	// const FIFO requires that the indexed FIFO already exist.  No need to lock.

	ZAssert( inWhichChannel < mFIFOs.size() );
	
	ZRef< ZPCMQueue > fifo( mFIFOs[ inWhichChannel ] );
	
	return *( fifo.GetObject() );
}

FIFOVector ZAudioPlayer::MakeFIFOs()
{
	FIFOVector result;
	
	result.push_back( ZRef< ZPCMQueue >( new ZPCMQueue( kOutputFIFOSize ) ) );
	result.push_back( ZRef< ZPCMQueue >( new ZPCMQueue( kOutputFIFOSize ) ) );

	return result;
}

