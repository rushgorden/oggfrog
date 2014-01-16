/*  @(#) $Id: ZDCRgn.h,v 1.17 2006/10/19 22:58:38 agreen Exp $ */

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

#ifndef __ZDCRgn__
#define __ZDCRgn__ 1
#include "zconfig.h"

#include "ZGeom.h"
#include "ZRefCount.h"
#include <vector>

#if ZCONFIG(API_Graphics, QD)
#	if ZCONFIG(OS, MacOSX)
#		include <QD/QuickDraw.h>
#	else
#		include <QuickDraw.h>
#	endif
#endif

#if ZCONFIG(API_Graphics, GDI)
#	include "ZWinHeader.h"
#endif

#if ZCONFIG(API_Graphics, X)
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#	ifdef Bool
#		undef Bool
#	endif
#endif

#if ZCONFIG(API_Graphics, Be)
#	include <interface/Region.h>
#endif

#if ZCONFIG(API_Graphics, ZooLib)
#	include "ZBigRegion.h"
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZDCRgn

class ZDCPoly;

class ZDCRgn
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZDCRgn, operator_bool_generator_type, operator_bool_type);

	struct Rep : public ZRefCounted
		{
		static inline bool sCheckAccessEnabled() { return false; }

		union
			{
			#if ZCONFIG(API_Graphics, QD)
			RgnHandle fRgnHandle;
			#endif

			#if ZCONFIG(API_Graphics, GDI)
			HRGN fHRGN;
			#endif

			#if ZCONFIG(API_Graphics, X)
			Region fRegion;
			#endif

			#if ZCONFIG(API_Graphics, Be)
			BRegion* fBRegion;
			#endif

			#if ZCONFIG(API_Graphics, ZooLib)
			ZBigRegion* fBigRegion;
			#endif
			};

		#if ZCONFIG_API_Graphics_Multi
		long fFormat;
		#endif

		Rep() {}
		virtual ~Rep();
		};

	#if ZCONFIG_API_Graphics_Multi
	void Internal_Harmonize(const ZDCRgn& iOther)  const;
	void Internal_ChangeFormat(short iFormat) const;
	void Internal_TouchAndSetFormat(short iRequiredFormat);
	#endif

	void Internal_Touch();
	void Internal_TouchReal();

	static void sInternal_CreateRep_Rect(ZRef<Rep>& oRep,
		ZCoord iLeft, ZCoord iTop, ZCoord iRight, ZCoord iBottom);

	static void sInternal_CreateRep_RoundRect(ZRef<Rep>& oRep,
		ZCoord iLeft, ZCoord iTop, ZCoord iRight, ZCoord iBottom,
		ZCoord iCornerSizeH, ZCoord iCornerSizeV);

	ZRef<Rep> fRep;

public:
// Static methods creating regions from other representations
// Line -- actually a six sided polygon like a Mac pen
	static ZDCRgn sLine(ZCoord iStartH, ZCoord iStartV,
		ZCoord iEndH, ZCoord iEndV, ZCoord iLineWidth);

	static ZDCRgn sLine(const ZPoint& iStart, const ZPoint& iEnd, ZCoord iLineWidth)
		{ return sLine(iStart.h, iStart.v, iEnd.h, iEnd.v, iLineWidth); }

// Rect
	static ZDCRgn sRect(const ZRect& iRect);
	static ZDCRgn sRect(ZCoord iLeft, ZCoord iTop, ZCoord iRight, ZCoord iBottom);
	static ZDCRgn sRects(const ZRect* iRects, size_t iCount, bool iAlreadySorted);

// Round rect
	static ZDCRgn sRoundRect(const ZRect& iRect, const ZPoint& iCornerSize);

	static ZDCRgn sRoundRect(ZCoord iLeft, ZCoord iTop,
		ZCoord iRight, ZCoord iBottom, ZCoord iCornerSizeH, ZCoord iCornerSizeV);

// Ellipse
	static ZDCRgn sEllipse(const ZRect& iBounds);
	static ZDCRgn sEllipse(ZCoord iLeft, ZCoord iTop, ZCoord iRight, ZCoord iBottom);

