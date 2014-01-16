#include "PainterTest_App.h"

#include "ZCommandLine.h"
#include "ZFile.h"
#include "ZLog.h"
#include "ZMain.h"
#include "ZStdIO.h"
#include "ZStream_POSIX.h"
#include "ZStrim_Stream.h"
#include "ZStrimmer_Streamer.h"
#include "ZUtil_Debug.h"
#include "ZUtil_UI.h"

// ====================================================================================================
#pragma mark -
#pragma mark * Windows-specific stuff to locate and communicate with a pre-existing instance

#if ZCONFIG(OS, Win32)

#include "ZMemoryBlock.h"
#include "ZOSWindow_Win.h"
#include "ZStream_Memory.h"
#include "ZUtil_Tuple.h"

/* For a discussion of what we're doing here, and why we're not just using DDE, see
see mozilla/xpfe/bootstrap/nsNativeAppSupportWin.cpp,
<https://bugzilla.mozilla.org/show_bug.cgi?id=53952>
and Paul DiLascia's column in MSDN September 2000, page 158.*/

static const char sIdentifyName[] = "com.motion.applications.paintertest";
static UINT sMSG_Identify = ::RegisterWindowMessageA(sIdentifyName);

static LRESULT CALLBACK sWindowProc_Identify(HWND iHWND, UINT iMessage,
	WPARAM iWPARAM, LPARAM iLPARAM)
	{
	if (iMessage == sMSG_Identify)
		{
		return 1;
		}
	else if (iMessage == WM_COPYDATA)
		{
		COPYDATASTRUCT* theCDS = reinterpret_cast<COPYDATASTRUCT*>(iLPARAM);
		if (theCDS->dwData == 1)
			{
			ZTupleValue theTV(ZStreamRPos_Memory(theCDS->lpData, theCDS->cbData));
			PainterTest_App::sPainterTest_App->OpenFileSpec(theTV.GetString());
			}
		return 0;
		}
	return ::DefWindowProcA(iHWND, iMessage, iWPARAM, iLPARAM);
	}

static BOOL WINAPI sEnumWindowsProc(HWND iHWND, LPARAM iLPARAM)
	{
	HWND* theHWND = reinterpret_cast<HWND*>(iLPARAM);
	char foundClassName[512];
	::GetClassNameA(iHWND, foundClassName, countof(foundClassName));
	if (0 == strcmp(sIdentifyName, foundClassName))
		{
		// We've found one. Now send it the identify message.
		if (::SendMessageA(iHWND, sMSG_Identify, 0, 0))
			{
			*theHWND = iHWND;
			return FALSE;
			}
		}
	return TRUE;
	}

// This is taken from ZOSWindow_Win.cpp, and perhaps
// should be made a utility function therein.
static void sRegisterWindowClassA(const UTF8* inWNDCLASSName, WNDPROC inWNDPROC)
	{
	WNDCLASSA windowClass;
	if (!::GetClassInfoA(::GetModuleHandleA(nil), inWNDCLASSName, &windowClass) &&
		!::GetClassInfoA(nil, inWNDCLASSName, &windowClass))
		{
		windowClass.style = 0;
		windowClass.lpfnWndProc = inWNDPROC;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = ::GetModuleHandleA(nil);
		windowClass.hIcon = nil;
		windowClass.hCursor = nil;
		windowClass.hbrBackground = nil;
		windowClass.lpszMenuName = nil;
		windowClass.lpszClassName = inWNDCLASSName;
		ATOM theResult = ::RegisterClassA(&windowClass);
		ZAssertStop(1, theResult != 0);
		}
	}

