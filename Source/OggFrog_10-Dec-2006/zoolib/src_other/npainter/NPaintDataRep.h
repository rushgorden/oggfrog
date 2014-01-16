#ifndef __NPaintDataRep__
#define __NPaintDataRep__ 1
#include "zconfig.h"

#include "ZDCFont.h"
#include "ZDCPixmap.h"
#include "ZDCRgn.h"

class ZDC;
class ZStreamR;
class ZStreamW;

class NPaintEngine;

// =================================================================================================
#pragma mark -
#pragma mark * NPaintDataRep

/* AG 1999-10-25. NPaintData holds data that can be passed to an NPainter for editing, or
retrieved from an NPainter for storage. It can be in one of three states:
1) Newly initialized, or subsequent to a call of FromStream that failed to parse. GetSize in this
case will return (0,0) and HasData will return false.
2) In a good condition, either because it was set up by NPainter::GetData, or FromStream was
called succesfully. GetSize returns the size of the painting and HasData returns true.
3) In poor condition, which will generally occur when FromStream has been called and the
stream contained data that is in a later format than the code knows how to parse. GetSize will
return the overall size of the painting (because we shove that size into a stream very early on),
but HasData will return false */

class NPaintDataRep : public ZRefCountedWithFinalization
	{
public:
	NPaintDataRep();
	NPaintDataRep(const NPaintDataRep* inOther);
	NPaintDataRep(const ZDCPixmap& iPixmap);
	NPaintDataRep(ZPoint inSize);
	NPaintDataRep(const ZRect& inMaxBounds, const string& inString);
	virtual ~NPaintDataRep();

	void FromStream(const ZStreamR& inStream);
	void FromStream_Old(const ZStreamR& inStream);

	void ToStream(const ZStreamW& inStream);

	void Draw(const ZDC& inDC, ZPoint inLocation);
	void DrawPixmap(const ZDC& inDC, ZPoint inLocation);

	bool HasData();

	ZPoint GetSize();

	void FromStream_BMP(const ZStreamR& inStream);
	void ToStream_BMP(const ZStreamW& inStream);

#if ZCONFIG(API_Graphics, QD)
	void FromStream_PICT(const ZStreamR& inStream);
	void ToStream_PICT(const ZStreamW& inStream);
#endif // ZCONFIG(API_Graphics, QD)

	const ZDCPixmap& GetPixmap();

protected:
	ZDCRgn fRegion;
	ZPoint fOffset;
	ZDCPixmap fPixmap;
	vector<string> fStrings; 
	vector<ZDCFont> fFonts;
	vector<ZRect> fBounds;
	vector<ZRGBColor> fColors;

	friend class NPaintEngine;
	};

#endif // __NPaintDataRep__
