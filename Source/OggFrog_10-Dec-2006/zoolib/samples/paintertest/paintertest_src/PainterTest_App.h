#ifndef __PainterTest_App__
#define __PainterTest_App__ 1

#include "ZApp.h"
#include "ZAsset.h"
#include "NPainterStd_UI.h"

class PainterTest_Window;
class ZFileSpec;

// ====================================================================================================
#pragma mark -
#pragma mark * PainterTest_App

class PainterTest_App : public ZApp
	{
public:
	static PainterTest_App* sPainterTest_App;

	PainterTest_App();
	virtual ~PainterTest_App();

// From ZWindowSupervisor via ZApp
	virtual void WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall);
	virtual void WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup);

// From ZMessageReceiver via ZApp
	virtual void ReceivedMessage(const ZMessage& inMessage);

// From ZApp
	virtual void OpenDocument(const ZMessage& inMessage);
	virtual void RunStarted();
	virtual void WindowRosterChanged();

// Our protocol
	PainterTest_Window* GetPainterWindow(const ZFileSpec& inFileSpec);
	void OpenFileSpec(const ZFileSpec& inFileSpec);

protected:
	ZRef<NPainterStd_UI::UIPaintState> fPaintState;
	ZAsset fAsset;
	ZAsset fAsset_NPainterStd;
	};

#endif // __PainterTest_App__
