/*  @(#) $Id: ZDCPixmapBlit.h,v 1.3 2006/04/26 22:31:48 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2005 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZDCPixmapBlit__
#define __ZDCPixmapBlit__ 1
#include "zconfig.h"

#include "ZDCPixmapNS.h"

// =================================================================================================

/**
There are a lot of parameters to many of the methods in ZDCPixmapBlit. In
order to reduce the visual load we use the following abbreviations:
RD = RasterDesc
PD = PixelDesc
B = Bounds
*/

namespace ZDCPixmapBlit {

typedef ZDCPixmapNS::RasterDesc RD;
typedef ZDCPixmapNS::PixelDesc PD;

enum EOp
	{
	eOp_Copy,
	eOp_Over,
	eOp_In,
	eOp_Plus
	};

/** Replicate iSourceB over iDestB, aligning iSourceOrigin with iDestB.TopLeft(). */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	ZPoint iSourceOrigin,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	EOp iOperation);

/** Copy source to iDestB without replication. The actual rectangle drawn
will be smaller than iDestB if iSourceB is smaller. */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	EOp iOperation);

/** Replicate iSourceB, with replicated matte. */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	ZPoint iSourceOrigin,
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	ZPoint iMatteOrigin,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	bool iSourcePremultiplied, EOp iOperation);

/** Replicate iSourceB, matted. */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	ZPoint iSourceOrigin,
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	bool iSourcePremultiplied, EOp iOperation);

/** Draw iSourceB into iDestB, with replicated matte. */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	ZPoint iMatteOrigin,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	bool iSourcePremultiplied, EOp iOperation);

/** Draw iSourceB masked by iMatteB into iDestB. */
void sBlit(
	const void* iSource, const RD& iSourceRD, const ZRect& iSourceB, const PD& iSourcePD,
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	bool iSourcePremultiplied, EOp iOperation);

/** Fill iDestB with iColor. */
void sColor(
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	const ZRGBColor& iColor,
	EOp iOperation);

/** Fill iDestB with iColor, matted. */
void sColor(
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	const ZRGBColor& iColor,
	EOp iOperation);

/** Fill iDestB with iColor, with replicated matte. */
void sColor(
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	ZPoint iMatteOrigin,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD,
	const ZRGBColor& iColor,
	EOp iOperation);

/** Invert, replacing each pixel with white minus that pixel. */
void sInvert(void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD);

/** Multiply r,g, b & alpha by iAmount/65536. */
void sOpaque(
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD, uint16 iAmount);

/** Multiply r, g, b by iAmount/65536, leaving alpha alone. */
void sDarken(
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD, uint16 iAmount);

/** Multiply alpha by iAmount/65536, leaving r,g,b alone. */
void sFade(
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD, uint16 iAmount);

/** Take the alpha channel of matte, store it in alpha channel of dest,
pre-multiplying r,g,b of dest as it does so. */
void sApplyMatte(
	const void* iMatte, const RD& iMatteRD, const ZRect& iMatteB, const PD& iMattePD,
	void* iDest, const RD& iDestRD, const ZRect& iDestB, const PD& iDestPD);

} // namespace ZDCPixmapBlit

#endif // __ZDCPixmapBlit__