// Polygon
	static ZDCRgn sPoly(const ZDCPoly& iPoly, bool iEvenOdd, ZCoord iOffsetH, ZCoord iOffsetV);

	static ZDCRgn sPoly(const ZDCPoly& iPoly, ZCoord iOffsetH, ZCoord iOffsetV)
		{ return sPoly(iPoly, true, iOffsetH, iOffsetV); }

	static ZDCRgn sPoly(const ZDCPoly& iPoly, bool iEvenOdd)
		{ return sPoly(iPoly, iEvenOdd, 0, 0); }

	static ZDCRgn sPoly(const ZDCPoly& iPoly)
		{ return sPoly(iPoly, true, 0, 0); }

	static ZDCRgn sPoly(const ZPoint* iPoints, size_t iCount, bool iEvenOdd,
		ZCoord iOffsetH, ZCoord iOffsetV);

	static ZDCRgn sPoly(const ZPoint* iPoints, size_t iCount, ZCoord iOffsetH, ZCoord iOffsetV)
		{ return sPoly(iPoints, iCount, true, iOffsetH, iOffsetV); }

	static ZDCRgn sPoly(const ZPoint* iPoints, size_t iCount, bool iEvenOdd)
		{ return sPoly(iPoints, iCount, iEvenOdd, 0, 0); }

	static ZDCRgn sPoly(const ZPoint* iPoints, size_t iCount)
		{ return sPoly(iPoints, iCount, true, 0, 0); }

// ctor/dtor
	ZDCRgn() {}
	~ZDCRgn() {}

	ZDCRgn(const ZDCRgn& iOther) : fRep(iOther.fRep) {}
	ZDCRgn& operator=(const ZDCRgn& iOther) { fRep = iOther.fRep; return *this; }

// Convenience construction, assignment from rects
	ZDCRgn(const ZRect& iRect);
	ZDCRgn(ZCoord iLeft, ZCoord iTop, ZCoord iRight, ZCoord iBottom);
	ZDCRgn& operator=(const ZRect& iRect);

	#if ZCONFIG(API_Graphics, QD)
	ZDCRgn(RgnHandle iRgnHandle, bool iAdopt);
	ZDCRgn& operator=(RgnHandle iRgnHandle);
	RgnHandle GetRgnHandle();
	#endif

	#if ZCONFIG(API_Graphics, GDI)
	ZDCRgn(HRGN iHRGN, bool iAdopt);
	ZDCRgn& operator=(HRGN iHRGN);
	HRGN GetHRGN();
	#endif

	#if ZCONFIG(API_Graphics, X)
	ZDCRgn(Region iRegion, bool iAdopt);
	ZDCRgn& operator=(Region iRegion);
	Region GetRegion();
	#endif

	#if ZCONFIG(API_Graphics, Be)
	ZDCRgn(BRegion* iBRegion, bool iAdopt);
	ZDCRgn& operator=(BRegion* iBRegion);
	BRegion* GetBRegion();
	#endif

// Scaling, required on Windows, could be implemented elsewhere if essential.
	#if ZCONFIG(API_Graphics, GDI)
	void MakeScale(float m11, float m12, float m21, float m22, float hOffset, float vOffset);
	ZDCRgn Scale(float m11, float m12, float m21, float m22, float hOffset, float vOffset) const
		{
		ZDCRgn temp(*this);
		temp.MakeScale(m11, m12, m21, m22, hOffset, vOffset);
		return temp;
		}
	#endif

// Ensure that our underlying rgn is not shared.
	void Touch() { this->Internal_Touch(); }

// Procedural API
	bool Contains(const ZPoint& iPoint) const { return this->Contains(iPoint.h, iPoint.v); }
	bool Contains(ZCoord iH, ZCoord iV) const;

	void MakeOutline();
	ZDCRgn Outline() const;

	ZDCRgn Inset(const ZPoint& iPoint) const { return this->Inset(iPoint.h, iPoint.v); }
	ZDCRgn Inset(ZCoord iH, ZCoord iV) const
		{
		ZDCRgn tempRgn(*this);
		tempRgn.MakeInset(iH, iV);
		return tempRgn;
		}

	void MakeInset(const ZPoint& iPoint) { this->MakeInset(iPoint.h, iPoint.v); }
	void MakeInset(ZCoord iH, ZCoord iV);

	void Clear();
	bool IsEmpty() const;
	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(!this->IsEmpty()); }

	ZRect Bounds() const;
	bool IsSimpleRect() const;

	void Decompose(std::vector<ZRect>& oRects) const;
	typedef bool (*DecomposeProc)(const ZRect& iRect, void* iRefcon);
	long Decompose(DecomposeProc iProc, void* iRefcon) const;

	bool IsEqualTo(const ZDCRgn& iOther) const;

	ZDCRgn& OffsetBy(ZCoord iOffsetH, ZCoord iOffsetV);
	ZDCRgn& OffsetBy(const ZPoint& iOffset)
		{ return this->OffsetBy(iOffset.h, iOffset.v); }

	ZDCRgn& OffsetByNegative(ZCoord iOffsetH, ZCoord iOffsetV);
	ZDCRgn& OffsetByNegative(const ZPoint& iOffset)
		{ return this->OffsetByNegative(iOffset.h, iOffset.v); }

	ZDCRgn& Include(const ZRect& iRect);
	ZDCRgn& Intersect(const ZRect& iRect);
	ZDCRgn& Exclude(const ZRect& iRect);

	ZDCRgn& Include(const ZDCRgn& iOther);
	ZDCRgn& Intersect(const ZDCRgn& iOther);
	ZDCRgn& Exclude(const ZDCRgn& iOther);
	ZDCRgn& Xor(const ZDCRgn& iOther);

