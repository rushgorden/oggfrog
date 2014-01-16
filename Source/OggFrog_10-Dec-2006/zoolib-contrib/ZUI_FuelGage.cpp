// ZUI_.cpp

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

#include "ZUI_FuelGage.h"
#include <math.h>

ZUI_FuelGage::ZUI_FuelGage( int capacity, 
					int fillLevel, 
					ZSuperPane *inSuperPane, 
					ZPaneLocator *inPaneLocator )
	: ZSubPane( inSuperPane, inPaneLocator ),
		mCapacity( capacity ),
		mFillLevel( fillLevel ),
		mFillRatio( FillRatio() ),
		mOldWidth( 0 )
{
	return;
}

ZUI_FuelGage::~ZUI_FuelGage()
{
	return;
}

float ZUI_FuelGage::FillRatio() const
{
	if ( mCapacity != 0 )
		return static_cast< float >( mFillLevel ) / static_cast< float >( mCapacity );

	return mFillLevel == 0 ? 0.0 : 1.0;
}
		
		
void ZUI_FuelGage::SetFillLevel( int fillLevel, int capacity )
{
	ZCoord oldLevel = Level();
	
	mFillRatio = static_cast< float >( fillLevel ) / static_cast< float >( capacity );

	ZPoint size( GetSize() );
	
	if ( size.h != mOldWidth ){
		mOldWidth = size.h;
		Refresh();
	}
	
	ZCoord newLevel = Level();
	
	if ( newLevel == oldLevel )
		return;
		
	ZDCRgn invalRgn( ZDCRgn::sRect( min( newLevel, oldLevel ), 0,
									newLevel < oldLevel? oldLevel + 1 : newLevel,
									 size.v ) );
						
	Invalidate( invalRgn );
		
	return;
}

ZCoord ZUI_FuelGage::Level()
{
	return ( GetSize().h * mFillRatio ) + 0.5;
}

void ZUI_FuelGage::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
{
	ZDC myDC( inDC );
	
	ZPoint size( GetSize() );

	myDC.SetInk( ZDCInk::sGray );
	
	ZCoord level = Level();
	
	ZRect full( 0, 0, level, size.v );
	
	myDC.Fill( full );
	
	myDC.SetInk( ZDCInk::sWhite );
	
	ZRect empty( level + 1, 0, size.h, size.v );
	
	myDC.Fill( empty );

	myDC.SetInk( ZDCInk::sBlack );
		
	ZRect outLine( 0, 0, size.h, size.v );
	
	myDC.Frame( outLine );
	
	return;
}
