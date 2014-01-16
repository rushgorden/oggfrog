static const char rcsid[] = "@(#) $Id: ZFile_Win.cpp,v 1.34 2006/08/02 21:44:36 agreen Exp $";

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

#include "ZFile_Win.h"

#if ZCONFIG_API_Enabled(File_Win)

#include "ZWinHeader.h"

#include <cctype>

using std::find;
using std::string;
using std::vector;

#define kDebug_File_Win 2
/*!\file
fBase is either a drive letter plus colon or a UNC path.
fComps form the path from fBase to the entity we're interested in.
If fBase is empty then fComps must also be empty and we represent the root.
*/

// =================================================================================================
#pragma mark -
#pragma mark * Shared implementation details

static uint64 sAsUInt64(uint32 iHigh, uint32 iLow)
	{
	return (uint64(iHigh) << 32) + iLow;
	}

static ZFile::Error sTranslateError(DWORD iNativeError)
	{
	ZFile::Error theErr = ZFile::errorGeneric;
	switch (iNativeError)
		{
		case 0:
			theErr = ZFile::errorNone;
			break;
		}
	return theErr;
	}

static HANDLE sCreate(const string& iPath, bool iOpenExisting, bool iRead, bool iPreventWriters, ZFile::Error* oErr)
	{
	DWORD realAccess = GENERIC_WRITE;
	DWORD realSharing = FILE_SHARE_READ;

	if (iRead)
		realAccess |= GENERIC_READ;

	if (!iPreventWriters)
		realSharing |= FILE_SHARE_WRITE;

	DWORD realFlags = FILE_ATTRIBUTE_NORMAL;

	HANDLE theFileHANDLE = ::CreateFileA(iPath.c_str(), // the path
									realAccess, realSharing,
									nil, // No security attributes
									iOpenExisting ? OPEN_ALWAYS : CREATE_NEW,
									realFlags,
									nil);// No template file

	if (oErr)
		{
		if (theFileHANDLE == INVALID_HANDLE_VALUE)
			*oErr = ::sTranslateError(::GetLastError());
		else
			*oErr = ZFile::errorNone;
		}

	return theFileHANDLE;
	}

static HANDLE sCreateNT(const string16& iPath, bool iOpenExisting, bool iRead, bool iPreventWriters, ZFile::Error* oErr)
	{
	DWORD realAccess = GENERIC_WRITE;
	DWORD realSharing = FILE_SHARE_READ;

	if (iRead)
		realAccess |= GENERIC_READ;

	if (!iPreventWriters)
		realSharing |= FILE_SHARE_WRITE;

	DWORD realFlags = FILE_ATTRIBUTE_NORMAL;

	HANDLE theFileHANDLE = ::CreateFileW(iPath.c_str(), // the path
									realAccess, realSharing,
									nil, // No security attributes
									iOpenExisting ? OPEN_ALWAYS : CREATE_NEW,
									realFlags,
									nil);// No template file

	if (oErr)
		{
		if (theFileHANDLE == INVALID_HANDLE_VALUE)
			*oErr = ::sTranslateError(::GetLastError());
		else
			*oErr = ZFile::errorNone;
		}

	return theFileHANDLE;
	}

static HANDLE sOpen(const string& iPath, bool iRead, bool iWrite, bool iPreventWriters, ZFile::Error* oErr)
	{
	DWORD realAccess = 0;
	DWORD realSharing = FILE_SHARE_READ;

	if (iRead)
		realAccess |= GENERIC_READ;

	if (!iPreventWriters)
		realSharing |= FILE_SHARE_WRITE;

	if (iWrite)
		realAccess |= GENERIC_WRITE;

	DWORD realFlags = FILE_ATTRIBUTE_NORMAL;

	HANDLE theFileHANDLE = ::CreateFileA(iPath.c_str(), // the path
									realAccess, realSharing,
									nil, // No security attributes
									OPEN_EXISTING, // Open the file only if it exists
									realFlags,
									nil);// No template file

	if (oErr)
		{
		if (theFileHANDLE == INVALID_HANDLE_VALUE)
			*oErr = ::sTranslateError(::GetLastError());
		else
			*oErr = ZFile::errorNone;
		}

	return theFileHANDLE;
	}

static HANDLE sOpenNT(const string16& iPath, bool iRead, bool iWrite, bool iPreventWriters, ZFile::Error* oErr)
	{
	DWORD realAccess = 0;
	DWORD realSharing = FILE_SHARE_READ;

	if (iRead)
		realAccess |= GENERIC_READ;

	if (!iPreventWriters)
		realSharing |= FILE_SHARE_WRITE;

	if (iWrite)
		realAccess |= GENERIC_WRITE;

	DWORD realFlags = FILE_ATTRIBUTE_NORMAL;

	HANDLE theFileHANDLE = ::CreateFileW(iPath.c_str(), // the path
									realAccess, realSharing,
									nil, // No security attributes
									OPEN_EXISTING, // Open the file only if it exists
									realFlags,
									nil);// No template file

	if (oErr)
		{
		if (theFileHANDLE == INVALID_HANDLE_VALUE)
			*oErr = ::sTranslateError(::GetLastError());
		else
			*oErr = ZFile::errorNone;
		}

	return theFileHANDLE;
	}

static ZFile::Error sCloseFileHANDLE(HANDLE iFileHANDLE)
	{
	::CloseHandle(iFileHANDLE);
	return ZFile::errorNone;
	}

static ZFile::Error sRead(HANDLE iFileHANDLE, void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (oCountRead)
		*oCountRead = 0;

	if (iCount == 0) // Avoid Win32 null read
		return ZFile::errorNone;

	DWORD countRead = 0;
	if (!::ReadFile(iFileHANDLE, iDest, iCount, &countRead, nil))
		return ::sTranslateError(::GetLastError());

	if (oCountRead)
		*oCountRead = countRead;

	return ZFile::errorNone;
	}

static ZFile::Error sWrite(HANDLE iFileHANDLE, const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (oCountWritten)
		*oCountWritten = 0;

	if (iCount == 0)	// Avoid Win32 null write
		return ZFile::errorNone;

	DWORD countWritten = 0;
	if (!::WriteFile(iFileHANDLE, iSource, iCount, &countWritten, nil))
		return ::sTranslateError(::GetLastError());

	if (oCountWritten)
		*oCountWritten = countWritten;

	return ZFile::errorNone;
	}

