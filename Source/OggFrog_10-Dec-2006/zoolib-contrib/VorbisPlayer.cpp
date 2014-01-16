// VorbisPlayer.cpp

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

 
#include "VorbisPlayer.h"
#include <iostream>
#include <new>
#include <math.h>
#include <cstring>

#if ZCONFIG( OS, Win32 )
#include "ZWinHeader.h"
#endif

#include "ZAudioPlayer.h"
#include "ZPCMQueue.h"

using std::auto_ptr;
using std::bad_alloc;
using std::vector;
using std::exception;

VorbisPlayer::VorbisPlayer( ZFileSpec const &inFile, ZMessenger inClient )
	: ZPCMSource( inFile, inClient ),
		mDecoder( Open( mRawStream ) )
{
	mTotalTime = ov_time_total( &mDecoder->mFile, -1 );

	ZThreadSimple< VorbisPlayer& > *decodeProc 
		= new ZThreadSimple< VorbisPlayer& >( &DecoderProc, *this, inFile.Name().c_str() );
	
	decodeProc->Start();

	ZAtomic_Set( &mStop, kPause );
	
	return;
}

VorbisPlayer::~VorbisPlayer()
{
	Terminate();

	return;
}

void VorbisPlayer::SetTime( float newTime )
{
	ZAtomic_Swap( &mDecoder->mNewTime,
					*reinterpret_cast< int* >( &newTime ) );
					
	return;
}

float VorbisPlayer::TotalTime() const 
{
	return mTotalTime;
}
			
float VorbisPlayer::CurTime() const 
{
	int haxor = ZAtomic_Get( &mDecoder->mCurTime );
				
	return *reinterpret_cast< float* >( static_cast< void* >( &haxor ) );
}

void VorbisPlayer::DecoderProc( VorbisPlayer &me )
{
	me.DecoderProc();
	
	return;
}

void VorbisPlayer::DecoderProc()
{
	try{
	
		Decoder &decoder = *mDecoder;

		int currentSection = 0;
			
		StreamChange( -1 );
		
		ZAudioPlayer &player = ZAudioPlayer::sGet();
					
		for ( int i = 0; i < decoder.mChannels; ++i ){
			mOutputs.push_back( &player.FIFO( i ) );
		}

		
		while( true ){
			
			do{
				ZThread::Error err = ZThread::errorNone;
				
				mPlayLock.Wait( 1, 50000 );
					
				if ( ZThread::errorNone == err  ){
					break;
				}else{
				
					if ( ZThread::errorTimeout == err ){
					
						int stop = ZAtomic_Get( &mStop );
				
						if ( stop == kTerminate ){
						
							throw DecoderTerminated();
							
						}else if ( stop == kStop ){
						
							throw DecoderDone();
						}
					}else{
					
						ZAssert( ZThread::errorDisposed == err );
						
						// This should only happen if the app is quitting out from under
						// us.  We can't count on our instance variables being valid.
						
						return;
					}
				}
			}while( true );
			
			if ( ZAtomic_Get( &mStop ) == kTerminate )
				throw DecoderTerminated();

			try{
							
				while ( true ){
					
					float **pcmChannels;
					
					int oldSection = currentSection;
					
					long samplesIn = ov_read_float( &decoder.mFile, &pcmChannels, decoder.mSamples, &currentSection );
					
					if ( currentSection != oldSection ){
						
						StreamChange( currentSection );
					}
						
					if ( 0 == samplesIn ){
					
						throw DecoderDone();
							
					} else if ( samplesIn < 0 ){
					
						// Missing or corrupt data
							
					}else{
						
						int iNewTime = ZAtomic_Swap( &decoder.mNewTime, -1 );
						
						if ( iNewTime != -1 ){
							int result;
							float fNewTime = *reinterpret_cast< float* >( &iNewTime );
							
							result = ov_time_seek( &decoder.mFile, fNewTime );
						}
						
						float curTime = static_cast< float >( ov_time_tell( &decoder.mFile ) );
						
						ZAtomic_Set( &decoder.mCurTime, *reinterpret_cast< int* >( static_cast< void* >( &curTime ) ) );
						
						for ( int i = 0; i < decoder.mChannels; ++i ){
						
							int stop = ZAtomic_Get( &mStop );
							
							ZAssert( stop >= kRun && stop < kDone );
							
							if ( stop != kRun ){
							
								if ( stop == kPause ){
								
									throw DecoderPaused();
									
								}else if ( stop == kTerminate ){
								
									throw DecoderTerminated();
									
								}else if ( stop == kStop ){
								
									throw DecoderDone();
									
								}
								
								ZAssert( false );
							}	
							
							for ( int j = 0; j < samplesIn; ++j )
								pcmChannels[ i ][ j ] = Clip( pcmChannels[ i ][ j ] );
										
							mOutputs[ i ]->Write( pcmChannels[ i ], samplesIn );
						}
					}
				}

			}catch( DecoderPaused &s ){
			
			}
			
		} // while true
		
	}catch ( ZPCMQueue::Terminated &t ){
	

	}catch ( DecoderTerminated &t ){
	
	
	}catch ( DecoderDone &d ){
	
		SendFinishedMessage();
	
	}
	
	ZAtomic_Set( &mStop, kDone );
	
	return;	// statement unreachable
}

