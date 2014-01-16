static const char rcsid[] = "@(#) $Id: ZFile_MacClassic.cpp,v 1.38 2006/07/27 22:05:26 agreen Exp $";

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

#include "ZFile_MacClassic.h"

#if ZCONFIG_API_Enabled(File_MacClassic)

#include "ZMacOSX.h"
#include "ZMemory.h"
#include "ZString.h"
#include "ZThreadTM.h"

#define kDebug_File_Mac 2

// =================================================================================================
#pragma mark -
#pragma mark * Shared implementation details

#if ZCONFIG(API_Thread, Mac)
#	define PBINVOKE(func, thePB) (ZThreadTM_SetupForAsyncCompletion(&thePB),\
		ZThreadTM_WaitForAsyncCompletion(&thePB, PB##func##Async(&thePB.fRealPB)))
#	define PBINVOKE2(func, thePB, theRealPB) (ZThreadTM_SetupForAsyncCompletion(&thePB),\
		ZThreadTM_WaitForAsyncCompletion(&thePB, PB##func##Async(theRealPB)))

#	define PBINVOKECHECKED(func, thePB, oErr) (ZThreadTM_SetupForAsyncCompletion(&thePB),\
		::sWaitForAsyncCompletion(&thePB, PB##func##Async(&thePB.fRealPB), oErr))
#	define PBINVOKECHECKED2(func, thePB, theRealPB, oErr) (ZThreadTM_SetupForAsyncCompletion(&thePB),\
		::sWaitForAsyncCompletion(&thePB, PB##func##Async(theRealPB), oErr))
#else
#	define PBINVOKE(func, thePB) PB##func##Sync(&thePB.fRealPB)
#	define PBINVOKE2(func, thePB, theRealPB) PB##func##Sync(theRealPB)
#	define PBINVOKECHECKED(func, thePB, oErr) sCheckSyncCompletion(PB##func##Sync(&thePB.fRealPB), oErr)
#	define PBINVOKECHECKED2(func, thePB, theRealPB, oErr) sCheckSyncCompletion(PB##func##Sync(theRealPB), oErr)
#endif

static void sCopyPString(const unsigned char* iSource, unsigned char* iDest, size_t iDestSize)
	{ ZBlockCopy(iSource, iDest, min(iDestSize, 1 + size_t(iSource[0]))); }

static int sComparePStrings(const unsigned char* iLHS, const unsigned char* iRHS)
	{
	int result = 0;
	size_t offset = 0;
	while (offset < iLHS[0] && offset < iRHS[0] && result == 0)
		{
		++offset;
		result = int(iLHS[offset]) - int(iRHS[offset]);
		}
	if (result == 0)
		{
		if (iLHS[0] < iRHS[0])
			result = -1;
		else if (iLHS[0] > iRHS[0])
			result = 1;
		}
	return result;
	}

static bool sGestalt_HasHFSPlus()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		if (noErr == ::Gestalt(gestaltFSAttr, &response))
			{
			if ((response & (1 << gestaltHasHFSPlusAPIs)))
				result = true;
			}
		checked = true;
		}
	return result;
	}

struct Threaded_HParamBlockRec : ZThreadTM_PBHeader
	{
	HParamBlockRec fRealPB;
	};

struct Threaded_ParamBlockRec : ZThreadTM_PBHeader
	{
	ParamBlockRec fRealPB;
	};

struct Threaded_FCBPBRec : ZThreadTM_PBHeader
	{
	FCBPBRec fRealPB;
	};

struct Threaded_CInfoPBRec : ZThreadTM_PBHeader
	{
	CInfoPBRec fRealPB;
	};

struct Threaded_CMovePBRec : ZThreadTM_PBHeader
	{
	CMovePBRec fRealPB;
	};

static ZFile::Error sTranslateError(OSErr iNativeError)
	{
	ZFile::Error theErr = ZFile::errorGeneric;
	switch (iNativeError)
		{
		case noErr:
			theErr = ZFile::errorNone;
			break;
		case eofErr:
			theErr = ZFile::errorReadPastEOF;
			break;
		case dirNFErr:
		case fnfErr:
			theErr = ZFile::errorDoesntExist;
			break;
		case bdNamErr:
			theErr = ZFile::errorIllegalFileName;
			break;
		case afpAccessDenied:
			theErr = ZFile::errorNoPermission;
			break;
		case dupFNErr:
		case afpObjectTypeErr:
			theErr = ZFile::errorAlreadyExists;
			break;
		}
	return theErr;
	}

static ZFile::Error sOpen_Old(short iVRefNum, long iDirID, const StrFileName& iName, bool iAllowRead, bool iAllowWrite, bool iPreventWriters, SInt16& oRefNum)
	{
	// First try to resolve the file, in case it's an alias. Note
	// that we transcribe the parameters into theSpec, and later
	// we'll use theSpec as the source of the data passed to PBHOpenDFAsync.
	FSSpec theSpec;
	theSpec.vRefNum = iVRefNum;
	theSpec.parID = iDirID;
	sCopyPString(iName, theSpec.name, sizeof(theSpec.name));

	Boolean targetIsFolder;
	Boolean wasAliased;
	OSErr result = ::ResolveAliasFileWithMountFlags(&theSpec, true, &targetIsFolder, &wasAliased, kResolveAliasFileNoUI);
	if (wasAliased)
		{
		// It was an alias.
		if (noErr == result)
			{
			// Success, theSpec has had its values updated to reference the target.
			if (targetIsFolder)
				{
				// But it's a folder, and thus unopenable.
				return ZFile::errorWrongTypeForOperation;
				}
			}
		else
			{
			// But it didn't resolve.
			return ZFile::errorGeneric;
			}
		}

	// Translate the permissions from our model to MacOS's.
	SInt8 realPerm = 0;

	if (iAllowRead && iAllowWrite)
		{
		// It's read/write
		if (iPreventWriters)
			realPerm = fsRdWrPerm;
		else
			realPerm = fsRdWrShPerm;
		}
	else if (iAllowRead && !iAllowWrite)
		{
		if (iPreventWriters)
			realPerm = fsRdPerm;
		else
			realPerm = fsRdWrShPerm;
		}
	else if (!iAllowRead && iAllowWrite)
		{
		if (iPreventWriters)
			realPerm = fsWrPerm;
		else
			realPerm = fsRdWrShPerm;
		}

	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioNamePtr = &theSpec.name[0];
	thePB.fRealPB.ioParam.ioVRefNum = theSpec.vRefNum;
	thePB.fRealPB.fileParam.ioDirID = theSpec.parID;
	thePB.fRealPB.ioParam.ioPermssn = realPerm;

	OSErr err = PBINVOKE(HOpenDF, thePB);

	if (noErr != err)
		return ::sTranslateError(err);

	oRefNum = thePB.fRealPB.ioParam.ioRefNum;
	return ZFile::errorNone;
	}

static ZFile::Error sCloseRefNum_Old(SInt16 iRefNum)
	{
	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;

	OSErr err = PBINVOKE(Close, thePB);

	return ::sTranslateError(err);
	}

static ZFile::Error sCreate_Old(short iVRefNum, long iDirID, const StrFileName& iName, bool iReplaceExisting)
	{
	// Create the file
	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioNamePtr = const_cast<unsigned char*>(&iName[0]);
	thePB.fRealPB.ioParam.ioVRefNum = iVRefNum;
	thePB.fRealPB.fileParam.ioDirID = iDirID;
	thePB.fRealPB.ioParam.ioVersNum = 0;

	OSErr err = PBINVOKE(HCreate, thePB);

	if (dupFNErr == err)
		{
		// A duplicate filename error is only a problem if if iReplaceExisting is false.
		if (!iReplaceExisting)
			return ZFile::errorAlreadyExists;
		}
	else if (noErr != err)
		{
		return ::sTranslateError(err);
		}

	return ZFile::errorNone;
	}

static ZFile::Error sReadAt_Old(SInt16 iRefNum, uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (oCountRead)
		*oCountRead = 0;

	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;
	thePB.fRealPB.ioParam.ioBuffer = reinterpret_cast<Ptr>(iDest);
	thePB.fRealPB.ioParam.ioReqCount = iCount;
	thePB.fRealPB.ioParam.ioPosMode = fsFromStart;
	thePB.fRealPB.ioParam.ioPosOffset = iOffset;

	OSErr err = PBINVOKE(Read, thePB);

	if (oCountRead)
		*oCountRead = thePB.fRealPB.ioParam.ioActCount;

	return ::sTranslateError(err);
	}

static ZFile::Error sWriteAt_Old(SInt16 iRefNum, uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (oCountWritten)
		*oCountWritten = 0;

	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;
	thePB.fRealPB.ioParam.ioBuffer = (Ptr)iSource;
	thePB.fRealPB.ioParam.ioReqCount = iCount;
	thePB.fRealPB.ioParam.ioPosMode = fsFromStart;
	thePB.fRealPB.ioParam.ioPosOffset = iOffset;

	OSErr err = PBINVOKE(Write, thePB);

	if (oCountWritten)
		*oCountWritten = thePB.fRealPB.ioParam.ioActCount;

	return ::sTranslateError(err);
	}

static ZFile::Error sGetSize_Old(SInt16 iRefNum, uint64& oSize)
	{
	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;

	OSErr err = PBINVOKE(GetEOF, thePB);

	oSize = reinterpret_cast<size_t>(thePB.fRealPB.ioParam.ioMisc);

	return ::sTranslateError(err);
	}

static ZFile::Error sSetSize_Old(SInt16 iRefNum, uint64 iSize)
	{
	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;
	thePB.fRealPB.ioParam.ioMisc = (Ptr)iSize;

	OSErr err = PBINVOKE(SetEOF, thePB);

	return ::sTranslateError(err);
	}

static ZFile::Error sFlush_Old(SInt16 iRefNum)
	{
	Threaded_ParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioRefNum = iRefNum;

	OSErr err = PBINVOKE(FlushFile, thePB);

	return ::sTranslateError(err);
	}

static ZFile::Error sFlushVolume_Old(SInt16 iRefNum)
	{
	// Get the vRefNum for the volume holding this file
	Threaded_FCBPBRec thePB;
	thePB.fRealPB.ioRefNum = iRefNum;
	thePB.fRealPB.ioNamePtr = nil;
	thePB.fRealPB.ioVRefNum = 0;
	thePB.fRealPB.ioFCBIndx = 0;

	OSErr err = PBINVOKE(GetFCBInfo, thePB);

	if (noErr != err)
		return ::sTranslateError(err);

	thePB.fRealPB.ioNamePtr = nil;

	// And flush the volume
	err = PBINVOKE2(FlushVol, thePB, (ParamBlockRec*)&thePB.fRealPB);

	return ::sTranslateError(err);
	}

static bool sWaitForAsyncCompletion(ZThreadTM_PBHeader* iPB, OSErr iErr, ZFile::Error* oErr)
	{
	OSErr err = ZThreadTM_WaitForAsyncCompletion(iPB, iErr);
	if (noErr != err)
		{
		if (oErr)
			*oErr = ::sTranslateError(err);
		return false;
		}
	return true;
	}

static bool sCheckSyncCompletion(OSErr iErr, ZFile::Error* oErr)
	{
	if (noErr != iErr)
		{
		if (oErr)
			*oErr = ::sTranslateError(iErr);
		return false;
		}
	return true;
	}

static long sGetParentDirID(short iVRefNum, long iDirID, unsigned char* oName, ZFile::Error* oErr)
	{
	Threaded_CInfoPBRec thePB;
	thePB.fRealPB.dirInfo.ioNamePtr = oName;
	thePB.fRealPB.dirInfo.ioFDirIndex = -1;
	thePB.fRealPB.dirInfo.ioVRefNum = iVRefNum;
	thePB.fRealPB.dirInfo.ioDrDirID = iDirID;

	if (!PBINVOKECHECKED2(FlushVol, thePB, (ParamBlockRec*)&thePB.fRealPB, oErr))
		return 0;

	return thePB.fRealPB.dirInfo.ioDrParID;
	}

static long sGetParentDirID(short iVRefNum, long iDirID, string& oName, ZFile::Error* oErr)
	{
	StrFileName theName;
	if (long parentID = ::sGetParentDirID(iVRefNum, iDirID, &theName[0], oErr))
		{
		oName = ZString::sFromPString(theName);
		return parentID;
		}
	return 0;
	}

static long sGetParentDirID(short iVRefNum, long iDirID, ZFile::Error* oErr)
	{ return ::sGetParentDirID(iVRefNum, iDirID, nil, oErr); }

static bool sVolNameToRefNum(const unsigned char* iName, short& oVRefNum, ZFile::Error* oErr)
	{
	// All this time I thought I knew how to do this but didn't. Anyway, in order to
	// *really* map a volume name to a vRefNum we have to pass ioVolIndex == -1 and
	// also pass the name as a minimal path -- a string with a colon at the end.

	Str32 volumeName;
	sCopyPString(iName, volumeName, sizeof(volumeName));
	volumeName[volumeName[0] + 1] = ':';
	volumeName[0] = volumeName[0] + 1;

	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.volumeParam.ioNamePtr = &volumeName[0];
	thePB.fRealPB.volumeParam.ioVRefNum = 0;
	thePB.fRealPB.volumeParam.ioVolIndex = -1;

	if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
		return false;

	oVRefNum = thePB.fRealPB.volumeParam.ioVRefNum;
	return true;
	}

static ZRef<ZFileLoc> sChangeVolumeName(short iSourceVRefNum, const unsigned char* iNewName, ZFile::Error* oErr)
	{
	Str31 volumeName;
	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.volumeParam.ioNamePtr = &volumeName[0];
	thePB.fRealPB.volumeParam.ioVRefNum = iSourceVRefNum;
	thePB.fRealPB.volumeParam.ioVolIndex = 0;

	if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
		return ZRef<ZFileLoc>();

	if (sComparePStrings(volumeName, iNewName) != 0)
		{
		thePB.fRealPB.volumeParam.ioNamePtr = const_cast<unsigned char*>(&iNewName[0]);
		if (!PBINVOKECHECKED(SetVInfo, thePB, oErr))
			return ZRef<ZFileLoc>();
		}

	return new ZFileLoc_Mac_FSSpec(iSourceVRefNum, fsRtDirID);
	}

static ZRef<ZFileLoc> sChangeVolumeName(const unsigned char* iOldName, const unsigned char* iNewName, ZFile::Error* oErr)
	{
	if (sComparePStrings(iOldName, iNewName) == 0)
		{
		if (oErr)
			*oErr = ZFile::errorNone;
		return new ZFileLoc_Mac_FSSpec(0, fsRtParID, iOldName);
		}

	Str32 volumeName;
	sCopyPString(iOldName, volumeName, sizeof(volumeName));
	volumeName[volumeName[0] + 1] = ':';
	volumeName[0] = volumeName[0] + 1;

	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.volumeParam.ioNamePtr = &volumeName[0];
	thePB.fRealPB.volumeParam.ioVRefNum = 0;
	thePB.fRealPB.volumeParam.ioVolIndex = -1;

	if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
		return ZRef<ZFileLoc>();

	thePB.fRealPB.volumeParam.ioNamePtr = const_cast<unsigned char*>(&iNewName[0]);
	thePB.fRealPB.volumeParam.ioVolIndex = 0;

	if (!PBINVOKECHECKED(SetVInfo, thePB, oErr))
		return ZRef<ZFileLoc>();

	return new ZFileLoc_Mac_FSSpec(thePB.fRealPB.volumeParam.ioVRefNum, fsRtDirID);
	}

static long sCreateUniqueDir(short iVRefNum, long iParentID, ZFile::Error* oErr)
	{
	Str31 uniqueTempDirName;
	::NumToString(::TickCount(), uniqueTempDirName);

	for (size_t x = 0; x < 64; ++x)
		{
		Threaded_HParamBlockRec thePB;
		thePB.fRealPB.fileParam.ioNamePtr = &uniqueTempDirName[0];
		thePB.fRealPB.fileParam.ioVRefNum = iVRefNum;
		thePB.fRealPB.fileParam.ioDirID = iParentID;

		OSErr err = PBINVOKE(DirCreate, thePB);

		if (noErr == err)
			{
			if (oErr)
				*oErr = ZFile::errorNone;
			return thePB.fRealPB.fileParam.ioDirID;
			}
		else if (err == dupFNErr)
			{
			// Duplicate name - change the first character to the next ASCII character.
			++uniqueTempDirName[1];
			// Make sure it isn't a colon!
			if (uniqueTempDirName[1] == ':')
				++uniqueTempDirName[1];
			}
		else
			{
			if (oErr)
				*oErr = ::sTranslateError(err);
			return 0;
			}
		}
	if (oErr)
		*oErr = ZFile::errorGeneric;
	return 0;
	}

static ZTime sAsZTime(unsigned long iMacTime)
	{
	// 66 years and 17 leap days between 1904 and 1970 epochs.
	return ZTime(double(iMacTime - ZTime::kEpochDelta_1904_To_1970));
	}

static bool sGet_CInfo(short iVRefNum, long iDirID, const StrFileName& iName, Threaded_CInfoPBRec& ioPB, ZFile::Error* oErr)
	{
	if (iDirID == fsRtParID)
		{
		// There's no info associated with the root of the filesystem.
		if (oErr)
			*oErr = ZFile::errorWrongTypeForOperation;
		return false;
		}

	StrFileName tempName;
	sCopyPString(iName, tempName, sizeof(tempName));

	ioPB.fRealPB.hFileInfo.ioNamePtr = &tempName[0];
	ioPB.fRealPB.hFileInfo.ioVRefNum = iVRefNum;
	ioPB.fRealPB.hFileInfo.ioFVersNum = 0;
	ioPB.fRealPB.hFileInfo.ioDirID = iDirID;
	if (iName[0])
		{
		// We have a name, so use it and iDirID.
		ioPB.fRealPB.hFileInfo.ioFDirIndex = 0;
		}
	else
		{
		// We don't have a name, use only iDirID.
		ioPB.fRealPB.hFileInfo.ioFDirIndex = 1;
		}

	return PBINVOKECHECKED(GetCatInfo, ioPB, oErr);
	}

static bool sGet_FInfo(short iVRefNum, long iDirID, const StrFileName& iName, Threaded_HParamBlockRec& ioPB, ZFile::Error* oErr)
	{
	if (!iName[0] || iDirID == fsRtParID)
		{
		if (oErr)
			*oErr = ZFile::errorWrongTypeForOperation;
		return false;
		}

	StrFileName tempName;
	sCopyPString(iName, tempName, sizeof(tempName));

	ioPB.fRealPB.fileParam.ioNamePtr = &tempName[0];
	ioPB.fRealPB.fileParam.ioVRefNum = iVRefNum;
	ioPB.fRealPB.fileParam.ioFVersNum = 0;
	ioPB.fRealPB.fileParam.ioDirID = iDirID;
	ioPB.fRealPB.fileParam.ioFDirIndex = 0;

	return PBINVOKECHECKED(HGetFInfo, ioPB, oErr);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileIterRep_Mac_FSSpec

class ZFileIterRep_Mac_FSSpec : public ZFileIterRep
	{
public:
	ZFileIterRep_Mac_FSSpec(short iVRefNum, long iDirID, const unsigned char* iName);
	ZFileIterRep_Mac_FSSpec(short iVRefNum, long iDirID, size_t iIndex, const unsigned char* iCurrentName, short iCurrentVRefNum);
	virtual ~ZFileIterRep_Mac_FSSpec();

// From ZFileIterRep
	virtual bool HasValue();
	virtual void Advance();
	virtual ZFileSpec Current();
	virtual string CurrentName() const;

	virtual ZRef<ZFileIterRep> Clone();

private:
	void GrabCurrent();

	short fVRefNum;
	long fDirID;
	size_t fIndex;

	StrFileName fCurrentName;
	short fCurrentVRefNum;
	};

ZFileIterRep_Mac_FSSpec::ZFileIterRep_Mac_FSSpec(short iVRefNum, long iDirID, const unsigned char* iName)
	{
	fVRefNum = iVRefNum;
	fDirID = iDirID;
	fIndex = 0;
	fCurrentName[0] = 0;
	fCurrentVRefNum = 0;

	// Turn parameters into non name-requiring cases (from 2, 4, 6 to 3 or 5).
	if (iName[0])
		{
		if (fDirID == fsRtParID)
			{
			// Case 2
			if (!sVolNameToRefNum(iName, fVRefNum, nil))
				{
				// It was a bad name. Bail out.
				return;
				}
			else
				{
				// fVRefNum was assigned the vRefNum of the volume named iName.
				fDirID = fsRtDirID;
				// We're now case 3.
				}
			}
		else
			{
			// Case 4 or 6
			FSSpec theSpec;
			sCopyPString(iName, theSpec.name, sizeof(theSpec.name));
			for (;;)
				{
				// We use a loop to allow the alias resolution code to reuse the logic herein.
				Threaded_CInfoPBRec thePB;
				thePB.fRealPB.dirInfo.ioNamePtr = &theSpec.name[0];
				thePB.fRealPB.dirInfo.ioFDirIndex = 0;
				thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
				thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

				if (!PBINVOKECHECKED(GetCatInfo, thePB, nil))
					{
					// It was a bad name. Bail out.
					return;
					}
				else
					{
					if (thePB.fRealPB.hFileInfo.ioFlAttrib & kioFlAttribDirMask)
						{
						// It's a directory.
						ZAssertStop(1, thePB.fRealPB.dirInfo.ioDrParID == fDirID);
						fDirID = thePB.fRealPB.dirInfo.ioDrDirID;
						// We're now case 3 or 5 and can drop out of the loop.
						break;
						}
					else if (thePB.fRealPB.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
						{
						// It's an alias.
						theSpec.vRefNum = fVRefNum;
						theSpec.parID = fDirID;
						Boolean targetIsFolder;
						Boolean wasAliased;
						OSErr result = ::ResolveAliasFileWithMountFlags(&theSpec, true, &targetIsFolder, &wasAliased, kResolveAliasFileNoUI);
						if (result != noErr || !wasAliased || !targetIsFolder)
							{
							// It was not a valid alias referencing a folder. Bail out.
							return;
							}
						// We can go back around the loop and derive a directory ID from the parID and name
						// returned by the alias resolution.
						fVRefNum = theSpec.vRefNum;
						fDirID = theSpec.parID;
						}
					else
						{
						// It's not a directory. Bail out.
						return;
						}
					}
				}
			}
		}

	fIndex = 1;
	this->GrabCurrent();
	}

ZFileIterRep_Mac_FSSpec::ZFileIterRep_Mac_FSSpec(short iVRefNum, long iDirID, size_t iIndex, const unsigned char* iCurrentName, short iCurrentVRefNum)
:	fVRefNum(iVRefNum),
	fDirID(iDirID),
	fIndex(iIndex),
	fCurrentVRefNum(iCurrentVRefNum)
	{
	sCopyPString(iCurrentName, fCurrentName, sizeof(fCurrentName));
	}

ZFileIterRep_Mac_FSSpec::~ZFileIterRep_Mac_FSSpec()
	{}

bool ZFileIterRep_Mac_FSSpec::HasValue()
	{ return fIndex; }

void ZFileIterRep_Mac_FSSpec::Advance()
	{
	if (fIndex)
		{
		++fIndex;
		this->GrabCurrent();
		}
	}

ZFileSpec ZFileIterRep_Mac_FSSpec::Current()
	{
	if (fIndex)
		{
		if (fDirID == fsRtParID)
			{
			ZAssertStop(kDebug_File_Mac, fCurrentVRefNum != 0);
			return ZFileSpec(new ZFileLoc_Mac_FSSpec(fCurrentVRefNum, fsRtDirID));
			}
		else
			{
			return ZFileSpec(new ZFileLoc_Mac_FSSpec(fVRefNum, fDirID, fCurrentName));
			}
		}
	return ZFileSpec();
	}

string ZFileIterRep_Mac_FSSpec::CurrentName() const
	{ return ZString::sFromPString(fCurrentName); }

ZRef<ZFileIterRep> ZFileIterRep_Mac_FSSpec::Clone()
	{ return new ZFileIterRep_Mac_FSSpec(fVRefNum, fDirID, fIndex, fCurrentName, fCurrentVRefNum); }

void ZFileIterRep_Mac_FSSpec::GrabCurrent()
	{
	if (fDirID == fsRtParID)
		{
		// Case 1.
		Threaded_HParamBlockRec thePB;
		thePB.fRealPB.volumeParam.ioNamePtr = &fCurrentName[0]; // returns name
		thePB.fRealPB.volumeParam.ioVRefNum = fVRefNum;
		thePB.fRealPB.volumeParam.ioVolIndex = fIndex;

		if (!PBINVOKECHECKED(HGetVInfo, thePB, nil))
			fIndex = 0;
		else
			fCurrentVRefNum = thePB.fRealPB.volumeParam.ioVRefNum;
		}
	else
		{
		// Case 3 or 5.
		Threaded_CInfoPBRec thePB;
		thePB.fRealPB.dirInfo.ioNamePtr = &fCurrentName[0];
		thePB.fRealPB.dirInfo.ioFDirIndex = fIndex;
		thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
		thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

		if (!PBINVOKECHECKED(GetCatInfo, thePB, nil))
			fIndex = 0;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileLoc_Mac_FSSpec

/* ZFileLoc_Mac_FSSpec represents locations in the file system using a volume
reference number, directory ID and name. We assign meaning to combinations of
values in a form that slightly extends that used by normal FSSpecs:

   VRefNum  DirID      Name    Designates
   -------  -----      ----    ----------
1  any      fsRtParID  ""      Root of the filesystem.

2  any      fsRtParID  "name"  The top level directory of the volume titled "name".

3  X        fsRtDirID  ""      The top level directory of the volume with VRefNum == X.

4  X        fsRtDirID  "name"  The file/directory titled "name" in the top level directory
                                 of the volume with VRefNum == X.

5  X        Y          ""      The directory with ID == Y on the volume with VRefNum == X.

6  X        Y          "name"  The file/directory titled "name" in the directory with
                                 ID == Y on the volume with VRefNum == X.

where fsRtParID == 1, fsRtDirID == 2
*/

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::sGet_CWD()
	{
	// There isn't a perfect match for a CWD on Mac. Use the app's location for now.
	FSSpec appSpec;
	appSpec.vRefNum = 0;
	appSpec.parID = 0;
	appSpec.name[0] = 0;

	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;
	ProcessInfoRec info;
	ZBlockSet(&info, sizeof(info), 0);
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processAppSpec = &appSpec;
	if (noErr == ::GetProcessInformation(&process, &info))
		return new ZFileLoc_Mac_FSSpec(appSpec.vRefNum, appSpec.parID);
	return ZRef<ZFileLoc>();
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::sGet_Root()
	{
	return new ZFileLoc_Mac_FSSpec(0, fsRtParID);
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::sGet_App()
	{
	FSSpec appSpec;
	appSpec.vRefNum = 0;
	appSpec.parID = 0;
	appSpec.name[0] = 0;

	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;
	ProcessInfoRec info;
	ZBlockSet(&info, sizeof(info), 0);
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processAppSpec = &appSpec;
	if (noErr == ::GetProcessInformation(&process, &info))
		return new ZFileLoc_Mac_FSSpec(appSpec.vRefNum, appSpec.parID, appSpec.name);
	return ZRef<ZFileLoc>();
	}

ZFileLoc_Mac_FSSpec::ZFileLoc_Mac_FSSpec(short iVRefNum, long iDirID, const unsigned char* iName)
	{
	fVRefNum = iVRefNum;
	fDirID = iDirID;
	sCopyPString(iName, fName, sizeof(fName));
	}

ZFileLoc_Mac_FSSpec::ZFileLoc_Mac_FSSpec(short iVRefNum, long iDirID)
	{
	fVRefNum = iVRefNum;
	fDirID = iDirID;
	fName[0] = 0;
	}

ZFileLoc_Mac_FSSpec::ZFileLoc_Mac_FSSpec(const FSSpec& iSpec)
	{
	fVRefNum = iSpec.vRefNum;
	fDirID = iSpec.parID;
	sCopyPString(iSpec.name, fName, sizeof(fName));
	}

ZFileLoc_Mac_FSSpec::~ZFileLoc_Mac_FSSpec()
	{}

ZRef<ZFileIterRep> ZFileLoc_Mac_FSSpec::CreateIterRep()
	{ return new ZFileIterRep_Mac_FSSpec(fVRefNum, fDirID, fName); }

string ZFileLoc_Mac_FSSpec::GetName(ZFile::Error* oErr) const
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fName[0])
		{
		// Cases 2, 4, 6
		return ZString::sFromPString(fName);
		}

	if (fDirID == fsRtParID)
		{
		// Case 1 -- we reference the root of the file system,
		// so there is no spoon ... sorry ... name.
		return string();
		}
	else if (fDirID == fsRtDirID)
		{
		// Case 3 -- we reference the top level directory of the volume
		// fVRefNum, so the leaf name is the volume name.
		Str31 volumeName;
		Threaded_HParamBlockRec thePB;
		thePB.fRealPB.volumeParam.ioNamePtr = &volumeName[0]; // returns name
		thePB.fRealPB.volumeParam.ioVRefNum = fVRefNum;
		thePB.fRealPB.volumeParam.ioVolIndex = 0;

		if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
			return string();

		return ZString::sFromPString(volumeName);
		}
	else
		{
		// Case 5 -- We're referencing the directory with id fDirID on
		// volume fVRefNum, the leaf name is the directory's name.
		Str31 directoryName;
		Threaded_CInfoPBRec thePB;
		thePB.fRealPB.dirInfo.ioNamePtr = directoryName;
		thePB.fRealPB.dirInfo.ioFDirIndex = -1;
		thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
		thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

		if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
			return string();

		return ZString::sFromPString(directoryName);
		}
	}

ZTrail ZFileLoc_Mac_FSSpec::TrailTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr) const
	{
	if (ZFileLoc_Mac_FSSpec* dest = ZRefDynamicCast<ZFileLoc_Mac_FSSpec>(iDest))
		{
		if (dest->fDirID == fDirID && (dest->fVRefNum == fVRefNum || (dest->fDirID == fsRtParID && fDirID == fsRtParID)))
			{
			ZTrail theTrail;
			// We reference the same directory ID on the same vRefnum (or we both reference the file system root).
			if (fName[0])
				theTrail.AppendBounce();
			if (dest->fName[0])
				theTrail.AppendComp(ZString::sFromPString(dest->fName));
			if (oErr)
				*oErr = ZFile::errorNone;
			return theTrail;
			}
		else if (dest->fDirID != fsRtParID && fDirID != fsRtParID && dest->fVRefNum == fVRefNum)
			{
			// We're on the same drive, we'll walk the two dirIDs up the tree until
			// destDirID tops out or intersects a sourceDirID we've already hit.
			vector<string> destDirNames;
			long destDirID = dest->fDirID;
			vector<long> sourceDirIDs;
			sourceDirIDs.push_back(fDirID);
			for (;;)
				{
				long destParentID = destDirID;
				if (destDirID != fsRtDirID)
					{
					string destName;
					destParentID = ::sGetParentDirID(fVRefNum, destDirID, destName, oErr);
					if (destParentID == 0)
						return ZTrail(false);
					destDirNames.push_back(destName);
					}

				vector<long>::iterator iter = find(sourceDirIDs.begin(), sourceDirIDs.end(), destDirID);
				if (iter != sourceDirIDs.end())
					{
					sourceDirIDs.erase(iter, sourceDirIDs.end());
					break;
					}

				destDirID = destParentID;
				if (sourceDirIDs.back() != fsRtDirID)
					{
					long sourceParentID = ::sGetParentDirID(fVRefNum, sourceDirIDs.back(), oErr);
					if (sourceParentID == 0)
						return ZTrail(false);
					sourceDirIDs.push_back(sourceParentID);
					}
				}

			ZTrail theTrail;
			theTrail.AppendBounces(sourceDirIDs.size());

			if (fName[0])
				theTrail.AppendBounce();

			for (size_t x = 0; x < destDirNames.size(); ++x)
				theTrail.AppendComp(destDirNames[x]);

			if (dest->fName[0])
				theTrail.AppendComp(ZString::sFromPString(dest->fName));

			if (oErr)
				*oErr = ZFile::errorNone;
			return theTrail;
			}
		else
			{
			// We're on different volumes, or one is pointing at the root.

			// Figure out how many steps from source to root.
			ZTrail theTrail;
			long sourceDirID = fDirID;
			while (sourceDirID != fsRtParID)
				{
				theTrail.AppendBounce();
				sourceDirID = ::sGetParentDirID(fVRefNum, sourceDirID, oErr);
				if (sourceDirID == 0)
					return ZTrail(false);
				}
			if (fName[0])
				theTrail.AppendBounce();

			// Build the list of names from dest to root.
			vector<string> destDirNames;
			long destDirID = dest->fDirID;
			while (destDirID != fsRtParID)
				{
				if (destDirID == fsRtDirID)
					{
					StrFileName destName;
					Threaded_HParamBlockRec thePB;
					thePB.fRealPB.volumeParam.ioNamePtr = &destName[0];
					thePB.fRealPB.volumeParam.ioVRefNum = dest->fVRefNum;
					thePB.fRealPB.volumeParam.ioVolIndex = 0;

					if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
						return ZTrail(false);

					destDirNames.push_back(ZString::sFromPString(destName));
					destDirID = fsRtParID;
					}
				else
					{
					string destName;
					destDirID = sGetParentDirID(fVRefNum, destDirID, destName, oErr);
					if (destDirID == 0)
						return ZTrail(false);
					destDirNames.push_back(destName);
					}
				}
			for (int x = destDirNames.size(); x > 0; --x)
				theTrail.AppendComp(destDirNames[x - 1]);

			if (dest->fName[0])
				theTrail.AppendComp(ZString::sFromPString(dest->fName));

			if (oErr)
				*oErr = ZFile::errorNone;
			return theTrail;
			}
		}
	if (oErr)
		*oErr = ZFile::errorGeneric;
	return ZTrail(false);
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::GetParent(ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fName[0])
		{
		// Cases 2, 4, 6 we've got a name, just strip it off.
		return new ZFileLoc_Mac_FSSpec(fVRefNum, fDirID);
		}

	if (fDirID == fsRtParID)
		{
		// Case 1 -- we reference the root of the file system, our parent is nil.
		return ZRef<ZFileLoc>();
		}
	else if (fDirID == fsRtDirID)
		{
		// Case 3 -- we reference the top level directory of the volume
		// fVRefNum, so the branch is the root.
		return new ZFileLoc_Mac_FSSpec(0, fsRtParID);
		}
	else
		{
		// Case 5 -- We're referencing the directory with id fDirID on
		// volume fVRefNum. The branch is therefore the parent of this
		// directory.
		Threaded_CInfoPBRec thePB;
		thePB.fRealPB.dirInfo.ioNamePtr = nil;
		thePB.fRealPB.dirInfo.ioFDirIndex = -1;
		thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
		thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

		if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
			return ZRef<ZFileLoc>();

		return new ZFileLoc_Mac_FSSpec(fVRefNum, thePB.fRealPB.dirInfo.ioDrParID);
		}
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::GetDescendant(const string* iComps, size_t iCount, ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (!iCount)
		return this;

	StrFileName curComponent;
	size_t nextComponentIndex;
	if (fName[0])
		{
		sCopyPString(fName, curComponent, sizeof(curComponent));
		nextComponentIndex = 0;
		}
	else
		{
		ZString::sToPString(iComps[0], curComponent, sizeof(curComponent) - 1);
		nextComponentIndex = 1;
		}

	long theDirID = fDirID;
	short theVRefNum = fVRefNum;

	for (;;)
		{
		// Cases 1, 3 & 5 do not occur -- we always have a name to use (in curComponent).
		if (theDirID == fsRtParID)
			{
			// Case 2: We're pointing at the top level directory on volume named curCompnent.
			if (!::sVolNameToRefNum(&curComponent[0], theVRefNum, oErr))
				{
				if (nextComponentIndex == iCount)
					return new ZFileLoc_Mac_FSSpec(0, fsRtParID, curComponent);
				return ZRef<ZFileLoc>();
				}
			theDirID = fsRtDirID;
			}
		else
			{
			// Case 4 & 6.
			Threaded_CInfoPBRec thePB;
			thePB.fRealPB.dirInfo.ioNamePtr = &curComponent[0];
			thePB.fRealPB.dirInfo.ioFDirIndex = 0;
			thePB.fRealPB.dirInfo.ioVRefNum = theVRefNum;
			thePB.fRealPB.dirInfo.ioDrDirID = theDirID;

			if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
				{
				// We didn't find anything called curComponent in theDirID.
				if (nextComponentIndex == iCount)
					{
					// We'd reached the last component, so we can return
					// a file loc, it just points to a non-existent entity.
					return new ZFileLoc_Mac_FSSpec(theVRefNum, theDirID, curComponent);
					}
				return ZRef<ZFileLoc>();
				}

			// We found something.
			if (thePB.fRealPB.hFileInfo.ioFlAttrib & kioFlAttribDirMask)
				{
				// It's a directory.
				ZAssertStop(kDebug_File_Mac, thePB.fRealPB.dirInfo.ioDrParID == theDirID);
				theDirID = thePB.fRealPB.dirInfo.ioDrDirID;
				}
			else if (thePB.fRealPB.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
				{
				// It's an alias.
				FSSpec theSpec;
				theSpec.vRefNum = theVRefNum;
				theSpec.parID = theDirID;
				sCopyPString(curComponent, theSpec.name, sizeof(theSpec.name));

				Boolean targetIsFolder;
				Boolean wasAliased;
				OSErr result = ::ResolveAliasFileWithMountFlags(&theSpec, true, &targetIsFolder, &wasAliased, kResolveAliasFileNoUI);
				if (wasAliased && noErr == result)
					{
					if (nextComponentIndex == iCount)
						{
						// We've sucked up the whole path, return the target of
						// the alias as the result.
						return new ZFileLoc_Mac_FSSpec(theSpec.vRefNum, theSpec.parID, theSpec.name);
						}
					else
						{
						if (targetIsFolder)
							{
							theVRefNum = theSpec.vRefNum;
							if (theSpec.parID == fsRtParID)
								{
								// We're referencing the top level of a volume, but we know what its
								// vrefnum is and don't want to re-resolve the name (possibly to a different
								// volume). So rejig values to get us into case 4 and let things take
								// their normal course.
								theDirID = fsRtDirID;
								}
							else
								{
								// We're referencing a folder within a volume. The current component has
								// been consumed, but we don't want to move on to the next. Instead
								// transcribe the values returned in theSpec into our controlling
								// variables and branch back to the top of the loop.
								theDirID = theSpec.parID;
								sCopyPString(theSpec.name, curComponent, sizeof(curComponent));
								continue;
								}
							}
						else
							{
							// We've got more components to resolve, and the target was not a folder,
							// so we've failed to physicalize the whole path.
							return ZRef<ZFileLoc>();
							}
						}
					}
				else
					{
					// The alias either didn't resolve or wasn't actually an alias.
					if (nextComponentIndex == iCount)
						{
						// We've sucked up the whole path so return a spec
						// referencing the alias itself and trust the other uses of it to spot the
						// failure or for the entity to show up later.
						return new ZFileLoc_Mac_FSSpec(theVRefNum, theDirID, curComponent);
						}
					else
						{
						// We have components remaining, and the current component is
						// neither a directory nor a working alias to a directory, so we've
						// failed to pyhsicalize the whole path.
						return ZRef<ZFileLoc>();
						}
					}
				}
			else
				{
				// It's a file.
				if (nextComponentIndex == iCount)
					{
					// We've sucked up the whole path and are now pointing at a file.
					return new ZFileLoc_Mac_FSSpec(theVRefNum, theDirID, curComponent);
					}
				else
					{
					// We've encountered a file but there are still components in the path,
					// so we've failed to pyhsicalize the whole path.
					return ZRef<ZFileLoc>();
					}
				}
			}

		if (nextComponentIndex == iCount)
			break;

		ZString::sToPString(iComps[nextComponentIndex], curComponent, sizeof(curComponent) - 1);
		++nextComponentIndex;
		}
	return new ZFileLoc_Mac_FSSpec(theVRefNum, theDirID);
	}

bool ZFileLoc_Mac_FSSpec::IsRoot()
	{ return fName[0] == 0 && fDirID == fsRtParID; }

string ZFileLoc_Mac_FSSpec::AsString_POSIX(const string* iComps, size_t iCount)
	{
	string result = this->pAsString('/', iComps, iCount);
	if (result.size())
		{
		result.erase(result.size() - 1, 1);
		return "/" + result;
		}
	else
		{
		return "/";
		}
	}

string ZFileLoc_Mac_FSSpec::AsString_Native(const string* iComps, size_t iCount)
	{
	string result = this->pAsString(':', iComps, iCount);
	if (result.size())
		result.erase(result.size() - 1, 1);
	return result;
	}

ZFile::Kind ZFileLoc_Mac_FSSpec::Kind(ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fDirID == fsRtParID)
		{
		if (fName[0])
			{
			// Case 2. The top level directory of the volume titled fName.
			short theVRefNum;
			if (!::sVolNameToRefNum(&fName[0], theVRefNum, oErr))
				return ZFile::kindNone;

			return ZFile::kindDir;
			}
		else
			{
			// Case 1 -- we reference the root of the file system.
			return ZFile::kindDir;
			}
		}

	if (fName[0])
		{
		// Cases 4 and 6, the file/directory named fName in directory fDir of volume fVRefNum
		FSSpec theSpec;
		sCopyPString(fName, theSpec.name, sizeof(theSpec.name));

		Threaded_CInfoPBRec thePB;
		thePB.fRealPB.dirInfo.ioNamePtr = &theSpec.name[0];
		thePB.fRealPB.dirInfo.ioFDirIndex = 0;
		thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
		thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

		if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
			return ZFile::kindNone;

		if (thePB.fRealPB.hFileInfo.ioFlAttrib & kioFlAttribDirMask)
			{
			return ZFile::kindDir;
			}
		else if (thePB.fRealPB.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
			{
			// It's an alias. theSpec already has our name in it.
			theSpec.vRefNum = fVRefNum;
			theSpec.parID = fDirID;

			Boolean targetIsFolder;
			Boolean wasAliased;
			OSErr result = ::ResolveAliasFileWithMountFlags(&theSpec, true, &targetIsFolder, &wasAliased, kResolveAliasFileNoUI);
			if (noErr == result)
				{
				// We take it on faith that the value of targetIsFolder is good.
				if (targetIsFolder)
					return ZFile::kindDir;
				else
					return ZFile::kindFile;
				}
			else
				{
				if (wasAliased)
					return ZFile::kindLink;
				else
					return ZFile::kindFile;
				}
			}
		else
			{
			// It must be a file.
			return ZFile::kindFile;
			}
		}
	else
		{
		if (fDirID == fsRtDirID)
			{
			// Case 3. The top level directory of volume fVRefNum.
			Threaded_HParamBlockRec thePB;
			thePB.fRealPB.volumeParam.ioNamePtr = nil;
			thePB.fRealPB.volumeParam.ioVRefNum = fVRefNum;
			thePB.fRealPB.volumeParam.ioVolIndex = 0;

			if (!PBINVOKECHECKED(HGetVInfo, thePB, oErr))
				return ZFile::kindNone;

			return ZFile::kindDir;
			}
		else
			{
			// Case 5. The directory fDirID of volume fVRefNum.
			StrFileName returnedName;
			returnedName[0] = 0;

			Threaded_CInfoPBRec thePB;
			thePB.fRealPB.dirInfo.ioNamePtr = &returnedName[0];
			thePB.fRealPB.dirInfo.ioFDirIndex = -1;
			thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
			thePB.fRealPB.dirInfo.ioDrDirID = fDirID;

			if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
				return ZFile::kindNone;

			if (thePB.fRealPB.hFileInfo.ioFlAttrib & kioFlAttribDirMask)
				return ZFile::kindDir;

			// Hmmm. It's got to be a directory, we only gave the OS the vrefnum
			// and the directory ID.
			ZUnimplemented();
			return ZFile::kindFile;
			}
		}
	}

uint64 ZFileLoc_Mac_FSSpec::Size(ZFile::Error* oErr)
	{
	Threaded_HParamBlockRec thePB;
	if (!sGet_FInfo(fVRefNum, fDirID, fName, thePB, oErr))
		return 0;

	return thePB.fRealPB.fileParam.ioFlRLgLen;
	}

ZTime ZFileLoc_Mac_FSSpec::TimeCreated(ZFile::Error* oErr)
	{
	Threaded_CInfoPBRec thePB;
	if (!sGet_CInfo(fVRefNum, fDirID, fName, thePB, oErr))
		return ZTime();

	return sAsZTime(thePB.fRealPB.hFileInfo.ioFlCrDat);
	}

ZTime ZFileLoc_Mac_FSSpec::TimeModified(ZFile::Error* oErr)
	{
	Threaded_CInfoPBRec thePB;
	if (!sGet_CInfo(fVRefNum, fDirID, fName, thePB, oErr))
		return ZTime();

	return sAsZTime(thePB.fRealPB.hFileInfo.ioFlMdDat);
	}

bool ZFileLoc_Mac_FSSpec::SetCreatorAndType(uint32 iCreator, uint32 iType, ZFile::Error* oErr)
	{
	if (!fName[0] || fDirID == fsRtParID)
		{
		if (oErr)
			*oErr = ZFile::errorWrongTypeForOperation;
		return false;
		}

	StrFileName tempName;
	sCopyPString(fName, tempName, sizeof(tempName));

	Threaded_CInfoPBRec thePB;
	thePB.fRealPB.hFileInfo.ioNamePtr = &tempName[0];
	thePB.fRealPB.hFileInfo.ioVRefNum = fVRefNum;
	thePB.fRealPB.hFileInfo.ioFVersNum = 0;
	thePB.fRealPB.hFileInfo.ioDirID = fDirID;
	thePB.fRealPB.hFileInfo.ioFDirIndex = 0;

	if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
		return false;

	if ((thePB.fRealPB.hFileInfo.ioFlAttrib & ioDirMask) != 0)
		return ZFile::errorWrongTypeForOperation;

	if (iCreator != 0)
		thePB.fRealPB.hFileInfo.ioFlFndrInfo.fdCreator = iCreator;
	if (iType != 0)
		thePB.fRealPB.hFileInfo.ioFlFndrInfo.fdType = iType;
	thePB.fRealPB.hFileInfo.ioDirID = fDirID;

	if (!PBINVOKECHECKED(SetCatInfo, thePB, oErr))
		return false;

	// Now bump the mod date on the parent directory (our fVRefNum/fDirID).
	tempName[0] = 0;
	thePB.fRealPB.hFileInfo.ioNamePtr = &tempName[0];
	thePB.fRealPB.hFileInfo.ioVRefNum = fVRefNum;
	thePB.fRealPB.hFileInfo.ioFDirIndex = -1;
	thePB.fRealPB.hFileInfo.ioDirID = fDirID;
	if (!PBINVOKECHECKED(GetCatInfo, thePB, oErr))
		return false;

	unsigned long secs;
	::GetDateTime(&secs);
	// Set mod date to current date, or one second into the future if mod date == current date
	if (thePB.fRealPB.hFileInfo.ioFlMdDat == secs)
		thePB.fRealPB.hFileInfo.ioFlMdDat = secs + 1;
	else
		thePB.fRealPB.hFileInfo.ioFlMdDat = secs;

	if (!PBINVOKECHECKED(SetCatInfo, thePB, oErr))
		return false;

	if (oErr)
		*oErr = ZFile::errorNone;
	return true;
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::CreateDir(ZFile::Error* oErr)
	{
	if (fName[0] == 0)
		{
		// Without a name we reference a directory that already exists, or one that
		// did exist and no longer does.
		if (oErr)
			*oErr = ZFile::errorGeneric;
		return ZRef<ZFileLoc>();
		}

	if (fDirID == fsRtParID)
		{
		// We're pointing at the root, we can't create a directory here.
		if (oErr)
			*oErr = ZFile::errorGeneric;
		return ZRef<ZFileLoc>();
		}

	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.fileParam.ioNamePtr = &fName[0];
	thePB.fRealPB.fileParam.ioVRefNum = fVRefNum;
	thePB.fRealPB.fileParam.ioDirID = fDirID;

	if (!PBINVOKECHECKED(DirCreate, thePB, oErr))
		return ZRef<ZFileLoc>();

	if (oErr)
		*oErr = ZFile::errorNone;

	return new ZFileLoc_Mac_FSSpec(fVRefNum, thePB.fRealPB.fileParam.ioDirID);
	}

static bool sCanUsePBHMoveRename(short iVRefNum)
	{
	GetVolParmsInfoBuffer theInfoBuffer;
	Threaded_HParamBlockRec thePB;
	thePB.fRealPB.ioParam.ioNamePtr = nil;
	thePB.fRealPB.ioParam.ioVRefNum = iVRefNum;
	thePB.fRealPB.ioParam.ioBuffer = (Ptr)&theInfoBuffer;
	thePB.fRealPB.ioParam.ioReqCount = sizeof(theInfoBuffer);

	if (!PBINVOKECHECKED(HGetVolParms, thePB, nil))
		return false;

	return 0 != (theInfoBuffer.vMAttrib & (1L << bHasMoveRename));
	}

static ZRef<ZFileLoc> sMoveTo(short iVRefNum, long iSourceDirID, const unsigned char* iSourceName,
						long iDestDirID, const unsigned char* iDestName, ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (sComparePStrings(iSourceName, iDestName) == 0)
		{
		// The name is not changing.
		if (iSourceDirID == iDestDirID)
			{
			// The directory's not changing either, so just return.
			return new ZFileLoc_Mac_FSSpec(iVRefNum, iSourceDirID, iSourceName);
			}
		else
			{
			// We're changing directories but not names, we can use PBCatMove.
			Threaded_CMovePBRec thePB;
			thePB.fRealPB.ioVRefNum = iVRefNum;
			thePB.fRealPB.ioDirID = iSourceDirID;
			thePB.fRealPB.ioNamePtr = const_cast<unsigned char*>(iSourceName);
			thePB.fRealPB.ioNewDirID = iDestDirID;
			thePB.fRealPB.ioNewName = nil;

			if (!PBINVOKECHECKED(CatMove, thePB, oErr))
				return ZRef<ZFileLoc>();

			return new ZFileLoc_Mac_FSSpec(iVRefNum, iDestDirID, iSourceName);
			}
		}
	else
		{
		// The name is changing.
		if (iSourceDirID == iDestDirID)
			{
			// The directory is not changing, we're only renaming.
			Threaded_HParamBlockRec thePB;
			thePB.fRealPB.fileParam.ioNamePtr = const_cast<unsigned char*>(iSourceName);
			thePB.fRealPB.fileParam.ioVRefNum = iVRefNum;
			thePB.fRealPB.fileParam.ioDirID = iSourceDirID;
			thePB.fRealPB.ioParam.ioMisc = (char*)iDestName;
			thePB.fRealPB.fileParam.ioFVersNum = 0;

			if (!PBINVOKECHECKED(HRename, thePB, oErr))
				return ZRef<ZFileLoc>();

			return new ZFileLoc_Mac_FSSpec(iVRefNum, iSourceDirID, iDestName);
			}
		else
			{
			// We're changing parent directory *and* name.
			if (::sCanUsePBHMoveRename(iVRefNum))
				{
				// We can use PBHMoveRename on this volume (it's an AppleShare volume,
				// or a local volume with file sharing enabled and thus the AS APIs are active).
				Threaded_HParamBlockRec thePB;
				thePB.fRealPB.fileParam.ioNamePtr = const_cast<unsigned char*>(iSourceName);
				thePB.fRealPB.fileParam.ioVRefNum = iVRefNum;
				thePB.fRealPB.fileParam.ioDirID = iSourceDirID;
				thePB.fRealPB.copyParam.ioNewDirID = iDestDirID;
				thePB.fRealPB.copyParam.ioNewName = nil;
				thePB.fRealPB.copyParam.ioCopyName = const_cast<unsigned char*>(iDestName);

				if (!PBINVOKECHECKED(HMoveRename, thePB, oErr))
					return ZRef<ZFileLoc>();

				return new ZFileLoc_Mac_FSSpec(iVRefNum, iDestDirID, iDestName);
				}
			else
				{
				// Ho hum, the tricky one. We're changing parent directory *and* name
				// but the volume does not support it directly. So we have to do each
				// as a separate operation. There is a potential problem situation
				// where we have file A in directory X to be moved to file B in directory Y.
				// If we have a file called A in directory Y we can't move then rename. If we
				// have a file called B in directory X we can't rename then move.
				// The solution used by Apple's MoreFiles is to move the file into
				// a uniquely named temporary folder, rename the file, then move the file
				// on to the final destination (and delete the temp folder).

				long tempDirID;
				OSErr err = ::FindFolder(iVRefNum, kTemporaryFolderType, kCreateFolder,
									&iVRefNum, &tempDirID);
				if (noErr != err)
					{
					if (oErr)
						*oErr = ::sTranslateError(err);
					}
				else
					{
					// Create a new uniquely named directory in the temporary directory.
					if (long uniqueDirID = ::sCreateUniqueDir(iVRefNum, tempDirID, oErr))
						{
						Threaded_HParamBlockRec theHPB;
						Threaded_CMovePBRec theCPB;

						// Move our file into the uniquely named directory.
						theCPB.fRealPB.ioVRefNum = iVRefNum;
						theCPB.fRealPB.ioDirID = iSourceDirID;
						theCPB.fRealPB.ioNamePtr = const_cast<unsigned char*>(iSourceName);
						theCPB.fRealPB.ioNewDirID = uniqueDirID;
						theCPB.fRealPB.ioNewName = nil;

						if (!PBINVOKECHECKED(CatMove, theCPB, oErr))
							{
							// The file's in the temp location, rename it.
							theHPB.fRealPB.fileParam.ioVRefNum = iVRefNum;
							theHPB.fRealPB.fileParam.ioDirID = uniqueDirID;
							theHPB.fRealPB.fileParam.ioNamePtr = const_cast<unsigned char*>(iSourceName);
							theHPB.fRealPB.ioParam.ioMisc = (char*)iDestName;
							theHPB.fRealPB.fileParam.ioFVersNum = 0;

							if (PBINVOKECHECKED(HRename, theHPB, oErr))
								{
								// It has the new name in the temp location, try to move it to the new location.
								theCPB.fRealPB.ioVRefNum = iVRefNum;
								theCPB.fRealPB.ioDirID = uniqueDirID;
								theCPB.fRealPB.ioNamePtr = const_cast<unsigned char*>(iDestName);
								theCPB.fRealPB.ioNewDirID = iDestDirID;
								theCPB.fRealPB.ioNewName = nil;

								if (PBINVOKECHECKED(CatMove, theCPB, oErr))
									{
									// Success, remove the unique directory and return.
									theHPB.fRealPB.ioParam.ioNamePtr = nil;
									theHPB.fRealPB.ioParam.ioVRefNum = iVRefNum;
									theHPB.fRealPB.fileParam.ioDirID = uniqueDirID;

									PBINVOKE(HDelete, theHPB);

									return new ZFileLoc_Mac_FSSpec(iVRefNum, iDestDirID, iDestName);
									}

								// Change the name back.
								theHPB.fRealPB.fileParam.ioNamePtr = const_cast<unsigned char*>(iDestName);
								theHPB.fRealPB.fileParam.ioVRefNum = iVRefNum;
								theHPB.fRealPB.fileParam.ioDirID = uniqueDirID;
								theHPB.fRealPB.ioParam.ioMisc = (char*)iSourceName;
								theHPB.fRealPB.fileParam.ioFVersNum = 0;

								PBINVOKE(HRename, theHPB);
								}

							// Move it back to its original directory.
							theCPB.fRealPB.ioDirID = uniqueDirID;
							theCPB.fRealPB.ioNamePtr = const_cast<unsigned char*>(iSourceName);
							theCPB.fRealPB.ioNewDirID = iSourceDirID;

							PBINVOKE(CatMove, theCPB);
							}

						// Remove the unique directory.
						theHPB.fRealPB.ioParam.ioNamePtr = nil;
						theHPB.fRealPB.ioParam.ioVRefNum = iVRefNum;
						theHPB.fRealPB.fileParam.ioDirID = uniqueDirID;

						PBINVOKE(HDelete, theHPB);
						}
					}
				}
			}
		}
	return ZRef<ZFileLoc>();
	}

ZRef<ZFileLoc> ZFileLoc_Mac_FSSpec::MoveTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr)
	{
	// MoveTo is rather complicated for MacOS. We've got three different scenarios:
	//   1. Changing the name of something.
	//   2. Moving something to a different directory.
	//   3. Both renaming and moving something.
	// We've also got the complication that a source directory can be specified in two ways.
	//   1. As a parent directory ID and a name within that directory.
	//   2. As a directory ID without a name.
	// We also have to watch out for moving between volumes, and attempting to operate
	// on the top level directory of a volume (ie the volume itself). So we do a bunch
	// of filtering to catch the unsupported cases, and then determine if we're
	// renaming/moving/both.

	ZFileLoc_Mac_FSSpec* dest = ZRefDynamicCast<ZFileLoc_Mac_FSSpec>(iDest);
	if (!dest)
		{
		if (oErr)
			*oErr = ZFile::errorInvalidFileSpec;
		return ZRef<ZFileLoc>();
		}

	if (dest->fName[0] == 0)
		{
		// Without a destination name we must be referring to a directory
		// that already exists, and thus can't be moved on top of, or a directory
		// that did exist and doesn't anymore, but for which we no longer know the name.
		if (oErr)
			*oErr = ZFile::errorInvalidFileSpec;
		return ZRef<ZFileLoc>();
		}

	if (fDirID == fsRtParID)
		{
		// Our source is the root or a volume.
		if (fName[0] == 0)
			{
			// It's the root ... can't be moved.
			if (oErr)
				*oErr = ZFile::errorWrongTypeForOperation;
			return ZRef<ZFileLoc>();
			}
		else
			{
			// Source is a volume called fName.
			if (dest->fDirID == fsRtParID)
				{
				// We're renaming the source volume. There is an ambiguity here, dest could
				// be referring by name to an extant volume, and thus the move should fail
				// if the caller is not aware that they're dealing with volumes and thus
				// expecting the name clash to be a problem. But MacOS supports multiple
				// volumes with the same name and this rename operation will thus not
				// fail because a volume with the dest name is mounted. It may fail for
				// other reasons of course.
				return sChangeVolumeName(fName, dest->fName, oErr);
				}
			else
				{
				// Can't move a volume to anything other than a new volume name.
				if (oErr)
					*oErr = ZFile::errorWrongTypeForOperation;
				return ZRef<ZFileLoc>();
				}
			}
		// Can't get here.
		ZUnimplemented();
		}
	else if (fDirID == fsRtDirID && fName[0] == 0)
		{
		// Source is top level directory on volume fVRefNum ie it *is* the volume.
		if (dest->fDirID == fsRtParID)
			{
			// We're renaming the source volume.
			return sChangeVolumeName(fVRefNum, dest->fName, oErr);
			}
		else if (dest->fDirID == fsRtDirID)
			{
			// Destination is the child of some top level directory.
			if (oErr)
				*oErr = ZFile::errorWrongTypeForOperation;
			return ZRef<ZFileLoc>();
			}
		}

	// To have got this far the source must be some normal directory or file.
	if (dest->fDirID == fsRtParID)
		{
		// The destination is a named volume.
		if (oErr)
			*oErr = ZFile::errorWrongTypeForOperation;
		return ZRef<ZFileLoc>();
		}

	// The destination is also some normal directory or file, however we can't move between volumes.
	if (fVRefNum != dest->fVRefNum)
		{
		if (oErr)
			*oErr = ZFile::errorGeneric;
		return ZRef<ZFileLoc>();
		}

	if (fName[0] == 0)
		{
		// Normalize the source -- we need a parent directory ID and current name
		// for sources in all cases.
		StrFileName sourceName;
		long parentID = sGetParentDirID(fVRefNum, fDirID, sourceName, oErr);
		if (parentID == 0)
			return ZRef<ZFileLoc>();
		return sMoveTo(fVRefNum, parentID, sourceName, dest->fDirID, dest->fName, oErr);
		}
	else
		{
		return sMoveTo(fVRefNum, fDirID, fName, dest->fDirID, dest->fName, oErr);
		}
	}

bool ZFileLoc_Mac_FSSpec::Delete(ZFile::Error* oErr)
	{
	if (fDirID == fsRtParID
		|| (fDirID == fsRtDirID && fName[0] == 0))
		{
		// Can't delete the root or a direct child of the root, obviously.
		if (oErr)
			*oErr = ZFile::errorGeneric;
		return false;
		}

	Threaded_HParamBlockRec thePB;
	if (fName[0])
		thePB.fRealPB.ioParam.ioNamePtr = &fName[0];
	else
		thePB.fRealPB.ioParam.ioNamePtr = nil;
	thePB.fRealPB.ioParam.ioVRefNum = fVRefNum;
	thePB.fRealPB.fileParam.ioDirID = fDirID;

	if (!PBINVOKECHECKED(HDelete, thePB, oErr))
		return false;
	return true;
	}

ZRef<ZStreamerRPos> ZFileLoc_Mac_FSSpec::OpenRPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, false, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;
	if (theError == ZFile::errorNone)
		return new ZStreamerRPos_File_MacOld(theRefNum, true);
	return ZRef<ZStreamerRPos>();
	}

ZRef<ZStreamerWPos> ZFileLoc_Mac_FSSpec::OpenWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, false, true, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;
	if (theError == ZFile::errorNone)
		return new ZStreamerWPos_File_MacOld(theRefNum, true);
	return ZRef<ZStreamerWPos>();
	}

ZRef<ZStreamerRWPos> ZFileLoc_Mac_FSSpec::OpenRWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, true, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;
	if (theError == ZFile::errorNone)
		return new ZStreamerRWPos_File_MacOld(theRefNum, true);
	return ZRef<ZStreamerRWPos>();
	}

ZRef<ZStreamerWPos> ZFileLoc_Mac_FSSpec::CreateWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	ZFile::Error theError = ::sCreate_Old(fVRefNum, fDirID, fName, iOpenExisting);
	if (theError == ZFile::errorNone)
		{
		SInt16 theRefNum;
		theError = ::sOpen_Old(fVRefNum, fDirID, fName, false, true, iPreventWriters, theRefNum);
		if (theError == ZFile::errorNone)
			{
			if (oErr)
				*oErr = ZFile::errorNone;
			return new ZStreamerWPos_File_MacOld(theRefNum, true);
			}
		}
	if (oErr)
		*oErr = theError;
	return ZRef<ZStreamerWPos>();
	}

ZRef<ZStreamerRWPos> ZFileLoc_Mac_FSSpec::CreateRWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	ZFile::Error theError = ::sCreate_Old(fVRefNum, fDirID, fName, iOpenExisting);
	if (theError == ZFile::errorNone)
		{
		SInt16 theRefNum;
		theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, true, iPreventWriters, theRefNum);
		if (theError == ZFile::errorNone)
			{
			if (oErr)
				*oErr = ZFile::errorNone;
			return new ZStreamerRWPos_File_MacOld(theRefNum, true);
			}
		}
	if (oErr)
		*oErr = theError;
	return ZRef<ZStreamerRWPos>();
	}

ZRef<ZFileR> ZFileLoc_Mac_FSSpec::OpenFileR(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, false, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;

	if (theError == ZFile::errorNone)
		return new ZFileR_MacOld(theRefNum, true);

	return ZRef<ZFileR>();
	}

ZRef<ZFileW> ZFileLoc_Mac_FSSpec::OpenFileW(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, false, true, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;

	if (theError == ZFile::errorNone)
		return new ZFileW_MacOld(theRefNum, true);

	return ZRef<ZFileW>();
	}

ZRef<ZFileRW> ZFileLoc_Mac_FSSpec::OpenFileRW(bool iPreventWriters, ZFile::Error* oErr)
	{
	SInt16 theRefNum;
	ZFile::Error theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, true, iPreventWriters, theRefNum);
	if (oErr)
		*oErr = theError;

	if (theError == ZFile::errorNone)
		return new ZFileRW_MacOld(theRefNum, true);

	return ZRef<ZFileRW>();
	}