static ZFile::Error sReadAt(HANDLE iFileHANDLE, uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (oCountRead)
		*oCountRead = 0;

	OVERLAPPED theOVERLAPPED;
	theOVERLAPPED.Internal = 0;
	theOVERLAPPED.InternalHigh = 0;
	theOVERLAPPED.Offset = iOffset;
	theOVERLAPPED.OffsetHigh = iOffset >> 32;
	theOVERLAPPED.hEvent = 0;
	DWORD countRead = 0;
	if (!::ReadFile(iFileHANDLE, iDest, iCount, &countRead, &theOVERLAPPED))
		{
		int err = ::GetLastError();
		if (err != ERROR_IO_PENDING)
			return ::sTranslateError(err);

		if (!::GetOverlappedResult(iFileHANDLE, &theOVERLAPPED, &countRead, TRUE))
			return ::sTranslateError(::GetLastError());
		}

	if (oCountRead)
		*oCountRead = countRead;

	return ZFile::errorNone;
	}

static ZFile::Error sWriteAt(HANDLE iFileHANDLE, uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (oCountWritten)
		*oCountWritten = 0;

	OVERLAPPED theOVERLAPPED;
	theOVERLAPPED.Internal = 0;
	theOVERLAPPED.InternalHigh = 0;
	theOVERLAPPED.Offset = iOffset;
	theOVERLAPPED.OffsetHigh = iOffset >> 32;
	theOVERLAPPED.hEvent = 0;
	DWORD countWritten = 0;
	if (!::WriteFile(iFileHANDLE, iSource, iCount, &countWritten, &theOVERLAPPED))
		{
		int err = ::GetLastError();
		if (err != ERROR_IO_PENDING)
			return ::sTranslateError(err);

		if (!::GetOverlappedResult(iFileHANDLE, &theOVERLAPPED, &countWritten, TRUE))
			return ::sTranslateError(::GetLastError());
		}

	if (oCountWritten)
		*oCountWritten = countWritten;

	return ZFile::errorNone;
	}

static ZFile::Error sGetPosition(HANDLE iFileHANDLE, uint64& oPosition)
	{
	LONG positionHigh = 0;	
	DWORD positionLow = ::SetFilePointer(iFileHANDLE, 0, &positionHigh, FILE_CURRENT);
	if (positionLow == -1)
		{
		// -1 is a valid return value for the low 32 bits of a large file, as well as being
		// the indication that an error occurred. So we must double check that we really did
		// get an error and only return if so.
		LONG err = ::GetLastError();
		if (err != NO_ERROR)
			return ::sTranslateError(err);
		}

	oPosition = sAsUInt64(positionHigh, positionLow);
	return ZFile::errorNone;
	}

static ZFile::Error sSetPosition(HANDLE iFileHANDLE, uint64 iPosition)
	{
	LONG positionHigh = iPosition >> 32;
	if (-1 == ::SetFilePointer(iFileHANDLE, iPosition, &positionHigh, FILE_BEGIN))
		{
		LONG err = ::GetLastError();
		if (err != NO_ERROR)
			return ::sTranslateError(err);
		}
	return ZFile::errorNone;
	}

static ZFile::Error sGetSize(HANDLE iFileHANDLE, uint64& oSize)
	{
	DWORD sizeHigh;
	DWORD sizeLow = ::GetFileSize(iFileHANDLE, &sizeHigh);
	if (sizeLow == -1)
		{
		LONG err = ::GetLastError();
		if (err != NO_ERROR)
			return ::sTranslateError(err);
		}
	oSize = sAsUInt64(sizeHigh, sizeLow);
	return ZFile::errorNone;
	}

static ZFile::Error sSetSize(HANDLE iFileHANDLE, uint64 iSize)
	{
	// Remember our current position
	LONG originalPositionHigh = 0;
	DWORD originalPositionLow = ::SetFilePointer(iFileHANDLE, 0, &originalPositionHigh, FILE_CURRENT);
	uint64 originalPosition = sAsUInt64(originalPositionHigh, originalPositionLow);

	// Put the position at the desired end of file
	LONG newSizeHigh = iSize >> 32;
	::SetFilePointer(iFileHANDLE, iSize, &newSizeHigh, FILE_BEGIN);
	if (!::SetEndOfFile(iFileHANDLE))
		return ::sTranslateError(::GetLastError());

	// Put our pointer back where we were, allowing for the possibility that we just truncated the file
	if (originalPosition > iSize)
		{
		originalPositionLow = iSize;
		originalPositionHigh = iSize >> 32;
		}

	if (-1 == ::SetFilePointer(iFileHANDLE, originalPositionLow, &originalPositionHigh, FILE_BEGIN))
		{
		LONG err = ::GetLastError();
		if (err != NO_ERROR)
			return ::sTranslateError(err);
		}
	return ZFile::errorNone;
	}

static ZFile::Error sFlush(HANDLE iFileHANDLE)
	{
	if (!::FlushFileBuffers(iFileHANDLE))
		return ::sTranslateError(::GetLastError());
	return ZFile::errorNone;
	}

static ZFile::Error sFlushVolume(HANDLE iFileHANDLE)
	{
	if (!::FlushFileBuffers(iFileHANDLE))
		return ::sTranslateError(::GetLastError());
	return ZFile::errorNone;
	}

static ZRef<ZFileLoc> sGetCWD(ZFile::Error* oErr)
	{
	size_t bufSize = 1024;
	while (bufSize < 16384)
		{
		vector<char> buffer(bufSize);
		DWORD result = ::GetCurrentDirectoryA(bufSize, &buffer[0]);
		if (result <= 0)
			{
			if (oErr)
				*oErr = ::sTranslateError(::GetLastError());					
			break;
			}
		if (result < bufSize)
			return ZFileLoc_Win::sFromFullWinPath(&buffer[0]);
		bufSize *= 2;
		}
	return ZRef<ZFileLoc>();
	}

static ZRef<ZFileLoc> sGetCWDNT(ZFile::Error* oErr)
	{
	size_t bufSize = 1024;
	while (bufSize < 16384)
		{
		vector<UTF16> buffer(bufSize);
		DWORD result = ::GetCurrentDirectoryW(bufSize, &buffer[0]);
		if (result <= 0)
			{
			if (oErr)
				*oErr = ::sTranslateError(::GetLastError());					
			break;
			}
		if (result < bufSize)
			return ZFileLoc_WinNT::sFromFullWinPath(&buffer[0]);
		bufSize *= 2;
		}
	return ZRef<ZFileLoc>();
	}

