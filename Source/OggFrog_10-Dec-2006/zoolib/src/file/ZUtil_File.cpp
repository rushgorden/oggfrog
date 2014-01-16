static const char rcsid[] = "@(#) $Id: ZUtil_File.cpp,v 1.19 2006/07/15 20:54:22 goingware Exp $";

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

#include "ZUtil_File.h"

// =================================================================================================

#if ZCONFIG_API_Enabled(File_MacClassic)

#include "ZMemory.h"
#include "ZString.h"
#include "ZUtil_Mac_HL.h"

#if ZCONFIG(OS, MacOSX)
#	include <AE/AppleEvents.h>
#	include <NavigationServices/Navigation.h>
#else
#	include <AppleEvents.h>
#	include <Navigation.h>
#endif

static pascal void sDummyEventProc(NavEventCallbackMessage iCallBackSelector, NavCBRecPtr iCallBackParms, void* iCallBackUD)
	{}

static NavEventUPP sDummyEventUPP = NewNavEventUPP(sDummyEventProc);

static OSType sGetProcessSignature()
	{
	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;
	ProcessInfoRec info;
	ZBlockSet(&info, sizeof(info), 0);
	info.processInfoLength = sizeof(ProcessInfoRec);
	if (noErr == ::GetProcessInformation(&process, &info))
		return info.processSignature;
	return '    ';
	}

static bool sGetAppFSSpec(FSSpec& oFSSpec)
	{
	oFSSpec.vRefNum = 0;
	oFSSpec.parID = 0;
	oFSSpec.name[0] = 0;

	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;

	ProcessInfoRec info;
	ZBlockSet(&info, sizeof(info), 0);
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processAppSpec = &oFSSpec;
	if (noErr == ::GetProcessInformation(&process, &info))
		return true;
	return false;
	}

bool ZUtil_File::sOpenFileDialog(const string& iPrompt, const vector<OSType>& iTypes, ZFileSpec& oSpec)
	{
	if (::NavServicesAvailable())
		{	    
		// Specify default options for dialog box
		NavDialogOptions dialogOptions;
		if (noErr != ::NavGetDefaultDialogOptions(&dialogOptions))
			return false;
		dialogOptions.dialogOptionFlags |= kNavAllFilesInPopup;
		dialogOptions.dialogOptionFlags &= ~kNavAllowMultipleFiles; // would be nice to support multiple files at some point. -ec 02.09.11
		
		AEDesc defaultLocationDesc;
		AEDesc* defaultLocationDescPtr = nil;

		FSSpec defaultFSSpec;
		if (ZCONFIG(OS, Carbon) && sGetAppFSSpec(defaultFSSpec))
			{
			::AECreateDesc(typeFSS, &defaultFSSpec, sizeof(FSSpec), &defaultLocationDesc);
			defaultLocationDescPtr = &defaultLocationDesc;
			}

		ZString::sToPString(iPrompt, dialogOptions.windowTitle, 255);

		NavTypeListHandle typeList = nil;
		if (iTypes.size())
			{
			typeList = NavTypeListHandle(::NewHandle((sizeof(NavTypeList) + (sizeof(OSType) * (iTypes.size() - 1)))));
			typeList[0]->componentSignature = sGetProcessSignature();
			typeList[0]->reserved = 0;
			typeList[0]->osTypeCount = iTypes.size();

			for (size_t i = 0; i < iTypes.size(); ++i )
				typeList[0]->osType[i] = iTypes.at(i);
			}

		NavReplyRecord reply;
#if 1 // this would allow us to have the Show: menu, but seems not to respect defaultLocationDesc. -ec 02.09.11
		OSErr anErr = ::NavGetFile(defaultLocationDescPtr, &reply, &dialogOptions, sDummyEventUPP,
							nil, nil, typeList, nil);
#else // this respects defaultLocationDesc, but does not support the Show: menu (which would allow the user to open non-'KFDB/LiM5' files). -ec 02.09.11
		OSErr anErr = ::NavChooseFile(&defaultLocationDesc, &reply, &dialogOptions, sDummyEventUPP,
							nil, nil, typeList, nil);
#endif
		if (typeList)
			::DisposeHandle(Handle(typeList));

		if (defaultLocationDescPtr)
			::AEDisposeDesc(defaultLocationDescPtr);

		if (anErr == noErr && reply.validRecord)
			{
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;
			FSSpec documentFSSpec;

			// Get a pointer to selected file
			if (noErr  == ::AEGetNthPtr(&(reply.selection), 1, typeFSS,
								&theKeyword, &actualType, &documentFSSpec, sizeof(documentFSSpec), &actualSize))
				{
				oSpec = ZFileSpec(new ZFileLoc_Mac_FSSpec(documentFSSpec));
				::NavDisposeReply(&reply);
				return true;
				}
			}
		::NavDisposeReply(&reply);
		return false;
		}
	return false;
	}

