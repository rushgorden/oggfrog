// ZPCMSource_Mad.cpp

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

#include "ZPCMSource_Mad.h"
#include <stdio.h>
#include "ZThreadSimple.h"
#include "ZAudioPlayer.h"
#include "id3tag.h"

ZPCMSource_Mad::ZPCMSource_Mad( ZFileSpec const &inFile, ZMessenger inClient )
	: ZPCMSource( inFile, inClient ),
		mReader( mRawStream->GetStreamRPos() ),
		mOutputBufferEnd( nil ),
		mGuardPtr( nil ),
		mFrameCount( 0 ),
		mTimeVec()
{
	mNewTime.fValue = -1;

	mTotalTime.fValue = 0;

	mad_stream_init( &mStream );
	mad_frame_init( &mFrame );
	mad_synth_init( &mSynth );
	mad_timer_reset( &mTimer );

	// We record file offsets to the TimeVec once a second
	mad_timer_set( &mOffsetInterval, 1, 0, 1 );

	ZThreadSimple< ZPCMSource_Mad& > *decodeProc 
		= new ZThreadSimple< ZPCMSource_Mad& >( &DecoderProc, *this, inFile.Name().c_str() );
	
	decodeProc->Start();

	ZAtomic_Set( &mStop, kPause );

	return;
}

ZPCMSource_Mad::~ZPCMSource_Mad()
{
	Terminate();

	mad_synth_finish( &mSynth );
	mad_frame_finish( &mFrame );
	mad_stream_finish( &mStream );

	//delete [] mInputBuffer;

	return;
}

void ZPCMSource_Mad::DecoderProc( ZPCMSource_Mad &me )
{
	me.DecoderProc();
	
	return;
}

void ZPCMSource_Mad::DecoderProc()
{
	try{			
		
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
		for ( int i = 0; i < 2; ++i ){
			mOutputs.push_back( &player.FIFO( i ) );
		}
		
		
		while( true ){
			
			ZThread::Error err = ZThread::errorNone;
			
			do{			
				err = mPlayLock.Wait( 1, 50000 );
					
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
				
				SkipTags();
				
				RecordOffset( mReader.GetPosition(), mad_timer_zero );
				
				// mTimeVec.push_back( TimeOffset( mReader.GetPosition(), mad_timer_zero ) );
				
				while ( true ){
				
					int iNewTime = ZAtomic_Swap( &mNewTime, -1 );
					
					if ( iNewTime != -1 ){

						float fNewTime = *reinterpret_cast< float* >( &iNewTime );
						
						mad_timer_t newTimer;
						
						FloatToTimer( newTimer, fNewTime );
						
						SetTime( newTimer );
					}

					size_t curOffset = mReader.GetPosition();

					Read();
					
					if( mad_frame_decode( &mFrame, &mStream) ){
					
						if( MAD_RECOVERABLE( mStream.error ) )
						{
							if( mStream.error != MAD_ERROR_LOSTSYNC ||
							   mStream.this_frame != mGuardPtr)
							{
								// report recoverable error
							}
							continue;
						}else{
							if( mStream.error == MAD_ERROR_BUFLEN ){
							
								continue;
							}else{
								
								// Unrecoverable error - corrupt input
								throw DecoderDone();
								
								break;
							}
						}
					}

					// if ( mFrameCount == 0 )
					// Setup output channels
					
					++mFrameCount;
		
					mad_timer_add( &mTimer, mFrame.header.duration);
				
					float curTime = TimerToFloat( mTimer );
			
					ZAtomic_Set( &mCurTime, *reinterpret_cast< int* >( &curTime ) );	
										
					// Apply filter here
					
					mad_synth_frame( &mSynth, &mFrame);


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
	
					float *leftOut = mOutputBuffer[ 0 ];
					float *rightOut = mOutputBuffer[ 1 ];
					long count;
											
					for( int i=0; i< mSynth.pcm.length; ++i ){
						
						float sample;

						/* Left channel */
						sample = Clip( mad_f_todouble( mSynth.pcm.samples[ 0 ][ i ] ) );
						*leftOut++ = sample;
						
						if( MAD_NCHANNELS( &mFrame.header )==2 ){
							sample = Clip( mad_f_todouble( mSynth.pcm.samples[ 1 ][ i ] ) );
						}
							
						*rightOut++ = sample;

						/* Flush the output buffer if it is full. */
						if( leftOut == mOutputBufferEnd ){
							
							count = leftOut - mOutputBuffer[ 0 ];
							
							mOutputs[ 0 ]->Write( mOutputBuffer[ 0 ], count );
							mOutputs[ 1 ]->Write( mOutputBuffer[ 1 ], count );

							leftOut = mOutputBuffer[ 0 ];
							rightOut = mOutputBuffer[ 1 ];

						}
					}
					
					count = leftOut - mOutputBuffer[ 0 ];
						
					mOutputs[ 0 ]->Write( mOutputBuffer[ 0 ], count );
					mOutputs[ 1 ]->Write( mOutputBuffer[ 1 ], count );
				}

			}catch( DecoderPaused &s ){
			
			}			
		} // while true
		
	}catch ( ZPCMQueue::Terminated &t ){
	

	}catch ( DecoderTerminated &t ){
	
	
	}catch ( DecoderDone &d ){
	
		SendFinishedMessage();
	
	}

#if 0
	for ( TimeVec::const_iterator it( mTimeVec.begin() );
			it != mTimeVec.end();
			++it ){
		printf( "%f\n", TimerToFloat( it->mTime ) );
	}
#endif
	
	ZAtomic_Set( &mStop, kDone );
	
	return;	// statement unreachable
}