static ZTime sAsZTime(const FILETIME& iFT)
	{
	// (Taken from ZTime::sNow)
	// 100 nanoseconds is one ten millionth of a second, so divide the
	// count of 100 nanoseconds by ten million to get the count of seconds.
	double result = (*reinterpret_cast<const int64*>(&iFT)) / 1e7;

	// Subtract the number of seconds between 1601 and 1970.
	result -= ZTime::kEpochDelta_1601_To_1970;
    return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * RealRep_Win

namespace ZANONYMOUS {

class RealRep_Win : public ZFileIterRep_Std::RealRep
	{
public:
	RealRep_Win(ZRef<ZFileLoc_Win> iFileLoc);
	virtual ~RealRep_Win();

	bool HasValue(size_t iIndex);
	ZFileSpec GetSpec(size_t iIndex);
	string GetName(size_t iIndex);

private:
	ZMutex fMutex;
	ZRef<ZFileLoc_Win> fFileLoc;
	HANDLE fHANDLE;
	vector<string> fNames;
	};

static bool sIgnore(const char* iName)
	{
	if (iName[0] == '.')
		{
		if (iName[1] == 0)
			{
			// Got a "."
			return true;
			}

		if (iName[1] == '.')
			{
			if (iName[2] == 0)
				{
				// Got a ".."
				return true;
				}
			}
		}
	return false;
	}

RealRep_Win::RealRep_Win(ZRef<ZFileLoc_Win> iFileLoc)
:	fFileLoc(iFileLoc),
	fHANDLE(INVALID_HANDLE_VALUE)
	{
	if (fFileLoc)
		{
		string query = iFileLoc->pGetPath();
		if (query.size())
			{
			if (query[query.size() - 1] != '\\')
				query += "\\*";
			else
				query += "*";
	
			WIN32_FIND_DATAA theWFD;
			fHANDLE = ::FindFirstFileA(query.c_str(), &theWFD);
			if (fHANDLE != INVALID_HANDLE_VALUE)
				{
				if (!sIgnore(theWFD.cFileName))
					fNames.push_back(theWFD.cFileName);
				}
			}
		}
	}

RealRep_Win::~RealRep_Win()
	{
	if (fHANDLE != INVALID_HANDLE_VALUE)
		::FindClose(fHANDLE);
	}

bool RealRep_Win::HasValue(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	while (fHANDLE != INVALID_HANDLE_VALUE && iIndex >= fNames.size())
		{
		WIN32_FIND_DATAA theWFD;
		if (::FindNextFileA(fHANDLE, &theWFD))
			{
			if (!sIgnore(theWFD.cFileName))
				fNames.push_back(theWFD.cFileName);
			}
		else
			{
			::FindClose(fHANDLE);
			fHANDLE = INVALID_HANDLE_VALUE;
			}
		}

	return iIndex < fNames.size();
	}

ZFileSpec RealRep_Win::GetSpec(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_File_Win, iIndex < fNames.size());
	return ZFileSpec(fFileLoc, fNames[iIndex]);
	}

string RealRep_Win::GetName(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_File_Win, iIndex < fNames.size());
	return fNames[iIndex];
	}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZFileLoc_Win

ZRef<ZFileLoc> ZFileLoc_Win::sGet_CWD()
	{ return ::sGetCWD(nil); }

ZRef<ZFileLoc> ZFileLoc_Win::sGet_Root()
	{ return new ZFileLoc_Win; }
	
ZRef<ZFileLoc> ZFileLoc_Win::sGet_App()
	{
	size_t bufSize = 1024;
	while (bufSize < 16384)
		{
		vector<char> buffer(bufSize);
		DWORD result = ::GetModuleFileNameA(::GetModuleHandleA(nil), &buffer[0], bufSize);
		if (result < 0)
			break;
		if (result < bufSize)
			return sFromFullWinPath(&buffer[0]);

		bufSize *= 2;
		}

	return ZRef<ZFileLoc>();
	}

ZRef<ZFileLoc> ZFileLoc_Win::sFromFullWinPath(const char* iPath)
	{
	size_t pathLength = ZUnicode::sCountCU(iPath);
	const char* pathEnd = iPath + pathLength;
	if (pathLength < 3)
		return ZRef<ZFileLoc>();

	if (iPath[0] == '\\' && iPath[1] == '\\')
		{
		// It's a UNC path.
		const char* nextDivider = find(iPath + 2, pathEnd, '\\');
		vector<string> components;
		ZTrail::sParseStringAndAppend("\\", ".", "..", nextDivider + 1, pathEnd - nextDivider, components);
		return new ZFileLoc_Win(string(iPath, nextDivider - iPath), components);
		}
	else if (isalpha(iPath[0]) && iPath[1] == ':')
		{
		// It's a regular drive letter-prefixed path.
		vector<string> components;
		ZTrail::sParseStringAndAppend("\\", ".", "..", iPath + 2, pathLength - 2, components);
		return new ZFileLoc_Win(string(iPath, 2), components);
		}		
	return ZRef<ZFileLoc>();
	}

ZFileLoc_Win::ZFileLoc_Win()
	{}

ZFileLoc_Win::ZFileLoc_Win(const string& iBase)
:	fBase(iBase)
	{}

ZFileLoc_Win::ZFileLoc_Win(const string& iBase, const vector<string>& iComps)
:	fBase(iBase),
	fComps(iComps)
	{
	ZAssertStop(2, !fBase.empty() || fComps.empty());
	}

ZFileLoc_Win::ZFileLoc_Win(const string& iBase, const string* iComps, size_t iCount)
:	fBase(iBase),
	fComps(iComps, iComps + iCount)
	{
	ZAssertStop(2, !fBase.empty() || fComps.empty());
	}

ZFileLoc_Win::~ZFileLoc_Win()
	{}

ZRef<ZFileIterRep> ZFileLoc_Win::CreateIterRep()
	{ return new ZFileIterRep_Std(new RealRep_Win(this), 0); }

string ZFileLoc_Win::GetName(ZFile::Error* oErr) const
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fComps.size())
		return fComps.back();

	if (fBase.size() == 2 && fBase[1] == ':')
		return fBase.substr(0, 1);

	return fBase;
	}

ZTrail ZFileLoc_Win::TrailTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr) const
	{
	if (ZFileLoc_Win* dest = ZRefDynamicCast<ZFileLoc_Win>(iDest))
		{
		if (fBase == dest->fBase)
			{
			// We have the same base.
			return ZTrail::sTrailFromTo(fComps, dest->fComps);
			}
		// We have different bases, which I'm not sure how to handle, so we'll bail.
		}
	if (oErr)
		*oErr = ZFile::errorGeneric;
	return ZTrail(false);
	}

ZRef<ZFileLoc> ZFileLoc_Win::GetParent(ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fComps.size())
		{
		// We have components, so just bounce up.
		return new ZFileLoc_Win(fBase, &fComps[0], fComps.size() - 1);
		}
	else if (fBase.empty())
		{
		// We're the root. Our parent is nil.
		return ZRef<ZFileLoc>();
		}
	else
		{
		// We're a child of the root. Our parent is the root.
		return new ZFileLoc_Win;
		}
	}

