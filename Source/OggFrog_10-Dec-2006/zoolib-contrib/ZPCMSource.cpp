// ZPCMSource.cpp

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

#include "ZPCMSource.h"
#include "VorbisPlayer.h"
#include "ZPCMSource_Mad.h"

ZPCMSource::ZPCMSource( ZFileSpec const &inFile, ZMessenger inClient )
	: mOutputs(),
		mRawStream( OpenInput( inFile ) ),
		mClient( inClient ),
		mPlayLock( 0, "ZPCMSource Play Lock" ),
		mStop(),
		mTotalTime( 0.0 )

{
	mStop.fValue = kDone;

	return;
}

ZPCMSource::~ZPCMSource()
{
	return;
}

void ZPCMSource::Terminate()
{
	// If we just try to dispose of the decoder data while the decoder
	// thread is still running, there will be a crash as the decoder
	// tries to use deleted data
	
	int previous = ZAtomic_Swap( &mStop, kTerminate );
		
	if ( previous != kDone ){
	
		mPlayLock.Signal( 1 );
	
		while ( ZAtomic_Get( &mStop ) != kDone )
			ZThread::sSleep( 10 );
	}

	return;
}

ZRef< ZPCMSource > ZPCMSource::Create( ZFileSpec const &inFile, ZMessenger inClient )
{
	ZRef< ZPCMSource > result;

	switch( inFile.Name()[ inFile.Name().size() - 1 ] ){
	
		case 'g':
			result = ZRef< ZPCMSource >( new VorbisPlayer( inFile, inClient ) );
			break;

		case '3':
			result = ZRef< ZPCMSource >( new ZPCMSource_Mad( inFile, inClient ) );
			break;
			
		default:
			result = ZRef< ZPCMSource >();
			break;
	}

	return result;
}
		
void ZPCMSource::Play()
{
	ZAtomic_Set( &mStop, kRun );
	
	mPlayLock.Signal( 1 );
	
	return;
}

bool ZPCMSource::Playing()
{
	return kRun == ZAtomic_Get( &mStop );
}

void ZPCMSource::Pause()
{
	ZAtomic_Set( &mStop, kPause );
	
	return;
}

void ZPCMSource::Stop()
{
	ZAtomic_Set( &mStop, kStop );
	
	return;
}
		
void ZPCMSource::SetTime( float newTime )
{
	return;
}

float ZPCMSource::TotalTime() const
{
	return 0.0;
}
			
float ZPCMSource::CurTime() const
{
	return 0.0;
}

ZRef< ZStreamerRPos > ZPCMSource::OpenInput( ZFileSpec const &inFile )
{
	ZFile::Error err;
	
	ZRef< ZStreamerRPos > result( inFile.OpenRPos( true, &err ) );
	
	if ( ZFile::errorNone != err ){
		throw std::bad_alloc();
	}
	
	return result;
}

void ZPCMSource::SendFinishedMessage()
{
	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:player-finished" );
	
	msg.SetPointer( "player", this );
	
	mClient.PostMessage( msg );
	
	return;
}

ZPCMSource::DecoderDone::DecoderDone() throw()
	: exception()
{
	return;
}

ZPCMSource::DecoderDone::~DecoderDone() throw()
{
	return;
}

ZPCMSource::DecoderPaused::DecoderPaused() throw()
	: exception()
{
	return;
}

ZPCMSource::DecoderPaused::~DecoderPaused() throw()
{
	return;
}

ZPCMSource::DecoderTerminated::DecoderTerminated() throw()
	: exception()
{
	return;
}

ZPCMSource::DecoderTerminated::~DecoderTerminated() throw()
{
	return;
}
