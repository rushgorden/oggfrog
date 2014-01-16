// ZPCMSink_Null.cpp

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

#include "ZPCMSink_Null.h"
#include "ZThreadSimple.h"

ZPCMSink_Null::ZPCMSink_Null( FIFOVector &inFIFOs )
	: ZPCMSink( inFIFOs ),
		mRawPCM( new float[ kBufferSize * 2 ] ),
		mStartSignal( 0, "ZPCMSink_Null semaphore" )
{
	mStop.fValue = true;
	mTerminate.fValue = false;
	
	ZThreadSimple< ZPCMSink_Null& > *thread = 
		new ZThreadSimple< ZPCMSink_Null& >( OutputThread,
													*this,
													"Null Output" );
	thread->Start(); 	

	return;
}

ZPCMSink_Null::~ZPCMSink_Null()
{
	ZAtomic_Set( &mTerminate, true );

	return;
}
		
void ZPCMSink_Null::Start()
{
	if ( ZAtomic_Get( &mStop ) == false )
		return;
		
	ZAtomic_Set( &mStop, false );
	
	mStartSignal.Signal( 1 );
	
	return;
}

void ZPCMSink_Null::Stop()
{
	ZAtomic_Set( &mStop, true );
	
	return;
}

bool ZPCMSink_Null::Stopped() const
{
	return ZAtomic_Get( &mStop );
}

void ZPCMSink_Null::OutputThread( ZPCMSink_Null &me )
{
	me.OutputThread();
	
	return;
}

void ZPCMSink_Null::OutputThread()
{

	//SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	do{
	
		if ( ZThread::errorNone != mStartSignal.Wait( 1 ) )
			return;
		
		while( !ZAtomic_Get( &mStop ) ){
			
			if ( ZAtomic_Get( &mTerminate ) )
				return;

			InputFIFOVector &inputs = GetInputs();
			
			inputs[ 0 ]->Read( mRawPCM, kBufferSize );
			inputs[ 1 ]->Read( mRawPCM + kBufferSize, kBufferSize );
			
			ZThread::sSleep( 44 );
		}		
	}while( true );
	return;
}