ZRef<ZFileLoc> ZFileLoc_Win::GetDescendant(const string* iComps, size_t iCount, ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (!iCount)
		return this;

	if (fComps.size())
		{
		vector<string> newComps = fComps;
		newComps.insert(newComps.end(), iComps, iComps + iCount);
		return new ZFileLoc_Win(fBase, newComps);
		}

	if (fBase.empty())
		{
		// We're the root, so try to use the first component as a drive letter.
		if (iComps[0].size() == 1 && isalpha(iComps[0][0]))
			return new ZFileLoc_Win(iComps[0] + ":", iComps + 1, iCount - 1);

		return ZRef<ZFileLoc>();
		}

	return new ZFileLoc_Win(fBase, iComps, iCount);
	}

bool ZFileLoc_Win::IsRoot()
	{ return fBase.empty(); }

string ZFileLoc_Win::AsString_POSIX(const string* iComps, size_t iCount)
	{
	string result = "/";
	if (fBase.empty())
		{
		if (iCount)
			{
			result += iComps[0];
			for (size_t x = 1; x < iCount; ++x)
				{
				result += '/';
				result += iComps[x];
				}
			}
		}
	else
		{
		if (fBase.size() == 2 && fBase[1] == ':')
			result += fBase[0];
		else
			result += fBase;

		for (size_t x = 0; x < fComps.size(); ++x)
			{
			result += "/";
			result += fComps[x];
			}

		for (size_t x = 0; x < iCount; ++x)
			{
			result += '/';
			result += iComps[x];
			}
		}

	return result;
	}

string ZFileLoc_Win::AsString_Native(const string* iComps, size_t iCount)
	{
	string result;
	if (fBase.empty())
		{
		if (iCount)
			{
			result = iComps[0];
			if (result.size() == 1 && isalpha(result[0]))
				result += ":";

			if (iCount == 1)
				{
				result += "\\";
				}
			else
				{
				for (size_t x = 1; x < iCount; ++x)
					{
					result += '\\';
					result += iComps[x];
					}
				}
			}
		}
	else
		{
		result = this->pGetPath();
		for (size_t x = 0; x < iCount; ++x)
			{
			result += '\\';
			result += iComps[x];
			}
		}
	return result;
	}

ZFile::Kind ZFileLoc_Win::Kind(ZFile::Error* oErr)
	{
	DWORD result = ::GetFileAttributesA(this->pGetPath().c_str());

	if (result == 0) // Should be INVALID_FILE_ATTRIBUTES, but I can't find a definition for it.
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZFile::kindNone;
		}
		
	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return ZFile::kindDir;

	if (!(result & FILE_ATTRIBUTE_DEVICE))
		return ZFile::kindFile;

	// How to deal with other types?
	return ZFile::kindNone;
	}

uint64 ZFileLoc_Win::Size(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAA theWFD;
	HANDLE theHANDLE = ::FindFirstFileA(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsUInt64(theWFD.nFileSizeHigh, theWFD.nFileSizeLow);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return 0;
		}
	}

ZTime ZFileLoc_Win::TimeCreated(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAA theWFD;
	HANDLE theHANDLE = ::FindFirstFileA(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsZTime(theWFD.ftCreationTime);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZTime();
		}
	}

ZTime ZFileLoc_Win::TimeModified(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAA theWFD;
	HANDLE theHANDLE = ::FindFirstFileA(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsZTime(theWFD.ftLastWriteTime);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZTime();
		}
	}

ZRef<ZFileLoc> ZFileLoc_Win::CreateDir(ZFile::Error* oErr)
	{
	if (!::CreateDirectoryA(this->pGetPath().c_str(), nil))
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZRef<ZFileLoc>();
		}

	if (oErr)
		*oErr = ZFile::errorNone;

	return this;
	}

ZRef<ZFileLoc> ZFileLoc_Win::MoveTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr)
	{
	if (ZFileLoc_Win* dest = ZRefDynamicCast<ZFileLoc_Win>(iDest))
		{
		if (::MoveFileA(this->pGetPath().c_str(), dest->pGetPath().c_str()))
			return iDest;
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZRef<ZFileLoc>();
		}

	if (oErr)
		*oErr = ZFile::errorGeneric;
	return ZRef<ZFileLoc>();
	}

bool ZFileLoc_Win::Delete(ZFile::Error* oErr)
	{
	string myPath = this->pGetPath();
	// Assume it's a file first
	if (!::DeleteFileA(myPath.c_str()))
		{
		// Try as if it's a directory
		if (!::RemoveDirectoryA(myPath.c_str()))
			{
			if (oErr)
				*oErr = ::sTranslateError(::GetLastError());
			return false;
			}
		}
	if (oErr)
		*oErr = ZFile::errorNone;
	return true;
	}

ZRef<ZStreamerRPos> ZFileLoc_Win::OpenRPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), true, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRPos>();
	return new ZStreamerRPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerWPos> ZFileLoc_Win::OpenWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), false, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerWPos>();
	return new ZStreamerWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerRWPos> ZFileLoc_Win::OpenRWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), true, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRWPos>();
	return new ZStreamerRWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerWPos> ZFileLoc_Win::CreateWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreate(this->pGetPath(), iOpenExisting, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerWPos>();
	return new ZStreamerWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerRWPos> ZFileLoc_Win::CreateRWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreate(this->pGetPath(), iOpenExisting, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRWPos>();
	return new ZStreamerRWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZFileR> ZFileLoc_Win::OpenFileR(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), true, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileR>();
	return new ZFileR_Win(theFileHANDLE, true);
	}

ZRef<ZFileW> ZFileLoc_Win::OpenFileW(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), false, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileW>();
	return new ZFileW_Win(theFileHANDLE, true);
	}

