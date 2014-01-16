// ZPCMSink_WinWaveOut.cpp

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

#include "ZPCMSink_WinWaveOut.h"
#include <new>
#include <cstring>
#include "ZThreadSimple.h"
#include "ZPCMQueue.h"
#include "ZTicks.h"

#if ZCONFIG( OS, Win32 )

using std::bad_alloc;

ZPCMSink_WinWaveOut::ZPCMSink_WinWaveOut( FIFOVector &inFIFOs )
	: ZPCMSink( inFIFOs ),
		mSineWavePhase( 0.0 ),
		mRawPCM( new float[ kBufferSize * 2 ] ),
		mStartSignal( 0, "Wave Out Start Signal" ),
		mStop(),
		mAvailableBuffers(),
		mTerminate(),
		mBufferAvailable( 0, "Wave Out Buffer Available Signal" ),
		mDevice( nil ),
		mBuffers(),
		mNextBuffer()
{
	mStop.fValue = true;
	
	for ( int i = 0; i < kMaxBuffers; ++i )
		mBuffers.push_back( Buffer() );
	
	mAvailableBuffers.fValue = kMaxBuffers;
	
	mTerminate.fValue = false;
	
	//mBufferAvailable.Signal( 1 );
		
	mNextBuffer = mBuffers.begin();
	
	if ( !waveOutGetNumDevs() )
		throw bad_alloc();

	WAVEFORMATEX format;
	
	memset(&format, 0, sizeof( format ) );
	
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.wBitsPerSample = 16;
	format.nChannels = 2;
	format.nSamplesPerSec = 44100;
	format.nAvgBytesPerSec = format.nSamplesPerSec	
								* format.nChannels
								* format.wBitsPerSample / 8;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;	// Ignored for WAVE_FORMAT_PCM
	
	MMRESULT err = waveOutOpen( &mDevice,
								WAVE_MAPPER,
								&format,
								reinterpret_cast< DWORD_PTR >( Callback ),
								reinterpret_cast< DWORD_PTR >( this ),
								CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT | WAVE_ALLOWSYNC );
								
	if ( err ){
	
		ZAssert( mDevice == nil );
		
		throw bad_alloc();
	}


	ZThreadSimple< ZPCMSink_WinWaveOut& > *thread = 
		new ZThreadSimple< ZPCMSink_WinWaveOut& >( OutputThread,
													*this,
													"Wave Out Output" );
	thread->Start(); 	
	
	return;
								
								
}

ZPCMSink_WinWaveOut::~ZPCMSink_WinWaveOut()
{
	if ( mDevice != nil ){
		HRESULT err;
		
		// If we just try to close while there are still buffers that
		// haven't been returned to the callback, waveOutClose will
		// return WAVERR_STILLPLAYING (33).  I'm not certain, but I expect it
		// could lead to a crash if the callback is called after we've
		// been destructed
		
		ZAtomic_Set( &mTerminate, true );
		
		while ( ZAtomic_Get( &mAvailableBuffers ) != kMaxBuffers ){
			
			// It would be more efficient to wait on a semaphore, but I'm
			// not sure how to avoid a race condition in which we wait just
			// after the last buffer has been freed.
			
			ZThread::sSleep( 10 );
		}
		
		err = waveOutClose( mDevice );
		
		
		ZAssert( err == MMSYSERR_NOERROR );
	}
	
	return;
}
		
void ZPCMSink_WinWaveOut::Start()
{
	if ( ZAtomic_Get( &mStop ) == false )
		return;
		
	ZAtomic_Set( &mStop, false );

bigtime_t start = ZTicks::sNow();

	mStartSignal.Signal( 1 );
	
bigtime_t elapsed = ZTicks::sNow() - start;
	
	return;
}

void ZPCMSink_WinWaveOut::Stop()
{
	ZAtomic_Set( &mStop, true );

	return;
}

bool ZPCMSink_WinWaveOut::Stopped() const
{
	return ZAtomic_Get( &mStop );
}

void ZPCMSink_WinWaveOut::FreeBuffer()
{
	int oldValue = ZAtomic_Add( &mAvailableBuffers, 1 );
	
	if ( oldValue == -1 ){	
	
		ZAtomic_Add( &mAvailableBuffers, 1 );
			
		mBufferAvailable.Signal( 1 );
	}
		
	return;
}