ZRef<ZFileW> ZFileLoc_Mac_FSSpec::CreateFileW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	ZFile::Error theError = ::sCreate_Old(fVRefNum, fDirID, fName, iOpenExisting);
	if (theError == ZFile::errorNone)
		{
		SInt16 theRefNum;
		theError = ::sOpen_Old(fVRefNum, fDirID, fName, false, true, iPreventWriters, theRefNum);
		if (theError == ZFile::errorNone)
			{
			if (oErr)
				*oErr = ZFile::errorNone;

			return new ZFileW_MacOld(theRefNum, true);
			}
		}

	if (oErr)
		*oErr = theError;
	return ZRef<ZFileW>();
	}

ZRef<ZFileRW> ZFileLoc_Mac_FSSpec::CreateFileRW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	ZFile::Error theError = ::sCreate_Old(fVRefNum, fDirID, fName, iOpenExisting);
	if (theError == ZFile::errorNone)
		{
		SInt16 theRefNum;
		theError = ::sOpen_Old(fVRefNum, fDirID, fName, true, true, iPreventWriters, theRefNum);
		if (theError == ZFile::errorNone)
			{
			if (oErr)
				*oErr = ZFile::errorNone;
			return new ZFileRW_MacOld(theRefNum, true);
			}
		}

	if (oErr)
		*oErr = theError;
	return ZRef<ZFileRW>();
	}

