// ZPCMSink.cpp

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

#include "ZPCMSink.h"
#include "ZPCMSink_CoreAudio.h"
#include "ZPCMSink_WinWaveOut.h"

ZPCMSink::ZPCMSink( FIFOVector &inFIFOs )
	: ZRefCounted(),
		mInputs()
{
	for ( FIFOVector::iterator it( inFIFOs.begin() );
			it != inFIFOs.end();
			++it ){
		mInputs.push_back( it->GetObject() );
	}
	
	return;
}

ZPCMSink::~ZPCMSink()
{
	return;
}
			
ZRef< ZPCMSink >ZPCMSink:: sCreateOutput( FIFOVector &inFIFOs )
{
#if ZCONFIG( OS, Carbon ) || ZCONFIG( OS, MacOSX )

	return ZRef< ZPCMSink >( new ZPCMSink_CoreAudio( inFIFOs ) );
	
#elif ZCONFIG( OS, Win32 )

	return ZRef< ZPCMSink >( new ZPCMSink_WinWaveOut( inFIFOs ) );
	
#else
#error Need sound output
#endif
}

#if 0	
ZPCMSink::InputFIFOVector ZPCMSink::GetInputs( int inChannels )
{
	InputFIFOVector result;

	OFApp &app = OFApp::sGet();
	
	ZLocker locker( app.GetLock() );

	for ( int i = 0; i < inChannels; ++i ){
		result.push_back( &app.FIFO( i ) );
	}
	
	return result;
}
#endif