ZRef<ZFileRW> ZFileLoc_Win::OpenFileRW(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpen(this->pGetPath(), true, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileRW>();
	return new ZFileRW_Win(theFileHANDLE, true);
	}

ZRef<ZFileW> ZFileLoc_Win::CreateFileW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreate(this->pGetPath(), iOpenExisting, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileW>();
	return new ZFileW_Win(theFileHANDLE, true);
	}

ZRef<ZFileRW> ZFileLoc_Win::CreateFileRW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreate(this->pGetPath(), iOpenExisting, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileRW>();
	return new ZFileRW_Win(theFileHANDLE, true);
	}

string ZFileLoc_Win::pGetPath()
	{
	// Check our constraint here. Either fBase is non-empty or fComps is empty.
	ZAssertStop(2, !fBase.empty() || fComps.empty());

	string result = fBase;

	if (fComps.empty())
		{
		result += "\\";
		}
	else
		{	
		for (size_t x = 0; x < fComps.size(); ++x)
			{
			result += "\\";
			result += fComps[x];
			}
		}

	return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * RealRep_WinNT

namespace ZANONYMOUS {

class RealRep_WinNT : public ZFileIterRep_Std::RealRep
	{
public:
	RealRep_WinNT(ZRef<ZFileLoc_WinNT> iFileLoc);
	virtual ~RealRep_WinNT();

	bool HasValue(size_t iIndex);
	ZFileSpec GetSpec(size_t iIndex);
	string GetName(size_t iIndex);

private:
	ZMutex fMutex;
	ZRef<ZFileLoc_WinNT> fFileLoc;
	HANDLE fHANDLE;
	vector<string16> fNames;
	};

static bool sIgnore(const UTF16* iName)
	{
	if (iName[0] == '.')
		{
		if (iName[1] == 0)
			{
			// Got a "."
			return true;
			}

		if (iName[1] == '.')
			{
			if (iName[2] == 0)
				{
				// Got a ".."
				return true;
				}
			}
		}
	return false;
	}

RealRep_WinNT::RealRep_WinNT(ZRef<ZFileLoc_WinNT> iFileLoc)
:	fFileLoc(iFileLoc),
	fHANDLE(INVALID_HANDLE_VALUE)
	{
	if (fFileLoc)
		{
		string16 query = iFileLoc->pGetPath();
		if (query.size())
			{
			if (query[query.size() - 1] != L'\\')
				query += L"\\*";
			else
				query += L"*";
	
			WIN32_FIND_DATAW theWFD;
			fHANDLE = ::FindFirstFileW(query.c_str(), &theWFD);
			if (fHANDLE != INVALID_HANDLE_VALUE)
				{
				if (!sIgnore(theWFD.cFileName))
					fNames.push_back(theWFD.cFileName);
				}
			}
		}
	}

RealRep_WinNT::~RealRep_WinNT()
	{
	if (fHANDLE != INVALID_HANDLE_VALUE)
		::FindClose(fHANDLE);
	}

bool RealRep_WinNT::HasValue(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	while (fHANDLE != INVALID_HANDLE_VALUE && iIndex >= fNames.size())
		{
		WIN32_FIND_DATAW theWFD;
		if (::FindNextFileW(fHANDLE, &theWFD))
			{
			if (!sIgnore(theWFD.cFileName))
				fNames.push_back(theWFD.cFileName);
			}
		else
			{
			::FindClose(fHANDLE);
			fHANDLE = INVALID_HANDLE_VALUE;
			}
		}

	return iIndex < fNames.size();
	}

ZFileSpec RealRep_WinNT::GetSpec(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_File_Win, iIndex < fNames.size());
	return ZFileSpec(fFileLoc, ZUnicode::sAsUTF8(fNames[iIndex]));
	}

string RealRep_WinNT::GetName(size_t iIndex)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_File_Win, iIndex < fNames.size());
	return ZUnicode::sAsUTF8(fNames[iIndex]);
	}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZFileLoc_WinNT

ZRef<ZFileLoc> ZFileLoc_WinNT::sGet_CWD()
	{ return ::sGetCWDNT(nil); }

ZRef<ZFileLoc> ZFileLoc_WinNT::sGet_Root()
	{ return new ZFileLoc_WinNT; }
	
ZRef<ZFileLoc> ZFileLoc_WinNT::sGet_App()
	{
	size_t bufSize = 1024;
	while (bufSize < 16384)
		{
		vector<UTF16> buffer(bufSize);
		DWORD result = ::GetModuleFileNameW(::GetModuleHandleW(nil), &buffer[0], bufSize);
		if (result < 0)
			break;
		if (result < bufSize)
			return sFromFullWinPath(&buffer[0]);

		bufSize *= 2;
		}

	return ZRef<ZFileLoc>();
	}

ZRef<ZFileLoc> ZFileLoc_WinNT::sFromFullWinPath(const UTF16* iPath)
	{
	size_t pathLength = ZUnicode::sCountCU(iPath);
	const UTF16* pathEnd = iPath + pathLength;
	if (pathLength < 3)
		return ZRef<ZFileLoc>();

	if (iPath[0] == '\\' && iPath[1] == '\\')
		{
		// It's a UNC path.
		const UTF16* nextDivider = find(iPath + 2, pathEnd, L'\\');
		vector<string16> components;
		ZTrail::sParseStringAndAppend(L"\\", L".", L"..", nextDivider + 1, pathEnd - nextDivider, components);
		return new ZFileLoc_WinNT(string16(iPath, nextDivider - iPath), components);
		}
	else if (isalpha(iPath[0]) && iPath[1] == ':')
		{
		// It's a regular drive letter-prefixed path.
		vector<string16> components;
		ZTrail::sParseStringAndAppend(L"\\", L".", L"..", iPath + 2, pathLength - 2, components);
		return new ZFileLoc_WinNT(string16(iPath, 2), components);
		}		
	return ZRef<ZFileLoc>();
	}

ZFileLoc_WinNT::ZFileLoc_WinNT()
	{}

ZFileLoc_WinNT::ZFileLoc_WinNT(const string16& iBase)
:	fBase(iBase)
	{}

ZFileLoc_WinNT::ZFileLoc_WinNT(const string16& iBase, const vector<string16>& iComps)
:	fBase(iBase),
	fComps(iComps)
	{}

ZFileLoc_WinNT::ZFileLoc_WinNT(const string16& iBase, const string16* iComps, size_t iCount)
:	fBase(iBase),
	fComps(iComps, iComps + iCount)
	{}

ZFileLoc_WinNT::~ZFileLoc_WinNT()
	{}

ZRef<ZFileIterRep> ZFileLoc_WinNT::CreateIterRep()
	{ return new ZFileIterRep_Std(new RealRep_WinNT(this), 0); }

string ZFileLoc_WinNT::GetName(ZFile::Error* oErr) const
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fComps.size())
		return ZUnicode::sAsUTF8(fComps.back());

	if (fBase.size() == 2 && fBase[1] == ':')
		return ZUnicode::sAsUTF8(fBase.substr(0, 1));

	return ZUnicode::sAsUTF8(fBase);
	}


ZTrail ZFileLoc_WinNT::TrailTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr) const
	{
	if (ZFileLoc_WinNT* dest = ZRefDynamicCast<ZFileLoc_WinNT>(iDest))
		{
		if (fBase == dest->fBase)
			{
			// We have the same base.
			vector<string> myComps;
			for (size_t x = 0; x < fComps.size(); ++x)
				myComps.push_back(ZUnicode::sAsUTF8(fComps[x]));

			vector<string> destComps;
			for (size_t x = 0; x < dest->fComps.size(); ++x)
				destComps.push_back(ZUnicode::sAsUTF8(dest->fComps[x]));

			return ZTrail::sTrailFromTo(myComps, destComps);
			}
		// We have different bases, which I'm not sure how to handle, so we'll bail.
		}
	if (oErr)
		*oErr = ZFile::errorGeneric;
	return ZTrail(false);
	}

