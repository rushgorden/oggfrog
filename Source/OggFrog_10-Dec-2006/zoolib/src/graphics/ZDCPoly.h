/*  @(#) $Id: ZDCPoly.h,v 1.4 2006/04/26 22:31:49 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2000 Andrew Green and Learning in Motion, Inc.
http://www.zoolib.org

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

#ifndef __ZDCPoly__
#define __ZDCPoly__
#include "zconfig.h"

#include "ZGeom.h"

#include <vector>

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPoly

class ZDCPoly
	{
public:
	ZDCPoly();
	ZDCPoly(const ZPoint& inStartPoint);
	ZDCPoly(ZCoord inPointH, ZCoord inPointV);
	ZDCPoly(const ZDCPoly& inOther);
	~ZDCPoly();

	ZDCPoly& operator=(const ZDCPoly& inOther);

	void Add(const ZPoint& inPoint);
	void Add(ZCoord iPointH, ZCoord iPointV) { this->Add(ZPoint(iPointH, iPointV)); }

	ZDCPoly operator+(const ZPoint& iPoint) const;
	ZDCPoly operator-(const ZPoint& iPoint) const;
	ZDCPoly& operator+=(const ZPoint& iPoint);
	ZDCPoly& operator-=(const ZPoint& iPoint);

	void GetPoints(const ZPoint& iOrigin, ZPoint*& oPoints, size_t& oCount) const;

#if ZCONFIG(API_Graphics, QD)
	PolyHandle GetPolyHandle(const ZPoint& iOrigin) const;
#endif

#if ZCONFIG(API_Graphics, GDI)
	void GetPOINTs(const ZPoint& iOrigin, POINT*& oPOINTs, size_t& oCount) const;
#endif

#if ZCONFIG(API_Graphics, X)
	void GetXPoints(const ZPoint& iOrigin, XPoint*& oXPoints, size_t& oCount) const;
#endif

#if ZCONFIG(API_Graphics, Be)
	void GetBPoints(const ZPoint& iOrigin, BPoint*& oBPoints, size_t& oCount) const;
#endif

// Decomposition of a ZDCPoly or list of ZPoints into a list of rectangles
	typedef bool (*DecomposeProc)(ZCoord iTopCoord, ZCoord iBottomCoord,
		ZCoord* iCoords, size_t iCoordsCount, void* iRefcon);

	void Decompose(bool iEvenOdd, ZCoord iOffsetH, ZCoord iOffsetV,
		DecomposeProc iDecomposeProc, void* iRefcon) const;

	static void sDecompose(const ZPoint* iPoints, size_t iCount, bool iEvenOdd,
		ZCoord iOffsetH, ZCoord iOffsetV,
		DecomposeProc iDecomposeProc, void* iRefcon);

private:
	void pEmptyCache();

	std::vector<ZPoint> fPoints;
	ZPoint fOrigin;

	ZPoint fCachedOrigin;
	ZPoint* fCachedPoints;

#if ZCONFIG(API_Graphics, QD)
	PolyHandle fCachedPolyHandle;
#endif

#if ZCONFIG(API_Graphics, GDI)
	POINT* fCachedPOINTs;
#endif

#if ZCONFIG(API_Graphics, X)
	XPoint* fCachedXPoints;
#endif

#if ZCONFIG(API_Graphics, Be)
	BPoint* fCachedBPoints;
#endif
	};

#endif // __ZDCPoly__