void ZPCMSource_Mad::FloatToTimer( mad_timer_t &outTimer, float inTime )
{
	outTimer.seconds = inTime;
	outTimer.fraction = ( inTime - outTimer.seconds ) * MAD_TIMER_RESOLUTION;
	
	return;
}

void ZPCMSource_Mad::Read()
{
	if( mStream.buffer==NULL || mStream.error == MAD_ERROR_BUFLEN)
	{
		size_t			readSize;
		size_t			remaining;
		unsigned char	*readStart;

		if( mStream.next_frame!=NULL)
		{
			remaining = mStream.bufend - mStream.next_frame;
			
			memmove( mInputBuffer, mStream.next_frame, remaining );
			
			readStart = mInputBuffer + remaining;
			readSize = kInputBufferSize - remaining;
		}else{
		
			readSize = kInputBufferSize;
			readStart = mInputBuffer;
			remaining = 0;
		}

		RecordOffset( mReader.GetPosition(), mTimer );

		mReader.Read( readStart, readSize, &readSize );
		
		if( readSize <= 0)
		{
			throw DecoderDone();
		}
		
		if( mReader.CountReadable() == 0 )
		{
			mGuardPtr = readStart + readSize;
			
			memset( mGuardPtr, 0, MAD_BUFFER_GUARD);
			
			readSize += MAD_BUFFER_GUARD;
		}

		mad_stream_buffer( &mStream, mInputBuffer, readSize + remaining);

		mStream.error = MAD_ERROR_NONE;

	}
		
	return;
}

void ZPCMSource_Mad::SetTime( mad_timer_t const &inNewTime )
{
	size_t newOffset = 0;
	mad_timer_t actualTime;
	bool found = false;
	
	for ( TimeVec::const_iterator it( mTimeVec.begin() );
			it != mTimeVec.end();
			++it ){
			
		if ( mad_timer_compare( it->mTime, inNewTime ) >= 0 ){

			mTimer = it->mTime;
			
			mReader.SetPosition( it->mOffset );

			mad_synth_finish( &mSynth );
			mad_frame_finish( &mFrame );
			mad_stream_finish( &mStream );

			mad_stream_init( &mStream );
			mad_frame_init( &mFrame );
			mad_synth_init( &mSynth );
			
			return;
		}
		
	}

	mad_header header;
	
	mad_header_init( &header );
	
	do{

		Read();

		float curTime = TimerToFloat( mTimer );

		ZAtomic_Set( &mCurTime, *reinterpret_cast< int* >( &curTime ) );
		
		if( mad_header_decode( &header, &mStream) ){
		
			if( MAD_RECOVERABLE( mStream.error ) )
			{
				if( mStream.error != MAD_ERROR_LOSTSYNC ||
				   mStream.this_frame != mGuardPtr)
				{
					// report recoverable error
				}
				continue;
			}else{
				if( mStream.error == MAD_ERROR_BUFLEN ){
				
					continue;
				}else{
					
					// Unrecoverable error - corrupt input
					throw DecoderDone();
					
					break;
				}
			}
		}

		// if ( mFrameCount == 0 )
		// Setup output channels
		
		++mFrameCount;

		if ( header.duration.seconds > 1 ){
			int x;
			x = 1;
		}
		
		mad_timer_add( &mTimer, header.duration);
	
	}while( mad_timer_compare( mTimer, inNewTime ) < 0 );

	return;
}