void ZFileLoc_Mac_FSSpec::GetFSSpec(FSSpec& oSpec)
	{
	oSpec.vRefNum = fVRefNum;
	oSpec.parID = fDirID;
	sCopyPString(fName, oSpec.name, sizeof(fName));
	}

string ZFileLoc_Mac_FSSpec::pAsString(char iSeparator, const string* iComps, size_t iCount)
	{
	string theResult;

	long theDirID = fDirID;

	for (;;)
		{
		if (theDirID == fsRtParID)
			{
			// Case 1. We've topped out and can bail
			break;
			}
		else if (theDirID == fsRtDirID)
			{
			// Case 3: We're pointing at the top level directory on volume vRefNum.
			Str31 volumeName;
			Threaded_HParamBlockRec thePB;
			thePB.fRealPB.volumeParam.ioNamePtr = &volumeName[0]; // returns name
			thePB.fRealPB.volumeParam.ioVRefNum = fVRefNum;
			thePB.fRealPB.volumeParam.ioVolIndex = 0;

			if (PBINVOKECHECKED(HGetVInfo, thePB, nil))
				theResult = ZString::sFromPString(&volumeName[0]) + iSeparator + theResult;
			break;
			}
		else
			{
			// Case 5 -- We're referencing the directory with id theDirID on
			// volume fVRefNum. The branch is therefore the parent of this
			// directory.
			Str31 directoryName;
			Threaded_CInfoPBRec thePB;
			thePB.fRealPB.dirInfo.ioNamePtr = &directoryName[0];
			thePB.fRealPB.dirInfo.ioFDirIndex = -1;
			thePB.fRealPB.dirInfo.ioVRefNum = fVRefNum;
			thePB.fRealPB.dirInfo.ioDrDirID = theDirID;

			if (!PBINVOKECHECKED(GetCatInfo, thePB, nil))
				break;

			theResult = ZString::sFromPString(&directoryName[0]) + iSeparator + theResult;

			theDirID = thePB.fRealPB.dirInfo.ioDrParID;
			}
		}

	// By handling fName here we avoid cases 2, 4, 6 above.
	if (fName[0])
		theResult += ZString::sFromPString(&fName[0]) + iSeparator;

	for (size_t x = 0; x < iCount; ++x)
		{
		theResult += iComps[x];
		theResult += iSeparator;
		}

	return theResult;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileR_MacOld

ZFileR_MacOld::ZFileR_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileR_MacOld::~ZFileR_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

ZFile::Error ZFileR_MacOld::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{ return ::sReadAt_Old(fRefNum, iOffset, iDest, iCount, oCountRead); }

ZFile::Error ZFileR_MacOld::GetSize(uint64& oSize)
	{ return ::sGetSize_Old(fRefNum, oSize); }

// =================================================================================================
#pragma mark -
#pragma mark * ZFileW_MacOld

ZFileW_MacOld::ZFileW_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileW_MacOld::~ZFileW_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

ZFile::Error ZFileW_MacOld::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{ return ::sWriteAt_Old(fRefNum, iOffset, iSource, iCount, oCountWritten); }

ZFile::Error ZFileW_MacOld::GetSize(uint64& oSize)
	{ return ::sGetSize_Old(fRefNum, oSize); }

ZFile::Error ZFileW_MacOld::SetSize(uint64 iSize)
	{ return ::sSetSize_Old(fRefNum, iSize); }

ZFile::Error ZFileW_MacOld::Flush()
	{ return ::sFlush_Old(fRefNum); }

ZFile::Error ZFileW_MacOld::FlushVolume()
	{ return ::sFlushVolume_Old(fRefNum); }

// =================================================================================================
#pragma mark -
#pragma mark * ZFileRW_MacOld

ZFileRW_MacOld::ZFileRW_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileRW_MacOld::~ZFileRW_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

ZFile::Error ZFileRW_MacOld::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{ return ::sReadAt_Old(fRefNum, iOffset, iDest, iCount, oCountRead); }

ZFile::Error ZFileRW_MacOld::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{ return ::sWriteAt_Old(fRefNum, iOffset, iSource, iCount, oCountWritten); }

ZFile::Error ZFileRW_MacOld::GetSize(uint64& oSize)
	{ return ::sGetSize_Old(fRefNum, oSize); }

ZFile::Error ZFileRW_MacOld::SetSize(uint64 iSize)
	{ return ::sSetSize_Old(fRefNum, iSize); }

ZFile::Error ZFileRW_MacOld::Flush()
	{ return ::sFlush_Old(fRefNum); }

ZFile::Error ZFileRW_MacOld::FlushVolume()
	{ return ::sFlushVolume_Old(fRefNum); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_File_MacOld

ZStreamRPos_File_MacOld::ZStreamRPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized),
	fPosition(0)
	{}

ZStreamRPos_File_MacOld::~ZStreamRPos_File_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

void ZStreamRPos_File_MacOld::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	size_t countRead;
	::sReadAt_Old(fRefNum, fPosition, iDest, iCount, &countRead);
	fPosition += countRead;
	if (oCountRead)
		*oCountRead = countRead;
	}