void VorbisPlayer::StreamChange( int inStreamIndex )
{
	// Pass -1 for inStreamIndex to set the number of samples for the current stream
	
	ZAssert( mDecoder->mOpened );
	
	vorbis_info *vi = ov_info( &mDecoder->mFile, -1 );
	
	mDecoder->mChannels = vi->channels;
	
	mDecoder->mSamples = sizeof( mDecoder->mPCMBuf ) / ( sizeof( float ) * mDecoder->mChannels );

	return;
}

auto_ptr< VorbisPlayer::Decoder > VorbisPlayer::Open( ZRef< ZStreamerRPos > &input )
{
	auto_ptr< Decoder > result( new Decoder );
	Decoder &dc = *result;
	
	ov_callbacks callbacks = { Read, Seek, Close, Tell };
	
	if ( 0 != ov_open_callbacks( &input, &dc.mFile, NULL, 0, callbacks ) )
		throw bad_alloc();
		
	dc.mOpened = true;
	
	return result;
}

VorbisPlayer::Decoder::Decoder()
	: mOpened( false )
{
	mNewTime.fValue = -1;
	
	return;
}

VorbisPlayer::Decoder::~Decoder()
{
	if ( mOpened ){
	
		ZAssert( mFile.datasource != nil );
		
		ov_clear( &mFile );
	}
		
	return;
}

size_t VorbisPlayer::Read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	ZAssert( datasource != nil );
	
	ZStreamR const &r = (* reinterpret_cast< ZRef< ZStreamerRPos > * >( datasource ) )->GetStreamR();
	
	size_t countRead;
	
	r.Read( ptr, size * nmemb, &countRead );
	
	return countRead / size;
}

int VorbisPlayer::Seek(void *datasource, ogg_int64_t offset, int whence)
{
	ZAssert( datasource != nil );
	
	ZStreamRPos const &r = (* reinterpret_cast< ZRef< ZStreamerRPos > * >( datasource ) )->GetStreamRPos();

	switch( whence ){
		case SEEK_SET:
			r.SetPosition( offset );
			break;
			
		case SEEK_CUR:
			r.SetPosition( offset + r.GetPosition() );
			break;
		
		case SEEK_END:
			r.SetPosition( r.GetSize() - offset );
			break;
			
		default:
			return -1;
			break;
	}
			
	return 0;
}

int VorbisPlayer::Close(void *datasource)
{
	ZAssert( datasource != nil );
	
	ZRef< ZStreamerRPos > *streamer = reinterpret_cast< ZRef< ZStreamerRPos > * >( datasource );

	*streamer = nil;
	
	return 0;
}

long VorbisPlayer::Tell(void *datasource)
{
	ZAssert( datasource != nil );
	
	ZStreamRPos const &r = (* reinterpret_cast< ZRef< ZStreamerRPos > * >( datasource ) )->GetStreamRPos();
	
	return r.GetPosition();
}
		
ZRef< ZStreamerRPos > VorbisPlayer::OpenInput( ZFileSpec const &inFile )
{
	ZFile::Error err;
	
	ZRef< ZStreamerRPos > result( inFile.OpenRPos( true, &err ) );
	
	if ( ZFile::errorNone != err ){
		throw std::bad_alloc();
	}
	
	return result;
}