ZRef<ZFileLoc> ZFileLoc_WinNT::GetParent(ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (fComps.size())
		{
		// We have components, so just bounce up.
		return new ZFileLoc_WinNT(fBase, &fComps[0], fComps.size() - 1);
		}
	else if (fBase.empty())
		{
		// We're the root. Our parent is nil.
		return ZRef<ZFileLoc>();
		}
	else
		{
		// We're a child of the root. Our parent is the root.
		return new ZFileLoc_WinNT;
		}
	}

ZRef<ZFileLoc> ZFileLoc_WinNT::GetDescendant(const string* iComps, size_t iCount, ZFile::Error* oErr)
	{
	if (oErr)
		*oErr = ZFile::errorNone;

	if (!iCount)
		return this;

	if (fComps.size())
		{
		vector<string16> newComps = fComps;
		for (size_t x = 0; x < iCount; ++x)
			newComps.push_back(ZUnicode::sAsUTF16(iComps[x]));

		return new ZFileLoc_WinNT(fBase, newComps);
		}

	if (fBase.empty())
		{
		// We're the root, so try to use the first component as a drive letter.
		if (iComps[0].size() == 1 && isalpha(iComps[0][0]))
			{
			vector<string16> newComps;
			for (size_t x = 1; x < iCount; ++x)
				newComps.push_back(ZUnicode::sAsUTF16(iComps[x]));
			return new ZFileLoc_WinNT(ZUnicode::sAsUTF16(iComps[0]) + L":", newComps);
			}

		return ZRef<ZFileLoc>();
		}

	vector<string16> newComps;
	for (size_t x = 0; x < iCount; ++x)
		newComps.push_back(ZUnicode::sAsUTF16(iComps[x]));

	return new ZFileLoc_WinNT(fBase, newComps);
	}

bool ZFileLoc_WinNT::IsRoot()
	{ return fBase.empty(); }

string ZFileLoc_WinNT::AsString_POSIX(const string* iComps, size_t iCount)
	{
	string result = "/";
	if (fBase.empty())
		{
		if (iCount)
			{
			result += iComps[0];
			for (size_t x = 1; x < iCount; ++x)
				{
				result += '/';
				result += iComps[x];
				}
			}
		}
	else
		{
		if (fBase.size() == 2 && fBase[1] == ':')
			result += char(fBase[0]);
		else
			result += ZUnicode::sAsUTF8(fBase);

		for (size_t x = 0; x < fComps.size(); ++x)
			{
			result += "/";
			result += ZUnicode::sAsUTF8(fComps[x]);
			}

		for (size_t x = 0; x < iCount; ++x)
			{
			result += '/';
			result += iComps[x];
			}
		}

	return result;
	}

string ZFileLoc_WinNT::AsString_Native(const string* iComps, size_t iCount)
	{
	string result;
	if (fBase.empty())
		{
		if (iCount)
			{
			result = iComps[0];
			if (result.size() == 1 && isalpha(result[0]))
				result += ":";

			if (iCount == 1)
				{
				result += "\\";
				}
			else
				{
				for (size_t x = 1; x < iCount; ++x)
					{
					result += '\\';
					result += iComps[x];
					}
				}
			}
		}
	else
		{
		result = ZUnicode::sAsUTF8(this->pGetPath());
		for (size_t x = 0; x < iCount; ++x)
			{
			result += '\\';
			result += iComps[x];
			}
		}
	return result;
	}

ZFile::Kind ZFileLoc_WinNT::Kind(ZFile::Error* oErr)
	{
	DWORD result = ::GetFileAttributesW(this->pGetPath().c_str());

	if (result == 0 || (result == ((DWORD)-1))) // Should be INVALID_FILE_ATTRIBUTES, but I can't find a definition for it.
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZFile::kindNone;
		}
		
	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return ZFile::kindDir;

	if (!(result & FILE_ATTRIBUTE_DEVICE))
		return ZFile::kindFile;

	// How to deal with other types?
	return ZFile::kindNone;
	}

uint64 ZFileLoc_WinNT::Size(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAW theWFD;
	HANDLE theHANDLE = ::FindFirstFileW(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsUInt64(theWFD.nFileSizeHigh, theWFD.nFileSizeLow);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return 0;
		}
	}

ZTime ZFileLoc_WinNT::TimeCreated(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAW theWFD;
	HANDLE theHANDLE = ::FindFirstFileW(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsZTime(theWFD.ftCreationTime);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZTime();
		}
	}

ZTime ZFileLoc_WinNT::TimeModified(ZFile::Error* oErr)
	{
	WIN32_FIND_DATAW theWFD;
	HANDLE theHANDLE = ::FindFirstFileW(this->pGetPath().c_str(), &theWFD);
	if (theHANDLE != INVALID_HANDLE_VALUE)
		{
		::FindClose(theHANDLE);

		if (oErr)
			*oErr = ZFile::errorNone;
			
		return sAsZTime(theWFD.ftLastWriteTime);
		}
	else
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZTime();
		}
	}

ZRef<ZFileLoc> ZFileLoc_WinNT::CreateDir(ZFile::Error* oErr)
	{
	if (!::CreateDirectoryW(this->pGetPath().c_str(), nil))
		{
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZRef<ZFileLoc>();
		}

	if (oErr)
		*oErr = ZFile::errorNone;

	return this;
	}

ZRef<ZFileLoc> ZFileLoc_WinNT::MoveTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr)
	{
	if (ZFileLoc_WinNT* dest = ZRefDynamicCast<ZFileLoc_WinNT>(iDest))
		{
		if (::MoveFileW(this->pGetPath().c_str(), dest->pGetPath().c_str()))
			return iDest;
		if (oErr)
			*oErr = ::sTranslateError(::GetLastError());
		return ZRef<ZFileLoc>();
		}

	if (oErr)
		*oErr = ZFile::errorGeneric;
	return ZRef<ZFileLoc>();
	}

bool ZFileLoc_WinNT::Delete(ZFile::Error* oErr)
	{
	string16 myPath = this->pGetPath();
	// Assume it's a file first
	if (!::DeleteFileW(myPath.c_str()))
		{
		// Try as if it's a directory
		if (!::RemoveDirectoryW(myPath.c_str()))
			{
			if (oErr)
				*oErr = ::sTranslateError(::GetLastError());
			return false;
			}
		}
	if (oErr)
		*oErr = ZFile::errorNone;
	return true;
	}