uint64 ZStreamRPos_File_MacOld::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRPos_File_MacOld::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamRPos_File_MacOld::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize_Old(fRefNum, theSize))
		return theSize;
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_File_MacOld

ZStreamerRPos_File_MacOld::ZStreamerRPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fStream(iRefNum, iCloseWhenFinalized)
	{}

ZStreamerRPos_File_MacOld::~ZStreamerRPos_File_MacOld()
	{}

const ZStreamRPos& ZStreamerRPos_File_MacOld::GetStreamRPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_File_MacOld

ZStreamWPos_File_MacOld::ZStreamWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized),
	fPosition(0)
	{}

ZStreamWPos_File_MacOld::~ZStreamWPos_File_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

void ZStreamWPos_File_MacOld::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	size_t countWritten;
	::sWriteAt_Old(fRefNum, fPosition, iSource, iCount, &countWritten);
	fPosition += countWritten;
	if (oCountWritten)
		*oCountWritten = countWritten;
	}

void ZStreamWPos_File_MacOld::Imp_Flush()
	{ ::sFlush_Old(fRefNum); }

uint64 ZStreamWPos_File_MacOld::Imp_GetPosition()
	{ return fPosition; }

void ZStreamWPos_File_MacOld::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamWPos_File_MacOld::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize_Old(fRefNum, theSize))
		return theSize;
	return 0;
	}

