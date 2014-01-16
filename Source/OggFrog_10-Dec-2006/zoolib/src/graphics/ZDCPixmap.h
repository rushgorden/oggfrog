/*  @(#) $Id: ZDCPixmap.h,v 1.16 2006/04/26 22:31:48 agreen Exp $ */

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

#ifndef __ZDCPixmap__
#define __ZDCPixmap__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZDCPixmapNS.h"
#include "ZGeom.h"
#include "ZRefCount.h"
#include "ZRGBColor.h"

// For documentation, see ZDCPixmapNS.cpp

class ZDCPixmapCache;
class ZDCPixmapRep;
class ZDCPixmapRaster;

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapFactory

class ZDCPixmapFactory
	{
protected:
	ZDCPixmapFactory();
	~ZDCPixmapFactory();

public:
	static ZRef<ZDCPixmapRep> sCreateRep(const ZRef<ZDCPixmapRaster>& iRaster,
		const ZRect& iBounds, const ZDCPixmapNS::PixelDesc& iPixelDesc);

	static ZRef<ZDCPixmapRep> sCreateRep(const ZDCPixmapNS::RasterDesc& iRasterDesc,
		const ZRect& iBounds, const ZDCPixmapNS::PixelDesc& iPixelDesc);

	static ZDCPixmapNS::EFormatStandard sMapEfficientToStandard(
		ZDCPixmapNS::EFormatEfficient iFormat);

	virtual ZRef<ZDCPixmapRep> CreateRep(const ZRef<ZDCPixmapRaster>& iRaster,
		const ZRect& iBounds, const ZDCPixmapNS::PixelDesc& iPixelDesc) = 0;

	virtual ZRef<ZDCPixmapRep> CreateRep(const ZDCPixmapNS::RasterDesc& iRasterDesc,
		const ZRect& iBounds, const ZDCPixmapNS::PixelDesc& iPixelDesc) = 0;

	virtual ZDCPixmapNS::EFormatStandard MapEfficientToStandard(
		ZDCPixmapNS::EFormatEfficient iFormat) = 0;

private:
	static ZDCPixmapFactory* sHead;
	ZDCPixmapFactory* fNext;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmap

/// Stuff about ZDCPixmap

class ZDCPixmap
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZDCPixmap, operator_bool_generator_type, operator_bool_type);
public:
// The usual mantra: constructor, copy constructor, destructor, assignment
	ZDCPixmap();
	ZDCPixmap(const ZDCPixmap& other);
	~ZDCPixmap();
	ZDCPixmap& operator=(const ZDCPixmap& other);
	ZDCPixmap& operator=(const ZRef<ZDCPixmapRep>& iRep);

	/** This is the most general and flexible constructor, but it does require
	that client code create the ZDCPixmapRep, and thus that code has to deal with
	the details of ZDCPixmapRep's API. */
	ZDCPixmap(const ZRef<ZDCPixmapRep>& iRep);

	/** Temporary rescuscitation of old constructor. This method will be
	replaced with a more general composition API ASAP. */
	ZDCPixmap(const ZDCPixmap& iSource1, const ZDCPixmap& iSource2, const ZDCPixmap& iMask);


	/** Generally this constructor is the best to use. It takes an EFormatEfficient enum that
	specifies the depth and type of pixmap that's required, but makes no comment on the layout
	of the pixmap. Platform specific code does the work of interpreting what 'efficient'
	means for its purposes. */
	ZDCPixmap(ZPoint iSize, ZDCPixmapNS::EFormatEfficient iFormat);

	/** \overload */
	ZDCPixmap(ZPoint iSize, ZDCPixmapNS::EFormatEfficient iFormat, const ZRGBColorPOD& iFillColor);

	/** This constructor is a shorthand for creating a pixmap that has a known pixel layout, where
	that layout is specified by the EFormatStandard enum that is passed in. The pixmap *may* be the
	same as what the 'efficient' would create, but that will depend entirely on which platform the
	code executes on. If iFillColor is non-nil, the pixmap's contents will be initialized to that
	color, otherwise its contents will be undefined. */
	ZDCPixmap(ZPoint iSize, ZDCPixmapNS::EFormatStandard iFormat);

	/** \overload */
	ZDCPixmap(ZPoint iSize, ZDCPixmapNS::EFormatStandard iFormat, const ZRGBColorPOD& iFillColor);

	/** \overload */
	ZDCPixmap(const ZDCPixmapNS::RasterDesc& iRasterDesc,
		ZPoint iSize, const ZDCPixmapNS::PixelDesc& iPixelDesc);

	/** Initialized from a subset of a source. The actual bounds to be copied will be the
	intersection of \a iSourceBounds and the bounds of \a iSource. The new pixmap will most
	likely share iSource's raster, making this a very inexpensive operation, until a mutating
	operation occurs. */
	ZDCPixmap(const ZDCPixmap& iSource, const ZRect& iSourceBounds);

	/** Referencing pixvals and copying a color table. */
	ZDCPixmap(ZPoint iSize, const uint8* iPixvals,
		const ZRGBColorPOD* iColorTable, size_t iColorTableSize);

	/** Referencing pixVals and copying a color map. */
	ZDCPixmap(ZPoint iSize, const char* iPixvals,
		const ZRGBColorMap* iColorMap, size_t iColorMapSize);

	bool operator==(const ZDCPixmap& other) const;

	/** Are we valid, i.e. do we have any pixels at all? */
	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(fRep && true); }