// Equality
	bool operator==(const ZDCRgn& iOther) const
		{ return this->IsEqualTo(iOther); }
	bool operator!=(const ZDCRgn& iOther) const
		{ return ! this->IsEqualTo(iOther); }

// Algrebraic API
	ZDCRgn& operator+=(const ZPoint& iOffset)
		{ return this->OffsetBy(iOffset.h, iOffset.v); }
	ZDCRgn operator+(const ZPoint& iOffset) const
		{ return ZDCRgn(*this).OffsetBy(iOffset.h, iOffset.v); }

	ZDCRgn& operator-=(const ZPoint& iOffset)
		{ return this->OffsetByNegative(iOffset.h, iOffset.v); }
	ZDCRgn operator-(const ZPoint& iOffset) const
		{ return ZDCRgn(*this).OffsetByNegative(iOffset.h, iOffset.v); }

// Combining with rectangles
	ZDCRgn& operator|=(const ZRect& iRect)
		{ return this->Include(iRect); }
	ZDCRgn operator|(const ZRect& iRect) const
		{ return ZDCRgn(*this).Include(iRect); }

	ZDCRgn& operator&=(const ZRect& iRect)
		{ return this->Intersect(iRect); }
	ZDCRgn operator&(const ZRect& iRect) const
		{ return ZDCRgn(*this).Intersect(iRect); }

	ZDCRgn& operator-=(const ZRect& iRect)
		{ return this->Exclude(iRect); }
	ZDCRgn operator-(const ZRect& iRect) const
		{ return ZDCRgn(*this).Exclude(iRect); }

// Combining with regions
	ZDCRgn& operator|=(const ZDCRgn& iOther)
		{ return this->Include(iOther); }
	ZDCRgn operator|(const ZDCRgn& iOther) const
		{ return ZDCRgn(*this).Include(iOther); }

	ZDCRgn& operator&=(const ZDCRgn& iOther)
		{ return this->Intersect(iOther); }
	ZDCRgn operator&(const ZDCRgn& iOther) const
		{ return ZDCRgn(*this).Intersect(iOther); }

	ZDCRgn& operator-=(const ZDCRgn& iOther)
		{ return this->Exclude(iOther); }
	ZDCRgn operator-(const ZDCRgn& iOther) const
		{ return ZDCRgn(*this).Exclude(iOther); }

	ZDCRgn& operator^=(const ZDCRgn& iOther)
		{ return this->Xor(iOther); }
	ZDCRgn operator^(const ZDCRgn& iOther) const
		{ return ZDCRgn(*this).Xor(iOther); }
	};

// Inline accessor if we're building for QD and no other graphics API is active.
#if !ZCONFIG_API_Graphics_Multi && ZCONFIG(API_Graphics, QD) && !OPAQUE_TOOLBOX_STRUCTS
inline ZRect ZDCRgn::Bounds() const
	{
	return fRep ? ZRect(fRep->fRgnHandle[0]->rgnBBox) : ZRect::sZero;
	}
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZDCRgnAccumulator

class ZDCRgnAccumulator
	{
public:
	ZDCRgnAccumulator();
	~ZDCRgnAccumulator();

	void Add(const ZDCRgn& iRgn);
	ZDCRgn GetResult() const;

private:
	std::vector<ZDCRgn> fStack;
	size_t fCount;
	};

// =================================================================================================

#endif // __ZDCRgn__