void ZStreamWPos_File_MacOld::Imp_SetSize(uint64 iSize)
	{
	if (iSize != size_t(iSize))
		sThrowBadSize();
	if (ZFile::errorNone != ::sSetSize_Old(fRefNum, iSize))
		sThrowBadSize();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos_File_MacOld

ZStreamerWPos_File_MacOld::ZStreamerWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fStream(iRefNum, iCloseWhenFinalized)
	{}

ZStreamerWPos_File_MacOld::~ZStreamerWPos_File_MacOld()
	{}

const ZStreamWPos& ZStreamerWPos_File_MacOld::GetStreamWPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_File_MacOld

ZStreamRWPos_File_MacOld::ZStreamRWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fRefNum(iRefNum),
	fCloseWhenFinalized(iCloseWhenFinalized),
	fPosition(0)
	{}

ZStreamRWPos_File_MacOld::~ZStreamRWPos_File_MacOld()
	{
	if (fCloseWhenFinalized)
		::sCloseRefNum_Old(fRefNum);
	}

void ZStreamRWPos_File_MacOld::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	size_t countRead;
	::sReadAt_Old(fRefNum, fPosition, iDest, iCount, &countRead);
	fPosition += countRead;
	if (oCountRead)
		*oCountRead = countRead;
	}