bool ZUtil_File::sSaveFileDialog(const string& iPrompt, const string* iSuggestedName, const ZFileSpec* iLocation, ZFileSpec& oSpec)
	{
	if (::NavServicesAvailable())
		{	    
		// Specify default options for dialog box
		NavDialogOptions dialogOptions;
		if (noErr != ::NavGetDefaultDialogOptions(&dialogOptions))
			return false;
		dialogOptions.dialogOptionFlags &= ~kNavAllowPreviews;
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;

		ZString::sToPString(iPrompt, dialogOptions.windowTitle, 255);

		// Ignore iLocation etc for now.
		AEDesc defaultLocationDesc;
		AEDesc* defaultLocationDescPtr = nil;
		
		FSSpec defaultFSSpec;
		if (ZCONFIG(OS, Carbon) && sGetAppFSSpec(defaultFSSpec))
			{
			::AECreateDesc(typeFSS, &defaultFSSpec, sizeof(FSSpec), &defaultLocationDesc);
			defaultLocationDescPtr = &defaultLocationDesc;
			}

		NavReplyRecord reply;  
  		OSErr anErr = ::NavPutFile(defaultLocationDescPtr, &reply, &dialogOptions, sDummyEventUPP, kNavGenericSignature, kNavGenericSignature, nil);

		::AEDisposeDesc(defaultLocationDescPtr);

		if (anErr == noErr && reply.validRecord)
			{
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;
			FSSpec documentFSSpec;

			// Get a pointer to selected file
			if (noErr  == ::AEGetNthPtr(&(reply.selection), 1, typeFSS,
								&theKeyword, &actualType, &documentFSSpec, sizeof(documentFSSpec), &actualSize))
				{
				oSpec = ZFileSpec(new ZFileLoc_Mac_FSSpec(documentFSSpec));
				::NavDisposeReply(&reply);
				return true;
				}
			}
		::NavDisposeReply(&reply);
		return false;
		}
	return false;
	}

bool ZUtil_File::sChooseFolderDialog(const string& iPrompt, ZFileSpec& outSpec)
	{
#if 0
	if (::NavServicesAvailable())
		{	    
		//  Specify default options for dialog box
		NavDialogOptions dialogOptions;
		if (noErr != ::NavGetDefaultDialogOptions(&dialogOptions))
			return false;
		dialogOptions.dialogOptionFlags &= ~kNavAllowPreviews;
		ZString::sToPString(iPrompt, dialogOptions.windowTitle, 255);

		// Use current dir as default location
		FSSpec fs;
		if (noErr != ::FSMakeFSSpec( 0, ::LMGetCurDirStore(), nil, &fs))
			return false;

		AEDesc defaultLocation;
		if (noErr != ::AECreateDesc(typeFSS, &fs, sizeof(fs), &defaultLocation))
			return false;

		NavReplyRecord reply;
		NavEventUPP openEventUPP = NewNavEventUPP(sOpenEventProc);
		OSErr anErr = ::NavChooseFolder(&defaultLocation,
							&reply,
							&dialogOptions,
							openEventUPP,
							nil,
							nil);
		DisposeNavEventUPP(openEventUPP);

		if (anErr == noErr && reply.validRecord)
			{
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;
			FSSpec documentFSSpec;

			// Get a pointer to selected file
			if (noErr  == ::AEGetNthPtr(&(reply.selection),
								1,
								typeFSS,
								&theKeyword,
								&actualType,
								&documentFSSpec,
								sizeof(documentFSSpec),
								&actualSize))
				{
				outSpec = documentFSSpec;
				::NavDisposeReply(&reply);
				return true;
				}
			}
		::NavDisposeReply(&reply);
		return false;
		}
	else
#endif
		{
		return false;
		}
	}