/** \name Dimensions
	The number of pixels horizontally and vertically.
*/	//@{
	ZPoint Size() const;
	ZCoord Width() const;
	ZCoord Height() const;
	//@}

/** \name Access to pixels, by RGB.
*/	//@{
	ZRGBColorPOD GetPixel(const ZPoint& iLocation) const;
	ZRGBColorPOD GetPixel(ZCoord iLocationH, ZCoord iLocationV) const;

	void SetPixel(const ZPoint& iLocation, const ZRGBColorPOD& iColor);
	void SetPixel(ZCoord iLocationH, ZCoord iLocationV, const ZRGBColorPOD& iColor);
	//@}

/** \name Access to pixels, by pixel value (aka pixval.)
*/	//@{
	uint32 GetPixval(const ZPoint& iLocation) const;
	uint32 GetPixval(ZCoord iLocationH, ZCoord iLocationV) const;

	void SetPixval(const ZPoint& iLocation, uint32 iPixval);
	void SetPixval(ZCoord iLocationH, ZCoord iLocationV, uint32 iPixval);
	//@}

/** \name Blitting rectangles of pixels
*/	//@{
	void CopyFrom(ZPoint iDestLocation, const ZDCPixmap& iSource, const ZRect& iSourceBounds);

	void CopyFrom(ZPoint iDestLocation,
			const void* iSourceBaseAddress,
			const ZDCPixmapNS::RasterDesc& iSourceRasterDesc,
			const ZDCPixmapNS::PixelDesc& iSourcePixelDesc,
			const ZRect& iSourceBounds);

	void CopyTo(ZPoint iDestLocation,
			void* iDestBaseAddress,
			const ZDCPixmapNS::RasterDesc& iDestRasterDesc,
			const ZDCPixmapNS::PixelDesc& iDestPixelDesc,
			const ZRect& iSourceBounds) const;
	//@}

/** \name Applying a function to every pixel
*/	//@{
	void Munge(ZDCPixmapNS::MungeProc iMungeProc, void* iRefcon);
	void Munge(bool iMungeColorTable, ZDCPixmapNS::MungeProc iMungeProc, void* iRefcon);
	//@}

	/** Ensure the pixmap's representation is not shared. */
	void Touch();

	/** Access to our rep. */
	const ZRef<ZDCPixmapRep>& GetRep() const;

/** \name Access to other info
*/	//@{
	const ZRef<ZDCPixmapRaster>& GetRaster() const;

	const void* GetBaseAddress() const;
	void* GetBaseAddress();
	const ZDCPixmapNS::RasterDesc& GetRasterDesc() const;
	const ZDCPixmapNS::PixelDesc& GetPixelDesc() const;
	const ZRect& GetBounds() const;
	//@}

private:
	void pTouch();
	ZRef<ZDCPixmapRep> fRep;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRaster

