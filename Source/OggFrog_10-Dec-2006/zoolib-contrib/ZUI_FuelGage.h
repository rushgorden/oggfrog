// ZUI_FuelGage.h

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
 
#ifndef __ZUI_FUELGAGE__
#define __ZUI_FUELGAGE__

#include "ZPane.h"

class ZUI_FuelGage: public ZSubPane
{
	public:
	
		ZUI_FuelGage( int capacity, int fillLevel, ZSuperPane *inSuperPane, ZPaneLocator *inPaneLocator );
		virtual ~ZUI_FuelGage();
		
		void SetFillLevel( int fillLevel, int capacity );
		void SetFillLevel( int fillLevel ) { SetFillLevel( fillLevel, mCapacity ); }

		virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
		
	public:
	
		int		mCapacity;
		int		mFillLevel;
		float	mFillRatio;
		ZCoord	mOldWidth;
		
		float FillRatio() const;
		ZCoord Level();
};

#endif // __ZUI_FUELGAGE__