void ZPCMSource_Mad::RecordOffset( size_t offset, mad_timer_t const &inTime )
{
	if ( mTimeVec.size() < 1 ){
		mTimeVec.push_back( TimeOffset( offset, mad_timer_zero ) );
		return;
	}

	mad_timer_t lastTime( ( mTimeVec.end() -1 )->mTime );

	mad_timer_negate( &lastTime );
	
	mad_timer_t diff( inTime );
	
	mad_timer_add( &diff, lastTime );
	
	if ( mad_timer_compare( diff, mOffsetInterval ) >= 0 ){
	
		mTimeVec.push_back( TimeOffset( offset, inTime ) );
		
		EstimateTotalTime();
		
		// printf( "%f\n", TotalTime() );
	}
	
	return;
}

void ZPCMSource_Mad::SetTime( float newTime )
{
	ZAtomic_Swap( &mNewTime,
					*reinterpret_cast< int* >( &newTime ) );
					
	return;
}
				
float ZPCMSource_Mad::TotalTime() const
{
	int iTotal = ZAtomic_Get( &mTotalTime );
	
	
	return *(float*)&iTotal;
}
			
float ZPCMSource_Mad::CurTime() const
{
	int iCurTime = ZAtomic_Get( &mCurTime );
	
	float floatCurTime = *reinterpret_cast< float* >( &iCurTime );
	
	return floatCurTime;
}

void ZPCMSource_Mad::SkipTags()
{
	do{
	
		unsigned char buf[ ID3_TAG_QUERYSIZE ];
	
		size_t countRead;
	
		mReader.Read( buf, ID3_TAG_QUERYSIZE, &countRead );
	
		if ( countRead != ID3_TAG_QUERYSIZE )
			return; // Throw?
			
		signed long tagLength;
		
		tagLength = id3_tag_query( buf, ID3_TAG_QUERYSIZE );
		
		if ( tagLength == 0 ){
			mReader.SetPosition( mReader.GetPosition() - ID3_TAG_QUERYSIZE );
			return;
		}
			
		if ( tagLength > 0 ){
			mReader.SetPosition( mReader.GetPosition() + tagLength - ID3_TAG_QUERYSIZE );
			continue;
		}
		
		if ( tagLength < 0 )
			mReader.SetPosition( mReader.GetPosition() + tagLength );
	}while( true );
	
}

void ZPCMSource_Mad::EstimateTotalTime()
{
	if ( mTimeVec.size() < 2 )
		return;

	TimeOffset beginning( *mTimeVec.begin() );
	
	TimeOffset curTimeOffset( *( mTimeVec.end() - 1 ) );
	
	float curTime = TimerToFloat( curTimeOffset.mTime );
	
	float portionPlayed = (float)( curTimeOffset.mOffset - beginning.mOffset )
							/ (float)( mReader.GetSize() - beginning.mOffset );
	
	float totalTime = curTime / portionPlayed;
	
	ZAtomic_Set( &mTotalTime, *reinterpret_cast< int* >( &totalTime ) );
	
	// printf( "%f\n", totalTime );
	
	return;
}

float ZPCMSource_Mad::TimerToFloat( mad_timer_t const &inTimer )
{
	float time = (float)inTimer.seconds + ( (float)inTimer.fraction / (float)MAD_TIMER_RESOLUTION );
	
	return time;
}

ZPCMSource_Mad::TimeOffset::TimeOffset( size_t inOffset, mad_timer_t const &inTime )
	: mOffset( inOffset ),
		mTime( inTime )
{
	return;
}

ZPCMSource_Mad::TimeOffset::TimeOffset( TimeOffset const &inOrig )
	: mOffset( inOrig.mOffset ),
		mTime( inOrig.mTime )
{
	return;
}

	