void CALLBACK ZPCMSink_WinWaveOut::Callback( HWAVEOUT device,      
												UINT message,         
												DWORD_PTR refCon,  
												DWORD msgParam1,    
												DWORD msgParam2 )
{
	if ( WOM_DONE != message )
		return;
	
	ZAssert( refCon != NULL );
		
	ZPCMSink_WinWaveOut &me = *reinterpret_cast< ZPCMSink_WinWaveOut* >( refCon );
	
	ZAssert( device == me.mDevice );
	
	WAVEHDR *header = reinterpret_cast< WAVEHDR* >( msgParam1 );
	
	ZAssert( header->dwFlags == ( WHDR_PREPARED | WHDR_DONE ) );
	
	HRESULT err = waveOutUnprepareHeader( device, header, sizeof( *header ) );

	ZAssert( header->dwFlags == WHDR_DONE );
	
	ZAssert( ( err == MMSYSERR_NOERROR ) 
				|| ( err == MMSYSERR_NOMEM ) );
	
	me.FreeBuffer();
	
	return;
}

void ZPCMSink_WinWaveOut::WaitForInput()
{
	InputFIFOVector &inputs = GetInputs();
	
	do{
	
		if ( ZAtomic_Get( &mStop ) || ZAtomic_Get( &mTerminate ) )
			return;
			
		float fillLevel = 1.0;
		
		for ( int i = 0; i < inputs.size(); ++i ){
		
			fillLevel = min( fillLevel, inputs[ i ]->FillLevel() );
		}
		
		if ( fillLevel > 0.1 )
			return;
		
		ZThread::sSleep( 100 );
	}while( true );
	
	// unreachable
}		

void ZPCMSink_WinWaveOut::OutputThread( ZPCMSink_WinWaveOut &me )
{
	me.OutputThread();
	
	return;
}

void ZPCMSink_WinWaveOut::OutputThread()
{

	//SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	do{
	
		if ( ZThread::errorNone != mStartSignal.Wait( 1 ) )
			return;
			
		WaitForInput();
		
		while( !ZAtomic_Get( &mStop ) ){
			
			if ( ZAtomic_Get( &mTerminate ) )
				return;

			int priorValue = ZAtomic_Add( &mAvailableBuffers, -1 );
			
			ZAssert( priorValue >= 0 );
			
			if ( priorValue == 0 ){

				mBufferAvailable.Wait( 1 );
				
				ZAssert( ZAtomic_Get( &mAvailableBuffers ) > 0 );
				
				priorValue = ZAtomic_Add( &mAvailableBuffers, -1 );
					
				ZAssert( priorValue > 0 );
				
			}
			
				
			Buffer &buf = *mNextBuffer++;
			
			if ( mNextBuffer == mBuffers.end() )
				mNextBuffer = mBuffers.begin();
			
			short *sample = buf.mSamples;

#if 0				
			for ( int i = 0; i < kBufferSize; ++i ){
				*sample++ = 32767.0 * sin( mSineWavePhase );
				*sample++ = 32767.0 * sin( mSineWavePhase );
				
				mSineWavePhase += 2 * 3.1415926 / 100.0;
			}
#else
			InputFIFOVector &inputs = GetInputs();
			
			inputs[ 0 ]->Read( mRawPCM, kBufferSize );
			inputs[ 1 ]->Read( mRawPCM + kBufferSize, kBufferSize );
			
			float *left = mRawPCM;
			float *right = mRawPCM + kBufferSize;
			
			for ( int i = 0; i < kBufferSize; ++i ){
				*sample++ = static_cast< short >( 32767.0 * *left++ );
				*sample++ = static_cast< short >( 32767.0 * *right++ );
			}
#endif
					
			memset( &buf.mHeader, 0, sizeof( buf.mHeader ) );
			
			// MSDN doc says the dwFlags must be set to WHDR_PREPARED,
			// but I don't think that's right			

			buf.mHeader.lpData = reinterpret_cast< LPSTR >( buf.mSamples );
			buf.mHeader.dwBufferLength = sizeof( buf.mSamples );

			HRESULT err = waveOutPrepareHeader( mDevice,
												&buf.mHeader,
												sizeof( buf.mHeader ) );	
			
			if ( err == MMSYSERR_NOMEM ){
			
				FreeBuffer();
				
				continue;
			}
			
			ZAssert( buf.mHeader.dwFlags == WHDR_PREPARED );
			
			ZAssert( err == MMSYSERR_NOERROR );
			
			err = waveOutWrite( mDevice,
								&buf.mHeader,
								sizeof( buf.mHeader ) );
								

			if ( err == MMSYSERR_NOMEM ){
			
				err = waveOutUnprepareHeader( mDevice,
												&buf.mHeader, 
												sizeof( buf.mHeader ) );
												
				ZAssert( ( err == MMSYSERR_NOERROR ) 
							|| ( err == MMSYSERR_NOMEM ) );
										
				FreeBuffer();
				continue;
			}
								
		
		}
		
	}while( true );
	return;
}


#endif // ZCONFIG( OS, Win32 )