void ZStreamRWPos_File_MacOld::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	size_t countWritten;
	::sWriteAt_Old(fRefNum, fPosition, iSource, iCount, &countWritten);
	fPosition += countWritten;
	if (oCountWritten)
		*oCountWritten = countWritten;
	}

void ZStreamRWPos_File_MacOld::Imp_Flush()
	{ ::sFlush_Old(fRefNum); }

uint64 ZStreamRWPos_File_MacOld::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRWPos_File_MacOld::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamRWPos_File_MacOld::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize_Old(fRefNum, theSize))
		return theSize;
	return 0;
	}

void ZStreamRWPos_File_MacOld::Imp_SetSize(uint64 iSize)
	{
	if (iSize != size_t(iSize))
		sThrowBadSize();
	if (ZFile::errorNone != ::sSetSize_Old(fRefNum, iSize))
		sThrowBadSize();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos_File_MacOld

ZStreamerRWPos_File_MacOld::ZStreamerRWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized)
:	fStream(iRefNum, iCloseWhenFinalized)
	{}

ZStreamerRWPos_File_MacOld::~ZStreamerRWPos_File_MacOld()
	{}

const ZStreamRWPos& ZStreamerRWPos_File_MacOld::GetStreamRWPos()
	{ return fStream; }

#endif // ZCONFIG_API_Enabled(File_MacClassic)
