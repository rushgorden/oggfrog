/*  @(#) $Id: ZDCInk.h,v 1.7 2006/04/26 22:31:48 agreen Exp $ */

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

#ifndef __ZDCInk__
#define __ZDCInk__ 1
#include "zconfig.h"

#include "ZRefCount.h"
#include "ZRGBColor.h"
#include "ZTypes.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPattern

class ZDCPattern
	{
public:
	uint8 pat[8];

	static const ZDCPattern sBlack;
	static const ZDCPattern sDkGray;
	static const ZDCPattern sGray;
	static const ZDCPattern sLtGray;
	static const ZDCPattern sWhite;

	static const ZDCPattern sBackOnly;
	static const ZDCPattern sForeOnly;

	static void sOffset(int32 inOffsetH, int32 inOffsetV, const uint8* inSource, uint8* inDest);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCInk

class ZDCInkRep;
class ZDCPixmap;

class ZDCInk
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZDCInk, operator_bool_generator_type, operator_bool_type);
public:
	enum Type { eTypeSolidColor, eTypeTwoColor, eTypeMultiColor };
	typedef ZDCInkRep Rep;

public:
	static const ZDCInk sBlack;
	static const ZDCInk sWhite;
	static const ZDCInk sGray;
	static const ZDCInk sDkGray;
	static const ZDCInk sLtGray;

	ZDCInk();
	ZDCInk(const ZDCInk& inOther);
	~ZDCInk();
	ZDCInk& operator=(const ZDCInk& inOther);

	ZDCInk(const ZRGBColorPOD& inSolidColor);

	ZDCInk(const ZRGBColorPOD& inForeColor,
		const ZRGBColorPOD& inBackColor, const ZDCPattern& inPattern);

	ZDCInk(const ZDCPixmap& inPixmap);

	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(fRep && true); }

	bool IsSameAs(const ZDCInk& inOther) const;

	const ZRef<Rep>& GetRep() const;

private:
	bool Internal_IsSameAs(const ZDCInk& inOther) const;

	ZRef<ZDCInkRep> fRep;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZDCInkRep

class ZDCInkRep : public ZRefCounted
	{
private:
	ZDCInkRep() {}

public:
	static bool sCheckAccessEnabled() { return false; }

	int fType;
	union
		{
		struct
			{
			ZRGBColorPOD fColor;
			} fAsSolidColor;
		struct
			{
			ZRGBColorPOD fForeColor;
			ZRGBColorPOD fBackColor;
			ZDCPattern fPattern;
			} fAsTwoColor;
		struct
			{
			ZDCPixmap* fPixmap;
			} fAsMultiColor;
		};

	ZDCInkRep(const ZDCInkRep& inOther);

	ZDCInkRep(const ZRGBColorPOD& inSolidColor);

	ZDCInkRep(const ZRGBColorPOD& inForeColor,
		const ZRGBColorPOD& inBackColor, const ZDCPattern& inPattern);

	ZDCInkRep(const ZDCPixmap& inPixmap);

	virtual ~ZDCInkRep();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCInk Inlines

inline ZDCInk::ZDCInk()
	{}

inline ZDCInk::ZDCInk(const ZDCInk& inOther)
:	fRep(inOther.fRep)
	{}

inline ZDCInk::~ZDCInk() {}

inline ZDCInk& ZDCInk::operator=(const ZDCInk& inOther)
	{
	fRep = inOther.fRep;
	return *this;
	}
inline ZDCInk::ZDCInk(const ZRGBColorPOD& inSolidColor)
:	fRep(new ZDCInkRep(inSolidColor))
	{}

inline ZDCInk::ZDCInk(const ZRGBColorPOD& inForeColor,
	const ZRGBColorPOD& inBackColor, const ZDCPattern& inPattern)
:	fRep(new ZDCInkRep(inForeColor, inBackColor, inPattern))
	{}

inline bool ZDCInk::IsSameAs(const ZDCInk& inOther) const
	{ return fRep == inOther.fRep || this->Internal_IsSameAs(inOther); }

inline const ZRef<ZDCInkRep>& ZDCInk::GetRep() const
	{ return fRep; }

#endif // __ZDCInk__
