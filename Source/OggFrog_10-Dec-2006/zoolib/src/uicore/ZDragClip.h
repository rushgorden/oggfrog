/*  @(#) $Id: ZDragClip.h,v 1.6 2006/07/15 20:54:22 goingware Exp $ */

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

#ifndef __ZDragClip__
#define __ZDragClip__ 1
#include "zconfig.h"

// ==================================================

#include "ZDCRgn.h"
#include "ZTuple.h"
#include "ZTypes.h"

#include <string>

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <MacTypes.h>
#	include <Scrap.h>
#endif

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/MacTypes.h>
#	include <HIToolbox/Scrap.h>
#endif

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

// ==================================================

class ZClipboard
	{
public:
	static ZClipboard* sGetClipboard();

	ZClipboard();
	virtual ~ZClipboard();

	static ZTuple sGet();
	static void sSet(const ZTuple& iTuple);
	static void sClear();

private:
	ZTuple Get();
	void Set(const ZTuple& iTuple);
	void Clear();

#if ZCONFIG(OS, MacOS7)

	ZTuple fTuple;
	SInt16 fLastKnownScrapCount;

#elif ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	ZTuple fTuple;
	ScrapRef fLastKnownScrapRef;

#endif // ZCONFIG(OS)

	static ZClipboard* sClipboard;
	};

// ==================================================

class ZDragClipManager
	{
public:
	static ZDragClipManager* sGet();

	ZDragClipManager();
	virtual ~ZDragClipManager();

#if ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX) || ZCONFIG(OS, MacOS7)

	size_t Count();
	string At(size_t inIndex);
	void RegisterMIME(const string& iMIME, ScrapFlavorType iScrapFlavorType, bool iIsString);
	bool LookupScrapFlavorType(ScrapFlavorType iScrapFlavorType, string& oMIME, bool& oIsString);
	bool LookupMIME(const string& iMIME, ScrapFlavorType& oScrapFlavorType, bool& oIsString);

#elif ZCONFIG(OS, Win32)

	void RegisterMIME(const string& iMIME, bool iIsString);
	void RegisterMIME(const string& iMIME, CLIPFORMAT iCLIPFORMAT, bool iIsString);
	bool LookupCLIPFORMAT(CLIPFORMAT iCLIPFORMAT, string& oMIME, bool& oIsString);
	bool LookupMIME(const string& iMIME, CLIPFORMAT& oCLIPFORMAT, bool& oIsString);

#endif // ZCONFIG(OS)

protected:
	static ZDragClipManager* sDragClipManager;

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	struct MIMENative
		{
		string fMIME;
		ScrapFlavorType fScrapFlavorType;
		bool fIsString;
		};

#elif ZCONFIG(OS, Win32)

	struct MIMENative
		{
		string fMIME;
		CLIPFORMAT fCLIPFORMAT;
		bool fIsString;
		};

#else // ZCONFIG(OS)

	struct MIMENative
		{
		string fMIME;
		};

#endif // ZCONFIG(OS)

	vector<MIMENative> fMIMENativeVector;
	};

// ==================================================

class ZDragSource
	{
protected:
	ZDragSource();
	~ZDragSource();
public:
	};

// ==================================================

class ZDCPixmap;
class ZDragInitiator
	{
public:
	ZDragInitiator();
	virtual ~ZDragInitiator();

	ZDCRgn OutlineRgn(const ZDCRgn& iRgn);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn, ZDragSource* iDragSource);

	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap);
	void DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, ZDragSource* iDragSource);

	virtual void Imp_DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn* iDragRgn, const ZDCPixmap* iDragPixmap, const ZDCRgn* iDragMaskRgn, const ZDCPixmap* iDragMaskPixmap, ZDragSource* iDragSource) = 0;
	};

// ==================================================

// AG 99-01-12. IMPORTANT. Ultimately, ZDrag will have some different capabilities from ZDrop. It will continue to share the
// tuple and drag source access methods, but may also have an additional set of access methods that operate at the meta level,
// without requiring that the tuple be returned. Also, there is cursor setting and feedback management to deal with.
class ZDragDropBase
	{
public:
	virtual size_t CountTuples() const = 0;
	virtual ZTuple GetTuple(size_t iItemNumber) const = 0;
	virtual ZDragSource* GetDragSource() const = 0;
	virtual void GetNative(void* oNative) = 0;
	};

// ----------

class ZDrag: public ZDragDropBase
	{
public:
	};

// ----------

class ZDrop: public ZDragDropBase
	{
public:
	};

// ==================================================

class ZSubPane;
class ZDragDropHandler
	{
protected:
	ZDragDropHandler();
	~ZDragDropHandler();
public:
	virtual void HandleDrag(ZSubPane* iSubPane, ZPoint iPoint, ZMouse::Motion iMotion, ZDrag& iDrag);
	virtual void HandleDrop(ZSubPane* iSubPane, ZPoint iPoint, ZDrop& iDrop);
	};

// ==================================================

#endif // __ZDragClip__