#endif // ZCONFIG_API_Enabled(File_MacClassic)

// =================================================================================================

#if ZCONFIG_API_Enabled(File_Win)

#include "ZHandle.h"
#include "ZMemory.h"
#include "ZOSWindow_Win.h" // For ZOSApp_Win::InvokeFunctionFromMessagePump
#include "ZUtil_Win.h"

#include <commdlg.h>

#undef GetOpenFileName
#undef GetSaveFileName

static UINT CALLBACK sOpenHook(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	return 0;
	}

static void* sCall_GetOpenFileNameA(void* inParam)
	{
	bool result = ::GetOpenFileNameA(reinterpret_cast<OPENFILENAMEA*>(inParam));
	return (void*)(result ? 0xFFFFFFFF : 0);
	}

static void* sCall_GetOpenFileNameW(void* inParam)
	{
	bool result = ::GetOpenFileNameW(reinterpret_cast<OPENFILENAMEW*>(inParam));
	return (void*)(result ? 0xFFFFFFFF : 0);
	}

bool ZUtil_File::sOpenFileDialog(const string& iPrompt, const vector<pair<string, string> >& inTypes, ZFileSpec& outSpec)
	{
	if (ZUtil_Win::sUseWAPI())
		{
		// Build the filter string
#if 0
		string defaultExtension;
		ZHandle tempHandle;
		ZStreamRWPos_Handle theStream(tempHandle);
		for (vector<pair<string, string> >::const_iterator i = inTypes.begin(); i != inTypes.end(); ++i)
			{
			if ((*i).first.empty() || (*i).second.empty())
				continue;
			if (defaultExtension.empty())
				defaultExtension = (*i).second;
			theStream.WriteString((*i).first);
			theStream.WriteUInt8(0);
			theStream.WriteString((*i).second);
			theStream.WriteUInt8(0);
			}
		theStream.WriteUInt8(0);
#endif

//		char initialFileString[2048];
//		initialFileString[0] = 0;

		wchar_t returnedFileString[2048];
		returnedFileString[0] = 0;

//		ZHandle::Ref theHandleRef(tempHandle);
		OPENFILENAMEW theOFN;
		ZBlockSet(&theOFN, sizeof(theOFN), 0);
		theOFN.lStructSize = sizeof(theOFN);
		theOFN.hwndOwner = nil;
		theOFN.lpstrFilter = nil;
//		theOFN.lpstrFilter = (const char*)theHandleRef.GetData();
		theOFN.nFilterIndex = 0;
		theOFN.lpstrFile = returnedFileString;
		theOFN.nMaxFile = sizeof(returnedFileString);
		theOFN.lpstrFileTitle = nil; //returnedFileString;
		theOFN.nMaxFileTitle = 0; //sizeof(returnedFileString);
		theOFN.lpstrInitialDir = nil;
		theOFN.lpstrTitle = nil;
		theOFN.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLEHOOK;
		theOFN.nFileOffset = 0;
		theOFN.nFileExtension = 0;
		theOFN.lpstrDefExt = nil; //defaultExtension.size() ? defaultExtension.c_str() : nil;
		theOFN.lCustData = 0;
		theOFN.lpfnHook = sOpenHook;
		theOFN.lpTemplateName = nil;

		if (!ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCall_GetOpenFileNameW, &theOFN))
			return false;

		outSpec = ZFileLoc_WinNT::sFromFullWinPath(returnedFileString);
		return true;
		}
	else
		{
		// Build the filter string
		string defaultExtension;
		ZHandle tempHandle;
		ZStreamRWPos_Handle theStream(tempHandle);
		for (vector<pair<string, string> >::const_iterator i = inTypes.begin(); i != inTypes.end(); ++i)
			{
			if ((*i).first.empty() || (*i).second.empty())
				continue;
			if (defaultExtension.empty())
				defaultExtension = (*i).second;
			theStream.WriteString((*i).first);
			theStream.WriteUInt8(0);
			theStream.WriteString((*i).second);
			theStream.WriteUInt8(0);
			}
		theStream.WriteUInt8(0);

		char initialFileString[2048];
		initialFileString[0] = 0;

		char returnedFileString[2048];
		returnedFileString[0] = 0;

		ZHandle::Ref theHandleRef(tempHandle);
		OPENFILENAMEA theOFN;
		ZBlockSet(&theOFN, sizeof(theOFN), 0);
		theOFN.lStructSize = sizeof(theOFN);
		theOFN.hwndOwner = nil;
		theOFN.lpstrFilter = nil;
		theOFN.lpstrFilter = (const char*)theHandleRef.GetData();
		theOFN.nFilterIndex = 0;
		theOFN.lpstrFile = returnedFileString;
		theOFN.nMaxFile = sizeof(returnedFileString);
		theOFN.lpstrFileTitle = nil; //returnedFileString;
		theOFN.nMaxFileTitle = 0; //sizeof(returnedFileString);
		theOFN.lpstrInitialDir = nil;
		theOFN.lpstrTitle = nil;
		theOFN.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_ENABLEHOOK;
		theOFN.nFileOffset = 0;
		theOFN.nFileExtension = 0;
		theOFN.lpstrDefExt = nil; //defaultExtension.size() ? defaultExtension.c_str() : nil;
		theOFN.lCustData = 0;
		theOFN.lpfnHook = sOpenHook;
		theOFN.lpTemplateName = nil;

		if (!ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCall_GetOpenFileNameA, &theOFN))
			return false;

		outSpec = ZFileLoc_Win::sFromFullWinPath(returnedFileString);
		return true;
		}
	}