ZRef<ZStreamerRPos> ZFileLoc_WinNT::OpenRPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), true, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRPos>();
	return new ZStreamerRPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerWPos> ZFileLoc_WinNT::OpenWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), false, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerWPos>();
	return new ZStreamerWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerRWPos> ZFileLoc_WinNT::OpenRWPos(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), true, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRWPos>();
	return new ZStreamerRWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerWPos> ZFileLoc_WinNT::CreateWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreateNT(this->pGetPath(), iOpenExisting, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerWPos>();
	return new ZStreamerWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZStreamerRWPos> ZFileLoc_WinNT::CreateRWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreateNT(this->pGetPath(), iOpenExisting, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZStreamerRWPos>();
	return new ZStreamerRWPos_File_Win(theFileHANDLE, true);
	}

ZRef<ZFileR> ZFileLoc_WinNT::OpenFileR(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), true, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileR>();
	return new ZFileR_WinNT(theFileHANDLE, true);
	}

ZRef<ZFileW> ZFileLoc_WinNT::OpenFileW(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), false, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileW>();
	return new ZFileW_WinNT(theFileHANDLE, true);
	}

ZRef<ZFileRW> ZFileLoc_WinNT::OpenFileRW(bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sOpenNT(this->pGetPath(), true, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileRW>();
	return new ZFileRW_WinNT(theFileHANDLE, true);
	}

ZRef<ZFileW> ZFileLoc_WinNT::CreateFileW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreateNT(this->pGetPath(), iOpenExisting, false, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileW>();
	return new ZFileW_WinNT(theFileHANDLE, true);
	}

ZRef<ZFileRW> ZFileLoc_WinNT::CreateFileRW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr)
	{
	HANDLE theFileHANDLE = ::sCreateNT(this->pGetPath(), iOpenExisting, true, iPreventWriters, oErr);
	if (theFileHANDLE == INVALID_HANDLE_VALUE)
		return ZRef<ZFileRW>();
	return new ZFileRW_WinNT(theFileHANDLE, true);
	}

string16 ZFileLoc_WinNT::pGetPath()
	{
	// Check our constraint here. Either fBase is non-empty or fComps is empty.
	ZAssertStop(2, !fBase.empty() || fComps.empty());

	string16 result = fBase;

	if (fComps.empty())
		{
		result += L"\\";
		}
	else
		{
		for (size_t x = 0; x < fComps.size(); ++x)
			{
			result += L"\\";
			result += fComps[x];
			}
		}

	return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileR_Win

ZFileR_Win::ZFileR_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fPosition(-1),
	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileR_Win::~ZFileR_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileR_Win::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{
	ZMutexLocker locker(fMutex);
	if (fPosition != iOffset)
		{
		ZFile::Error err = ::sSetPosition(fFileHANDLE, iOffset);
		if (err != ZFile::errorNone)
			{
			fPosition = -1;
			return err;
			}
		fPosition = iOffset;
		}

	size_t countRead;
	ZFile::Error err = ::sRead(fFileHANDLE, iDest, iCount, &countRead);
	if (err != ZFile::errorNone)
		{
		if (oCountRead)
			*oCountRead = 0;
		fPosition = -1;
		return err;
		}

	fPosition += countRead;
	if (oCountRead)
		*oCountRead = countRead;
	return ZFile::errorNone;
	}

ZFile::Error ZFileR_Win::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

// =================================================================================================
#pragma mark -
#pragma mark * ZFileR_WinNT

ZFileR_WinNT::ZFileR_WinNT(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileR_WinNT::~ZFileR_WinNT()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileR_WinNT::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{ return ::sReadAt(fFileHANDLE, iOffset, iDest, iCount, oCountRead); }

ZFile::Error ZFileR_WinNT::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

// =================================================================================================
#pragma mark -
#pragma mark * ZFileW_Win

ZFileW_Win::ZFileW_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fPosition(-1),
	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileW_Win::~ZFileW_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileW_Win::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	ZMutexLocker locker(fMutex);
	if (fPosition != iOffset)
		{
		ZFile::Error err = ::sSetPosition(fFileHANDLE, iOffset);
		if (err != ZFile::errorNone)
			{
			fPosition = -1;
			return err;
			}
		fPosition = iOffset;
		}

	size_t countWritten;
	ZFile::Error err = ::sWrite(fFileHANDLE, iSource, iCount, &countWritten);
	if (err != ZFile::errorNone)
		{
		if (oCountWritten)
			*oCountWritten = 0;
		fPosition = -1;
		return err;
		}

	fPosition += countWritten;
	if (oCountWritten)
		*oCountWritten = countWritten;
	return ZFile::errorNone;
	}

ZFile::Error ZFileW_Win::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

ZFile::Error ZFileW_Win::SetSize(uint64 iSize)
	{
	ZMutexLocker locker(fMutex);
	fPosition = -1;
	return ::sSetSize(fFileHANDLE, iSize);
	}

ZFile::Error ZFileW_Win::Flush()
	{
	ZMutexLocker locker(fMutex);
	return ::sFlushVolume(fFileHANDLE);
	}

ZFile::Error ZFileW_Win::FlushVolume()
	{
	ZMutexLocker locker(fMutex);
	return ::sFlushVolume(fFileHANDLE);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileW_WinNT

ZFileW_WinNT::ZFileW_WinNT(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileW_WinNT::~ZFileW_WinNT()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileW_WinNT::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{ return ::sWriteAt(fFileHANDLE, iOffset, iSource, iCount, oCountWritten); }

ZFile::Error ZFileW_WinNT::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

ZFile::Error ZFileW_WinNT::SetSize(uint64 iSize)
	{ return ::sSetSize(fFileHANDLE, iSize); }

ZFile::Error ZFileW_WinNT::Flush()
	{ return ::sFlush(fFileHANDLE); }

ZFile::Error ZFileW_WinNT::FlushVolume()
	{ return ::sFlushVolume(fFileHANDLE); }

// =================================================================================================
#pragma mark -
#pragma mark * ZFileRW_Win

ZFileRW_Win::ZFileRW_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileRW_Win::~ZFileRW_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileRW_Win::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{
	ZMutexLocker locker(fMutex);
	if (fPosition != iOffset)
		{
		ZFile::Error err = ::sSetPosition(fFileHANDLE, iOffset);
		if (err != ZFile::errorNone)
			{
			fPosition = -1;
			return err;
			}
		fPosition = iOffset;
		}

	size_t countRead;
	ZFile::Error err = ::sRead(fFileHANDLE, iDest, iCount, &countRead);
	if (err != ZFile::errorNone)
		{
		if (oCountRead)
			*oCountRead = 0;
		fPosition = -1;
		return err;
		}

	fPosition += countRead;
	if (oCountRead)
		*oCountRead = countRead;
	return ZFile::errorNone;
	}

ZFile::Error ZFileRW_Win::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	ZMutexLocker locker(fMutex);
	if (fPosition != iOffset)
		{
		ZFile::Error err = ::sSetPosition(fFileHANDLE, iOffset);
		if (err != ZFile::errorNone)
			{
			fPosition = -1;
			return err;
			}
		fPosition = iOffset;
		}

	size_t countWritten;
	ZFile::Error err = ::sWrite(fFileHANDLE, iSource, iCount, &countWritten);
	if (err != ZFile::errorNone)
		{
		if (oCountWritten)
			*oCountWritten = 0;
		fPosition = -1;
		return err;
		}

	fPosition += countWritten;
	if (oCountWritten)
		*oCountWritten = countWritten;
	return ZFile::errorNone;
	}

ZFile::Error ZFileRW_Win::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

ZFile::Error ZFileRW_Win::SetSize(uint64 iSize)
	{
	ZMutexLocker locker(fMutex);
	fPosition = -1;
	return ::sSetSize(fFileHANDLE, iSize);
	}

ZFile::Error ZFileRW_Win::Flush()
	{
	ZMutexLocker locker(fMutex);
	return ::sFlush(fFileHANDLE);
	}

ZFile::Error ZFileRW_Win::FlushVolume()
	{
	ZMutexLocker locker(fMutex);
	return ::sFlushVolume(fFileHANDLE);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZFileRW_WinNT

ZFileRW_WinNT::ZFileRW_WinNT(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZFileRW_WinNT::~ZFileRW_WinNT()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

ZFile::Error ZFileRW_WinNT::ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead)
	{ return ::sReadAt(fFileHANDLE, iOffset, iDest, iCount, oCountRead); }

ZFile::Error ZFileRW_WinNT::WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten)
	{ return ::sWriteAt(fFileHANDLE, iOffset, iSource, iCount, oCountWritten); }

ZFile::Error ZFileRW_WinNT::GetSize(uint64& oSize)
	{ return ::sGetSize(fFileHANDLE, oSize); }

ZFile::Error ZFileRW_WinNT::SetSize(uint64 iSize)
	{ return ::sSetSize(fFileHANDLE, iSize); }

ZFile::Error ZFileRW_WinNT::Flush()
	{ return ::sFlush(fFileHANDLE); }

ZFile::Error ZFileRW_WinNT::FlushVolume()
	{ return ::sFlushVolume(fFileHANDLE); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_File_Win

ZStreamRPos_File_Win::ZStreamRPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZStreamRPos_File_Win::~ZStreamRPos_File_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

void ZStreamRPos_File_Win::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{ ::sRead(fFileHANDLE, iDest, iCount, oCountRead); }

uint64 ZStreamRPos_File_Win::Imp_GetPosition()
	{
	uint64 pos;
	if (ZFile::errorNone == ::sGetPosition(fFileHANDLE, pos))
		return pos;
	return 0;
	}

void ZStreamRPos_File_Win::Imp_SetPosition(uint64 iPosition)
	{ ::sSetPosition(fFileHANDLE, iPosition); }

uint64 ZStreamRPos_File_Win::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize(fFileHANDLE, theSize))
		return theSize;
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_File_Win

ZStreamerRPos_File_Win::ZStreamerRPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fStream(iFileHANDLE, iCloseWhenFinalized)
	{}

ZStreamerRPos_File_Win::~ZStreamerRPos_File_Win()
	{}

const ZStreamRPos& ZStreamerRPos_File_Win::GetStreamRPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_File_Win

ZStreamWPos_File_Win::ZStreamWPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZStreamWPos_File_Win::~ZStreamWPos_File_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

void ZStreamWPos_File_Win::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{ ::sWrite(fFileHANDLE, iSource, iCount, oCountWritten); }

void ZStreamWPos_File_Win::Imp_Flush()
	{ ::sFlush(fFileHANDLE); }

uint64 ZStreamWPos_File_Win::Imp_GetPosition()
	{
	uint64 pos;
	if (ZFile::errorNone == ::sGetPosition(fFileHANDLE, pos))
		return pos;
	return 0;
	}

void ZStreamWPos_File_Win::Imp_SetPosition(uint64 iPosition)
	{ ::sSetPosition(fFileHANDLE, iPosition); }

uint64 ZStreamWPos_File_Win::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize(fFileHANDLE, theSize))
		return theSize;
	return 0;
	}