class ZDCPixmapRaster : public ZRefCountedWithFinalization, ZooLib::NonCopyable
	{
protected:
	ZDCPixmapRaster(const ZDCPixmapNS::RasterDesc& iRasterDesc);

public:
	static bool sCheckAccessEnabled() { return false; }

	virtual ~ZDCPixmapRaster();

	void* GetBaseAddress() { return fBaseAddress; }
	bool GetCanModify() { return fCanModify; }
	const ZDCPixmapNS::RasterDesc& GetRasterDesc() { return fRasterDesc; }

	void Fill(uint32 iPixval);

	uint32 GetPixval(ZCoord iLocationH, ZCoord iLocationV);
	void SetPixval(ZCoord iLocationH, ZCoord iLocationV, uint32 iPixval);

protected:
	ZDCPixmapNS::RasterDesc fRasterDesc;
	bool fCanModify;
	void* fBaseAddress;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRaster_Simple

class ZDCPixmapRaster_Simple : public ZDCPixmapRaster
	{
public:
	ZDCPixmapRaster_Simple(const ZDCPixmapNS::RasterDesc& iRasterDesc);
	ZDCPixmapRaster_Simple(ZRef<ZDCPixmapRaster> iOther);
	virtual ~ZDCPixmapRaster_Simple();

protected:
	uint8* fBuffer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRaster_StaticData

class ZDCPixmapRaster_StaticData : public ZDCPixmapRaster
	{
public:
	ZDCPixmapRaster_StaticData(const void* iBaseAddress,
		const ZDCPixmapNS::RasterDesc& iRasterDesc);

	ZDCPixmapRaster_StaticData(const uint8* iBaseAddress, ZCoord iWidth, ZCoord iHeight);

	ZDCPixmapRaster_StaticData(const char* iBaseAddress, ZCoord iWidth, ZCoord iHeight);

	virtual ~ZDCPixmapRaster_StaticData();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapCache

class ZDCPixmapCache : public ZRefCountedWithFinalization
	{
protected:
	ZDCPixmapCache();
	ZDCPixmapCache(const ZDCPixmapCache& iOther);
	ZDCPixmapCache& operator=(const ZDCPixmapCache& iOther);

public:
	virtual ~ZDCPixmapCache();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRep

class ZDCPixmapRep : public ZRefCountedWithFinalization, ZooLib::NonCopyable
	{
protected:
	ZDCPixmapRep();

public:
	static bool sCheckAccessEnabled() { return false; }

	ZDCPixmapRep(const ZRef<ZDCPixmapRaster>& iRaster,
		const ZRect& iBounds, const ZDCPixmapNS::PixelDesc& iPixelDesc);

	virtual ~ZDCPixmapRep();

// Access to pixels
	ZRGBColorPOD GetPixel(ZCoord iLocationH, ZCoord iLocationV) const;
	void SetPixel(ZCoord iLocationH, ZCoord iLocationV, const ZRGBColorPOD& iColor);

	uint32 GetPixval(ZCoord iLocationH, ZCoord iLocationV);
	void SetPixval(ZCoord iLocationH, ZCoord iLocationV, uint32 iPixval);

	void CopyFrom(ZPoint iDestLocation,
		const ZRef<ZDCPixmapRep>& iSourceRep, const ZRect& iSourceBounds);

	void CopyFrom(ZPoint iDestLocation,
			const void* iSourceBaseAddress,
			const ZDCPixmapNS::RasterDesc& iSourceRasterDesc,
			const ZDCPixmapNS::PixelDesc& iSourcePixelDesc,
			const ZRect& iSourceBounds);

	void CopyTo(ZPoint iDestLocation,
			void* iDestBaseAddress,
			const ZDCPixmapNS::RasterDesc& iDestRasterDesc,
			const ZDCPixmapNS::PixelDesc& iDestPixelDesc,
			const ZRect& iSourceBounds);

	ZPoint GetSize();

	const ZRect& GetBounds();

	const ZDCPixmapNS::PixelDesc& GetPixelDesc();

	const ZRef<ZDCPixmapRaster>& GetRaster();

	const ZRef<ZDCPixmapCache>& GetCache();
	void SetCache(ZRef<ZDCPixmapCache> iCache);

	virtual ZRef<ZDCPixmapRep> Touch();

protected:
	ZRef<ZDCPixmapRaster> fRaster;
	ZRect fBounds;
	ZDCPixmapNS::PixelDesc fPixelDesc;
	ZRef<ZDCPixmapCache> fCache;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmap inline definitions

inline ZDCPixmap::ZDCPixmap()
	{}

inline ZDCPixmap::ZDCPixmap(const ZDCPixmap& other)
:	fRep(other.fRep)
	{}

inline ZDCPixmap::~ZDCPixmap()
	{}

inline ZDCPixmap& ZDCPixmap::operator=(const ZDCPixmap& other)
	{
	fRep = other.fRep;
	return *this;
	}

inline ZDCPixmap& ZDCPixmap::operator=(const ZRef<ZDCPixmapRep>& iRep)
	{
	fRep = iRep;
	return *this;
	}

inline ZDCPixmap::ZDCPixmap(const ZRef<ZDCPixmapRep>& iRep)
:	fRep(iRep)
	{}

inline ZRGBColorPOD ZDCPixmap::GetPixel(const ZPoint& iLocation) const
	{ return this->GetPixel(iLocation.h, iLocation.v); }

inline uint32 ZDCPixmap::GetPixval(const ZPoint& iLocation) const
	{ return this->GetPixval(iLocation.h, iLocation.v); }

inline void ZDCPixmap::SetPixel(const ZPoint& iLocation, const ZRGBColorPOD& iColor)
	{ this->SetPixel(iLocation.h, iLocation.v, iColor); }

inline void ZDCPixmap::SetPixval(const ZPoint& iLocation, uint32 iPixval)
	{ this->SetPixval(iLocation.h, iLocation.v, iPixval); }

inline void ZDCPixmap::Munge(ZDCPixmapNS::MungeProc iMungeProc, void* iRefcon)
	{ this->Munge(true, iMungeProc, iRefcon); }

inline const ZRef<ZDCPixmapRep>& ZDCPixmap::GetRep() const
	{ return fRep; }

inline const ZRef<ZDCPixmapRaster>& ZDCPixmap::GetRaster() const
	{ return fRep->GetRaster(); }

inline const void* ZDCPixmap::GetBaseAddress() const
	{ return fRep->GetRaster()->GetBaseAddress(); }

inline void* ZDCPixmap::GetBaseAddress()
	{ return fRep->GetRaster()->GetBaseAddress(); }

inline const ZDCPixmapNS::RasterDesc& ZDCPixmap::GetRasterDesc() const
	{ return fRep->GetRaster()->GetRasterDesc(); }

inline const ZDCPixmapNS::PixelDesc& ZDCPixmap::GetPixelDesc() const
	{ return fRep->GetPixelDesc(); }

inline const ZRect& ZDCPixmap::GetBounds() const
	{ return fRep->GetBounds(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRep inline definitions

inline const ZRect& ZDCPixmapRep::GetBounds()
	{ return fBounds; }

inline ZPoint ZDCPixmapRep::GetSize()
	{ return fBounds.Size(); }

inline const ZDCPixmapNS::PixelDesc& ZDCPixmapRep::GetPixelDesc()
	{ return fPixelDesc; }

inline const ZRef<ZDCPixmapRaster>& ZDCPixmapRep::GetRaster()
	{ return fRaster; }

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapCombo

class ZDCPixmapCombo
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZDCPixmapCombo,
    	operator_bool_generator_type, operator_bool_type);

public:
	ZDCPixmapCombo();

	ZDCPixmapCombo(const ZDCPixmap& iColorPixmap,
		const ZDCPixmap& iMonoPixmap,
		const ZDCPixmap& iMaskPixmap);

	ZDCPixmapCombo(const ZDCPixmap& iMainPixmap, const ZDCPixmap& iMaskPixmap);

	ZDCPixmapCombo& operator=(const ZDCPixmapCombo& iOther);

	bool operator==(const ZDCPixmapCombo& other) const;
	bool operator!=(const ZDCPixmapCombo& other) const
		{ return ! (*this == other); }

	void SetPixmaps(const ZDCPixmap& iColorPixmap,
		const ZDCPixmap& iMonoPixmap,
		const ZDCPixmap& iMaskPixmap);

	void SetPixmaps(const ZDCPixmap& iMainPixmap, const ZDCPixmap& iMaskPixmap);

	void GetPixmaps(ZDCPixmap& oColorPixmap, ZDCPixmap& oMonoPixmap, ZDCPixmap& oMaskPixmap) const;

	void SetColor(const ZDCPixmap& iPixmap) { fColorPixmap = iPixmap; }
	const ZDCPixmap& GetColor() const { return fColorPixmap; }

	void SetMono(const ZDCPixmap& iPixmap) { fMonoPixmap = iPixmap; }
	const ZDCPixmap& GetMono() const { return fMonoPixmap; }

	void SetMask(const ZDCPixmap& iPixmap) { fMaskPixmap = iPixmap; }
	const ZDCPixmap& GetMask() const { return fMaskPixmap; }

	ZPoint Size() const;

// Are we valid, i.e. do we have any pixels at all?
	operator operator_bool_type() const;

protected:
	ZDCPixmap fColorPixmap;
	ZDCPixmap fMonoPixmap;
	ZDCPixmap fMaskPixmap;
	};

// =================================================================================================

#endif // __ZDCPixmap__