static void* sCall_GetSaveFileName(void* inParam)
	{
	bool result = ::GetSaveFileNameA(reinterpret_cast<OPENFILENAMEA*>(inParam));
	return (void*)(result ? 0xFFFFFFFF : 0);
	}

bool ZUtil_File::sSaveFileDialog(const string& iPrompt, const string* iSuggestedName, const ZFileSpec* inOriginal, ZFileSpec& outSpec)
	{
	char initialFileString[2048];
	initialFileString[0] = 0;
	if (iSuggestedName)
		::strcpy(initialFileString, iSuggestedName->c_str());

	char returnedFileString[2048];
	returnedFileString[0] = 0;

//	ZHandle::Ref theHandleRef(tempHandle);
	OPENFILENAMEA theOFN;
	ZBlockSet(&theOFN, sizeof(theOFN), 0);
	theOFN.lStructSize = sizeof(theOFN);
	theOFN.hwndOwner = nil;
	theOFN.lpstrFilter = nil; //(const char*)theHandleRef.GetData();
	theOFN.nFilterIndex = 0;
	theOFN.lpstrFile = initialFileString;
	theOFN.nMaxFile = sizeof(initialFileString);
	theOFN.lpstrFileTitle = returnedFileString;
	theOFN.nMaxFileTitle = sizeof(returnedFileString);
	theOFN.lpstrInitialDir = nil;
	theOFN.lpstrTitle = nil;
	theOFN.Flags = OFN_HIDEREADONLY | OFN_LONGNAMES; //### is this right?
	theOFN.nFileOffset = 0;
	theOFN.nFileExtension = 0;
	theOFN.lpstrDefExt = nil; //defaultExtension.size() ? defaultExtension.c_str() : nil;
	theOFN.lCustData = 0;
	theOFN.lpfnHook = nil;
	theOFN.lpTemplateName = nil;

	if (!ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCall_GetSaveFileName, &theOFN))
		return false;

	//outSpec = ZFileLoc_Win::sFromWinPath(returnedFileString);
	outSpec = ZFileLoc_Win::sFromFullWinPath(theOFN.lpstrFile);

	return true;
	}


#endif // ZCONFIG_API_Enabled(File_Win)

// =================================================================================================