static void* sCheckForOtherPainter(void* iParam)
	{
	// This method is executed by the message loop thread.
	if (HANDLE theMUTEX = ::CreateMutex(0, FALSE, sIdentifyName))
		{
		DWORD result = WaitForSingleObject(theMUTEX, INFINITE);
		
		// Look for the other window.
		HWND theHWND = nil;
		EnumWindows(sEnumWindowsProc, reinterpret_cast<LPARAM>(&theHWND));

		if (theHWND)
			{
			ZMemoryBlock theMB;
			static_cast<const ZTupleValue*>(iParam)->ToStream(ZStreamRWPos_MemoryBlock(theMB));

			COPYDATASTRUCT theCDS;
			theCDS.dwData = 1;
			theCDS.cbData = theMB.GetSize();
			theCDS.lpData = theMB.GetData();
			::SendMessageA(theHWND, WM_COPYDATA,
				0, reinterpret_cast<LPARAM>(&theCDS));
			
			// We indicate success by returning a non-nil pointer.
			return reinterpret_cast<void*>(1);	
			}
		else
			{
			::sRegisterWindowClassA(sIdentifyName, sWindowProc_Identify);
			HWND theHWND = ::CreateWindowExA(
				0, // Extended attributes
				sIdentifyName, // window class name
				sIdentifyName, // window caption
				0,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				nil,
				(HMENU)nil,
				::GetModuleHandleA(nil), // program instance handle
				nil); // creation parameters			
			}

		ReleaseMutex(theMUTEX);
		CloseHandle(theMUTEX);
		}

	// We couldn't find another instance of the app, indicate that by returning nil.
	return nil;
	}

#endif // ZCONFIG(OS, Win32)

// ====================================================================================================
#pragma mark -
#pragma mark * PainterTest_CommandLine

struct PainterTest_CommandLine : public ZCommandLine
	{
	Bool fShowUsage;
	Bool fShowUsage2;
	String fFile;

	PainterTest_CommandLine();
	};

PainterTest_CommandLine::PainterTest_CommandLine()
:	fShowUsage("-u", "Print this message and exit", false),
	fShowUsage2("--help", "Print this message and exit", false),
	fFile("-f", "File to read")
	{}

// ====================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char **argv)
	{
	ZUtil_Debug::sInstall();
	ZUtil_Debug::sSetLogPriority(ZLog::ePriority_Debug);
	ZRef<ZStrimmerW> theStrimmer
		= new ZStrimmerW_Streamer_T<ZStrimW_StreamUTF8>(new ZStreamerW_T<ZStreamW_FILE>(stdout));
	ZUtil_Debug::sSetStrimmer(theStrimmer);

#if !ZCONFIG(OS, Carbon) && !ZCONFIG(OS, MacOSX)
	PainterTest_CommandLine theCommandLine;

	const ZStrimW& serr = ZStdIO::strim_err;
	const ZStrimW& sout = ZStdIO::strim_out;

 	// handle command line
	if (!theCommandLine.Parse(argc, argv))
		{
		theCommandLine.WriteUsage(serr);
		return 1;
		}

	if (theCommandLine.fShowUsage() || theCommandLine.fShowUsage2())
		{
		theCommandLine.WriteUsage(sout);
		return 0;
		}

	if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZMain"))
		s << "theCommandLine.fFile() = " << theCommandLine.fFile();
#endif


	ZUtil_UI::sInitializeUIFactories();

	try
		{		
		PainterTest_App theApp;
		bool doRun = true;

		#if !ZCONFIG(OS, Carbon) && !ZCONFIG(OS, MacOSX)
			if (theCommandLine.fFile.HasValue())
				{
				ZTupleValue theTV = theCommandLine.fFile();
				if (false) {}
				#if ZCONFIG(OS, Win32)
				else if (static_cast<ZOSApp_Win*>(theApp.GetOSApp())->InvokeFunctionFromMessagePump(sCheckForOtherPainter, &theTV))
					{
					// We were able to find another instance of the application, and
					// passed the path to it, so we shouldn't run theApp -- we need to
					// simply exit quietly.
					doRun = false;
					}
				#endif
				else
					{
					// No other instance could be found, so open the file ourselves.
					theApp.OpenFileSpec(theCommandLine.fFile());
					}
				}
		#endif

		if (doRun)
			theApp.Run();
		}
	catch (...)
		{}

	ZUtil_UI::sTearDownUIFactories();

	return 0;
	}
