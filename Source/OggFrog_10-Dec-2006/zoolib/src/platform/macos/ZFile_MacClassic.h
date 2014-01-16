/*  @(#) $Id: ZFile_MacClassic.h,v 1.26 2006/07/15 20:54:22 goingware Exp $ */

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

#ifndef __ZFile_MacOld__
#define __ZFile_MacOld__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"
#include "ZCONFIG_SPI.h"

#ifndef ZCONFIG_API_Avail__File_MacClassic
#	define ZCONFIG_API_Avail__File_MacClassic (ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX))
#endif

#ifndef ZCONFIG_API_Desired__File_MacClassic
#	define ZCONFIG_API_Desired__File_MacClassic 1
#endif

#include "ZFile.h"

#if ZCONFIG_API_Enabled(File_MacClassic)

#ifndef __FILES__
#	if ZCONFIG(OS, MacOSX)
#		include <CarbonCore/Files.h>
#	else
#		include <Files.h>
#	endif
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZFileLoc_Mac_FSSpec

class ZFileLoc_Mac_FSSpec : public ZFileLoc
	{
public:
	static ZRef<ZFileLoc> sGet_CWD();
	static ZRef<ZFileLoc> sGet_Root();
	static ZRef<ZFileLoc> sGet_App();

	ZFileLoc_Mac_FSSpec(short iVRefNum, long iDirID, const unsigned char* iName);
	ZFileLoc_Mac_FSSpec(short iVRefNum, long iDirID);
	ZFileLoc_Mac_FSSpec(const FSSpec& iSpec);
	virtual ~ZFileLoc_Mac_FSSpec();

// From ZFileLoc
	virtual ZRef<ZFileIterRep> CreateIterRep();

	virtual string GetName(ZFile::Error* oErr) const;
	virtual ZTrail TrailTo(ZRef<ZFileLoc> iDest, ZFile::Error* oErr) const;

	virtual ZRef<ZFileLoc> GetParent(ZFile::Error* oErr);
	virtual ZRef<ZFileLoc> GetDescendant(const string* iComps, size_t iCount, ZFile::Error* oErr);

	virtual bool IsRoot();

	virtual string AsString_POSIX(const string* iComps, size_t iCount);
	virtual string AsString_Native(const string* iComps, size_t iCount);

	virtual ZFile::Kind Kind(ZFile::Error* oErr);
	virtual uint64 Size(ZFile::Error* oErr);
	virtual ZTime TimeCreated(ZFile::Error* oErr);
	virtual ZTime TimeModified(ZFile::Error* oErr);

	virtual bool SetCreatorAndType(uint32 iCreator, uint32 iType, ZFile::Error* oErr);

	virtual ZRef<ZFileLoc> CreateDir(ZFile::Error* oErr);

	virtual ZRef<ZFileLoc> MoveTo(ZRef<ZFileLoc> iOther, ZFile::Error* oErr);
	virtual bool Delete(ZFile::Error* oErr);

	virtual ZRef<ZStreamerRPos> OpenRPos(bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZStreamerWPos> OpenWPos(bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZStreamerRWPos> OpenRWPos(bool iPreventWriters, ZFile::Error* oErr);

	virtual ZRef<ZStreamerWPos> CreateWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZStreamerRWPos> CreateRWPos(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr);

	virtual ZRef<ZFileR> OpenFileR(bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZFileW> OpenFileW(bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZFileRW> OpenFileRW(bool iPreventWriters, ZFile::Error* oErr);

	virtual ZRef<ZFileW> CreateFileW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr);
	virtual ZRef<ZFileRW> CreateFileRW(bool iOpenExisting, bool iPreventWriters, ZFile::Error* oErr);

// Our protocol
	void GetFSSpec(FSSpec& oSpec);

private:
	string pAsString(char iSeparator, const string* iComps, size_t iCount);

	short fVRefNum;
	long fDirID;
	StrFileName fName;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZFileR_MacOld

class ZFileR_MacOld : public ZFileR
	{
public:
	ZFileR_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZFileR_MacOld();

// From ZFileR
	virtual ZFile::Error ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead);

	virtual ZFile::Error GetSize(uint64& oSize);
	
private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZFileW_MacOld

class ZFileW_MacOld : public ZFileW
	{
public:
	ZFileW_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZFileW_MacOld();

// From ZFileW
	virtual ZFile::Error WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten);

	virtual ZFile::Error GetSize(uint64& oSize);
	virtual ZFile::Error SetSize(uint64 iSize);

	virtual ZFile::Error Flush();
	virtual ZFile::Error FlushVolume();
	
private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZFileRW_MacOld

class ZFileRW_MacOld : public ZFileRW
	{
public:
	ZFileRW_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZFileRW_MacOld();

// From ZFileRW
	virtual ZFile::Error ReadAt(uint64 iOffset, void* iDest, size_t iCount, size_t* oCountRead);
	virtual ZFile::Error WriteAt(uint64 iOffset, const void* iSource, size_t iCount, size_t* oCountWritten);

	virtual ZFile::Error GetSize(uint64& oSize);
	virtual ZFile::Error SetSize(uint64 iSize);

	virtual ZFile::Error Flush();
	virtual ZFile::Error FlushVolume();
	
private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_File_MacOld

class ZStreamRPos_File_MacOld : public ZStreamRPos
	{
public:
	ZStreamRPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	~ZStreamRPos_File_MacOld();

// From ZStreamR via ZStreamRPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

// From ZStreamRPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	uint64 fPosition;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_File_MacOld

class ZStreamerRPos_File_MacOld : public ZStreamerRPos
	{
public:
	ZStreamerRPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZStreamerRPos_File_MacOld();

// From ZStreamerRPos
	virtual const ZStreamRPos& GetStreamRPos();

private:
	ZStreamRPos_File_MacOld fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_File_MacOld

class ZStreamWPos_File_MacOld : public ZStreamWPos
	{
public:
	ZStreamWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	~ZStreamWPos_File_MacOld();

// From ZStreamW via ZStreamWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

// From ZStreamWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();
	virtual void Imp_SetSize(uint64 inSize);

private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	uint64 fPosition;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos_File_MacOld

class ZStreamerWPos_File_MacOld : public ZStreamerWPos
	{
public:
	ZStreamerWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZStreamerWPos_File_MacOld();

// From ZStreamerWPos
	virtual const ZStreamWPos& GetStreamWPos();

private:
	ZStreamWPos_File_MacOld fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_File_MacOld

class ZStreamRWPos_File_MacOld : public ZStreamRWPos
	{
public:
	ZStreamRWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	~ZStreamRWPos_File_MacOld();

private:
// From ZStreamR via ZStreamRWPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

// From ZStreamRPos/ZStreamWPos via ZStreamRWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 iSize);

private:
	SInt16 fRefNum;
	bool fCloseWhenFinalized;
	uint64 fPosition;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos_File_MacOld

class ZStreamerRWPos_File_MacOld : public ZStreamerRWPos
	{
public:
	ZStreamerRWPos_File_MacOld(SInt16 iRefNum, bool iCloseWhenFinalized);
	virtual ~ZStreamerRWPos_File_MacOld();

// From ZStreamerRWPos
	virtual const ZStreamRWPos& GetStreamRWPos();

private:
	ZStreamRWPos_File_MacOld fStream;
	};

#endif // ZCONFIG_API_Enabled(File_MacClassic)

#endif // __ZFile_MacOld__