void ZStreamWPos_File_Win::Imp_SetSize(uint64 iSize)
	{ ::sSetSize(fFileHANDLE, iSize); }

void ZStreamWPos_File_Win::Imp_Truncate()
	{
	if (!::SetEndOfFile(fFileHANDLE))
		sThrowBadSize();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos_File_Win

ZStreamerWPos_File_Win::ZStreamerWPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fStream(iFileHANDLE, iCloseWhenFinalized)
	{}

ZStreamerWPos_File_Win::~ZStreamerWPos_File_Win()
	{}

const ZStreamWPos& ZStreamerWPos_File_Win::GetStreamWPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_File_Win

ZStreamRWPos_File_Win::ZStreamRWPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fFileHANDLE(iFileHANDLE),
	fCloseWhenFinalized(iCloseWhenFinalized)
	{}

ZStreamRWPos_File_Win::~ZStreamRWPos_File_Win()
	{
	if (fCloseWhenFinalized)
		::sCloseFileHANDLE(fFileHANDLE);
	}

void ZStreamRWPos_File_Win::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{ ::sRead(fFileHANDLE, iDest, iCount, oCountRead); }

void ZStreamRWPos_File_Win::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{ ::sWrite(fFileHANDLE, iSource, iCount, oCountWritten); }

void ZStreamRWPos_File_Win::Imp_Flush()
	{ ::sFlush(fFileHANDLE); }

uint64 ZStreamRWPos_File_Win::Imp_GetPosition()
	{
	uint64 pos;
	if (ZFile::errorNone == ::sGetPosition(fFileHANDLE, pos))
		return pos;
	return 0;
	}

void ZStreamRWPos_File_Win::Imp_SetPosition(uint64 iPosition)
	{ ::sSetPosition(fFileHANDLE, iPosition); }

uint64 ZStreamRWPos_File_Win::Imp_GetSize()
	{
	uint64 theSize;
	if (ZFile::errorNone == ::sGetSize(fFileHANDLE, theSize))
		return theSize;
	return 0;
	}

void ZStreamRWPos_File_Win::Imp_SetSize(uint64 iSize)
	{ ::sSetSize(fFileHANDLE, iSize); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos_File_Win

ZStreamerRWPos_File_Win::ZStreamerRWPos_File_Win(HANDLE iFileHANDLE, bool iCloseWhenFinalized)
:	fStream(iFileHANDLE, iCloseWhenFinalized)
	{}

ZStreamerRWPos_File_Win::~ZStreamerRWPos_File_Win()
	{}

const ZStreamRWPos& ZStreamerRWPos_File_Win::GetStreamRWPos()
	{ return fStream; }

#endif // ZCONFIG_API_Enabled(File_Win)
