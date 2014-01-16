#ifndef __PainterTest_Window__
#define __PainterTest_Window__ 1

#include "zconfig.h"

#include "ZFile.h"
#include "ZWindow.h"

#include "NPainter.h"

class ZApp;
class ZAsset;
class ZUIScrollPane;
class ZUIScrollBar;

class PainterTest_PaintPane;

class PainterTest_Window : public ZWindow,
					public ZPaneLocator
	{
public:
	static ZOSWindow* sCreateOSWindow(ZApp* inApp);

	PainterTest_Window(ZApp* inApp, const ZAsset& iAsset, ZRef<NPaintEngine::PaintState> inPaintState, ZRef<NPaintDataRep> iPaintDataRep, const ZFileSpec& iFileSpec, const string& iFormat);
	virtual ~PainterTest_Window();
	
	ZFileSpec GetFileSpec() { return fFileSpec; }

// From ZPaneLocator
	virtual bool GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk);
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);

// From ZEventHr via ZWindow
	virtual void DoSetupMenus(ZMenuSetup* inMenuSetup);
	virtual bool DoMenuMessage(const ZMessage& inMessage);

// From ZWindow
	virtual bool WindowQuitRequested();
	virtual void WindowCloseByUser();

private:
	bool pDoClose();
	void pDoSave();

protected:
	ZFileSpec fFileSpec;
	string fFormat;
	long fChangeCount;

	ZWindowPane* fWindowPane;
		ZUIScrollPane* fScrollPane;
			PainterTest_PaintPane* fPaintPane;
		ZUIScrollBar* fScrollBar_H;
		ZUIScrollBar* fScrollBar_V;
	};


#endif // __PainterTest_Window__
