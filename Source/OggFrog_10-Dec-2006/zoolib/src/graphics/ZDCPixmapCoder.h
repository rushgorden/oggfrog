/*  @(#) $Id: ZDCPixmapCoder.h,v 1.5 2006/04/26 22:31:48 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZDCPixmapCoder__
#define __ZDCPixmapCoder__ 1
#include "zconfig.h"

#include "ZDCPixmap.h"
#include "ZStream.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapEncoder

class ZDCPixmapEncoder
	{
public:
	static void sWritePixmap(const ZStreamW& iStream,
		const ZDCPixmap& iPixmap, ZDCPixmapEncoder& iEncoder);

protected:
	ZDCPixmapEncoder();
	ZDCPixmapEncoder(const ZDCPixmapEncoder&);
	ZDCPixmapEncoder& operator=(const ZDCPixmapEncoder&);

public:
	virtual ~ZDCPixmapEncoder();

	void Write(const ZStreamW& iStream, const ZDCPixmap& iPixmap);
	void Write(const ZStreamW& iStream,
		const void* iBaseAddress,
		const ZDCPixmapNS::RasterDesc& iRasterDesc,
		const ZDCPixmapNS::PixelDesc& iPixelDesc,
		const ZRect& iBounds);

	/** API that must be overridden. */
	virtual void Imp_Write(const ZStreamW& iStream,
		const void* iBaseAddress,
		const ZDCPixmapNS::RasterDesc& iRasterDesc,
		const ZDCPixmapNS::PixelDesc& iPixelDesc,
		const ZRect& iBounds) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapDecoder

class ZDCPixmapDecoder
	{
public:
	static ZDCPixmap sReadPixmap(const ZStreamR& iStream, ZDCPixmapDecoder& iDecoder);

protected:
	ZDCPixmapDecoder();
	ZDCPixmapDecoder(const ZDCPixmapDecoder&);
	ZDCPixmapDecoder& operator=(const ZDCPixmapDecoder&);

public:
	virtual ~ZDCPixmapDecoder();

	ZDCPixmap Read(const ZStreamR& iStream);

	/** API that must be overridden. */
	virtual void Imp_Read(const ZStreamR& iStream, ZDCPixmap& oPixmap) = 0;

	/** \todo. Need an API that reads into a subset only. */
	};

#endif // __ZDCPixmapCoder__
