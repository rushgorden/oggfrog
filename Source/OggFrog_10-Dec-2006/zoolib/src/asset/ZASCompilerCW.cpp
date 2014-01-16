static const char rcsid[] = "@(#) $Id: ZASCompilerCW.cpp,v 1.22 2006/04/10 20:44:19 agreen Exp $";

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

#define CWPLUGIN_LONG_FILENAME_SUPPORT 0

#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"

#include "ZASCompiler.h"
#include "ZFile.h"
#include "ZFile_MacClassic.h"
#include "ZFile_Win.h"
#include "ZMemory.h"
#include "ZStream_Buffered.h"
#include "ZStream_Memory.h"
#include "ZStrim_Stream.h"
#include "ZString.h"
#include "ZTicks.h"

// =================================================================================================
#pragma mark -
#pragma mark * Forward declarations

static CWResult	sCompileMac(CWPluginContext inContext);
static CWResult	sCompileWin(CWPluginContext inContext);
static CWResult	sPreprocess(CWPluginContext inContext);

#define CallReturnIfError(a) { if (CWResult err = a) return err; }

// =================================================================================================
#pragma mark -
#pragma mark * Plug in entry points

#if CW_USE_PRAGMA_EXPORT
#	pragma export on
#endif

CWPLUGIN_ENTRY(CWPlugin_GetDropInFlags)(const DropInFlags** outFlags, long* outFlagsSize)
	{
	static const DropInFlags sFlags =
		{
		kCurrentDropInFlagsVersion,
		CWDROPINCOMPILERTYPE,
		DROPINCOMPILERLINKERAPIVERSION,
//		DROPINCOMPILERLINKERAPIVERSION_7,
		kCanprecompile | kCandisassemble | kCanpreprocess | kCompAlwaysReload | kCompMultiTargAware,
		CWFOURCHAR('z','a','s',' '),
		DROPINCOMPILERLINKERAPIVERSION
		};

	*outFlags = &sFlags;
	*outFlagsSize = sizeof(sFlags);

	return cwNoErr;
	}

CWPLUGIN_ENTRY(CWPlugin_GetPluginInfo)(const CWPluginInfo** oPluginInfo)
	{
	static CWPluginInfo sCWPluginInfo;
	static bool sInited = false;
	if (!sInited)
		{
		sCWPluginInfo.version = kCurrentCWPluginInfoVersion;
		sCWPluginInfo.companyName = "www.zoolib.org";
		sCWPluginInfo.pluginName = "ZooLib Asset Compiler";
		sCWPluginInfo.pluginDisplayName = "ZooLib Asset Compiler";
		sCWPluginInfo.familyName = "ZooLib";
		sCWPluginInfo.majorIDEVersion = 4;
		sCWPluginInfo.minorIDEVersion = 0;
		sInited = true;
		}
	*oPluginInfo = &sCWPluginInfo;
	return cwNoErr;
	}

CWPLUGIN_ENTRY(CWPlugin_GetTargetList)(const CWTargetList** outTargetList)
	{
	static CWDataType sCPUs[] = { targetCPUAny };

	static CWDataType sOSes[] = { CWFOURCHAR('R','h','a','p'), CWFOURCHAR('m','a','c','h'), targetOSMacintosh, targetOSWindows };
	static CWTargetList sTargetList = { kCurrentCWTargetListVersion, countof(sCPUs), sCPUs, countof(sOSes), sOSes };

	*outTargetList = &sTargetList;

	return cwNoErr;
	}

CWPLUGIN_ENTRY(CWPlugin_GetDefaultMappingList)(const CWExtMapList** outDefaultMappingList)
	{
	static CWExtensionMapping sExtension = {'TEXT', ".zas", kPrecompile};
	static CWExtMapList sExtensionMapList = { kCurrentCWExtMapListVersion, 1, &sExtension };

	*outDefaultMappingList = &sExtensionMapList;

	return cwNoErr;
	}

#if CW_USE_PRAGMA_EXPORT
#	pragma export off
#endif

CWPLUGIN_ENTRY(main)(CWPluginContext inContext)
	{
	long request;
	CallReturnIfError(::CWGetPluginRequest(inContext, &request));

	// dispatch on compiler request
	switch (request)
		{
		case reqInitCompiler:
			{
			// compiler has just been loaded into memory
			return cwNoErr;
			}
		case reqTermCompiler:
			{
			// compiler is about to be unloaded from memory
			return cwNoErr;
			}
		case reqCompDisassemble:
			{
			return sPreprocess(inContext);
			}
		case reqCompile:
			{
			// compile a source file
			Boolean isPreprocessing;
			CallReturnIfError(::CWIsPreprocessing(inContext, &isPreprocessing));
			if (isPreprocessing)
				return sPreprocess(inContext);

			CWTargetInfo theTargetInfo;
			CallReturnIfError(::CWGetTargetInfo(inContext, &theTargetInfo));
			if (theTargetInfo.targetOS == targetOSWindows)
				return sCompileWin(inContext);

			return sCompileMac(inContext);
			}
		}
	return cwErrRequestFailed;
	}

// =================================================================================================
#pragma mark -
#pragma mark * Utility routines

static ZFileSpec sFromCWFileSpec(const CWFileSpec& inSpec)
	{
#if CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS
	#if CWPLUGIN_LONG_FILENAME_SUPPORT
		#error "We don't support long filenames yet"
	#else
		return ZFileSpec(new ZFileLoc_Mac_FSSpec(inSpec));
	#endif
#elif CWPLUGIN_HOST == CWPLUGIN_HOST_WIN32

	return ZFileLoc_Win::sFromFullWinPath(inSpec.path);

#else

	#error Unsupported platform

#endif
	}

static CWFileSpec sFromZFileSpec(const ZFileSpec& inSpec)
	{
#if CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS

	#if CWPLUGIN_LONG_FILENAME_SUPPORT
		#error "We don't support long filenames yet"
	#else
		ZFileLoc_Mac_FSSpec* theFileLoc = ZRefDynamicCast<ZFileLoc_Mac_FSSpec>(inSpec.GetFileLoc());
		ZAssertStop(0, theFileLoc);

		FSSpec theSpec;
		theFileLoc->GetFSSpec(theSpec);
		return theSpec;
	#endif

#elif CWPLUGIN_HOST == CWPLUGIN_HOST_WIN32

	CWFileSpec result;
	strncpy(result.path, inSpec.AsString_Native().c_str(), MAX_PATH);
	return result;

#else

	#error Unsupported platform

#endif
	}

static void sReportMessage(CWPluginContext inContext, short inLevel, const string& inMessage)
	{
	::CWReportMessage(inContext, 0, inMessage.c_str(), 0, inLevel, 0);
	}

// =================================================================================================
#pragma mark -
#pragma mark * StreamerRPos_CW

class StreamerRPos_CW : public ZStreamerRPos
	{
public:
	StreamerRPos_CW(CWPluginContext inContext, const char* inText, size_t inLength);
	virtual ~StreamerRPos_CW();

// From ZStreamerRPos
	virtual ZStreamRPos& GetStreamRPos();

protected:
	CWPluginContext fContext;
	const char* fText;
	ZStreamRPos_Memory fStream;
	};

StreamerRPos_CW::StreamerRPos_CW(CWPluginContext inContext, const char* inText, size_t inLength)
:	fContext(inContext),
	fText(inText),
	fStream(inText, inLength)
	{}

StreamerRPos_CW::~StreamerRPos_CW()
	{ ::CWReleaseFileText(fContext, fText); }

ZStreamRPos& StreamerRPos_CW::GetStreamRPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * StreamProvider_CW

class StreamProvider_CW : public ZASParser::StreamProvider
	{
public:
	StreamProvider_CW(CWPluginContext inContext);

// From ZASParser::StreamProvider
	virtual ZRef<ZStreamerR> ProvideStreamSource(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const;
	virtual ZRef<ZStreamerR> ProvideStreamBinary(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const;

protected:
	CWPluginContext fContext;
	};

StreamProvider_CW::StreamProvider_CW(CWPluginContext inContext)
:	fContext(inContext)
	{}

ZRef<ZStreamerR> StreamProvider_CW::ProvideStreamSource(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const
	{
	CWFileInfo fileinfo;
	// Zero out the CWFileInfo struct
	ZBlockSet(&fileinfo, sizeof(fileinfo), 0);
	fileinfo.fullsearch = iSearchUserDirectories;
	fileinfo.dependencyType = cwNormalDependency;
	fileinfo.isdependentoffile = kCurrentCompiledFile;

#if 0
	// This variant has CW find and load the source data. So if the file is in an
	// open window we'll see any edits that have been made to it. As CW prior
	// to version 9 strips zero bytes this variant can't work with UTF-16.

	fileinfo.suppressload = true;

	if (cwNoErr == ::CWFindAndLoadFile(fContext, iPathRequested.c_str(), &fileinfo))
		{
		const char* theText;
		long theTextLength;
		short theFileDataType;
		if (cwNoErr == ::CWGetFileText(fContext, &fileinfo.filespec, &theText, &theTextLength, &theFileDataType))
			{
			oPathUsed = iPathRequested;
			return new StreamerRPos_CW(fContext, theText, theTextLength);
			}
		}
	return ZRef<ZStreamerR>();

#elif 1
	// This variant opens the file itself. It won't see any pending edits in
	// an open window, but does mean we can see data that CW would otherwise munge.

	fileinfo.suppressload = true;

	CWResult callbackResult = ::CWFindAndLoadFile(fContext, iPath.c_str(), &fileinfo);
	if (callbackResult == cwNoErr)
		{
		oFileSpec = sFromCWFileSpec(fileinfo.filespec);
		if (ZRef<ZStreamerR> theStreamer = oFileSpec.OpenRPos())
			return new ZStreamerR_Buffered(8192, theStreamer);
		}
	return ZRef<ZStreamerR>();

#else
	// And this variant is like the first, but further ensures that CW thinks the file
	// is a text file, which breaks with some text encodings.

	CWResult callbackResult = ::CWFindAndLoadFile(fContext, iPathRequested.c_str(), &fileinfo);
	if (callbackResult == cwNoErr)
		{
		if (fileinfo.filedatatype == cwFileTypeText)
			{
			oPathUsed = iPathRequested;
			return new StreamerRPos_CW(fContext, fileinfo.filedata, fileinfo.filedatalength);
			}
		else
			{
			::CWReleaseFileText(fContext, fileinfo.filedata);
			}
		}
	return ZRef<ZStreamerR>();

#endif
	}

ZRef<ZStreamerR> StreamProvider_CW::ProvideStreamBinary(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const
	{
	CWFileInfo fileinfo;
	// Zero out the CWFileInfo struct
	ZBlockSet(&fileinfo, sizeof(fileinfo), 0);
	fileinfo.fullsearch = iSearchUserDirectories;
	fileinfo.dependencyType = cwNormalDependency;
	fileinfo.isdependentoffile = kCurrentCompiledFile;
	fileinfo.suppressload = true;

	CWResult callbackResult = ::CWFindAndLoadFile(fContext, iPath.c_str(), &fileinfo);
	if (callbackResult == cwNoErr)
		{
		if (fileinfo.filedatatype == cwFileTypeUnknown)
			{
			oFileSpec = sFromCWFileSpec(fileinfo.filespec);
			if (ZRef<ZStreamerR> theStreamer = oFileSpec.OpenRPos())
				return new ZStreamerR_Buffered(8192, theStreamer);
			}
		}
	return ZRef<ZStreamerR>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ErrorHandler_CW

class ErrorHandler_CW : public ZASParser::ErrorHandler
	{
public:
	ErrorHandler_CW(CWPluginContext inContext);

// From ZASParser::ErrorHandler
	virtual void ReportError(const vector<pair<ZFileSpec, int> >& inSources, const string& inMessage) const;
	virtual void ReportError(const string& iMessage) const;
	virtual void ReportProgress(int iLinesProcessed) const;

protected:
	CWPluginContext fContext;
	mutable bigtime_t fNextTime;
	mutable int fLastLineCount;
	};

ErrorHandler_CW::ErrorHandler_CW(CWPluginContext inContext)
:	fContext(inContext)
	{
	fNextTime = ZTicks::sNow();
	fLastLineCount = 0;
	}

void ErrorHandler_CW::ReportError(const vector<pair<ZFileSpec, int> >& inSources, const string& inMessage) const
	{
	string message;
	if (inSources.empty())
		{
		ZDebugPrintf(0, ("error: %s\n", inMessage.c_str()));
		}
	else
		{
		message = ZString::sFormat("%s in \"%s\" at line %d\n", inMessage.c_str(), inSources.front().first.AsString_Native().c_str(), inSources.front().second + 1);
		for (vector<pair<ZFileSpec, int> >::const_iterator iter = inSources.begin() + 1; iter != inSources.end(); ++iter)
			message += ZString::sFormat("\t included from \"%s\" at line %d\n", (*iter).first.AsString_Native().c_str(), (*iter).second + 1);

		CWMessageRef theMessageRef;
		ZBlockZero(&theMessageRef, sizeof(theMessageRef));
		theMessageRef.sourcefile = sFromZFileSpec(inSources.front().first);
		theMessageRef.linenumber = inSources.front().second + 1;
		::CWReportMessage(fContext, &theMessageRef, message.c_str(), 0, messagetypeError, 0);
		}
	}

void ErrorHandler_CW::ReportError(const string& inMessage) const
	{
	::CWReportMessage(fContext, nil, inMessage.c_str(), 0, messagetypeError, 0);
	}

void ErrorHandler_CW::ReportProgress(int iLinesProcessed) const
	{
	if (fLastLineCount != iLinesProcessed && fNextTime <= ZTicks::sNow())
		{
		fLastLineCount = iLinesProcessed;
		fNextTime = ZTicks::sNow() + ZTicks::sPerSecond() / 4;
		::CWDisplayLines(fContext, iLinesProcessed);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * StreamW_Win_MultiResource

class StreamW_Win_MultiResource : public ZStreamW
	{
public:
	StreamW_Win_MultiResource(const string& inBaseName, size_t inChunkSize, size_t inLineLength, const ZStreamW& inStreamSink);
	~StreamW_Win_MultiResource();

// From ZStreamW
	virtual void Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten);
	virtual void Imp_Flush();

protected:
	const ZStreamW& fStreamSink;
	string fBaseName;
	size_t fChunkSize;
	size_t fLineLength;
	size_t fCurrentChunkLength;
	size_t fCurrentLineLength;
	size_t fCurrentChunkIndex;
	};

static void sWriteBegin(const ZStreamW& iStream, const string& inBaseName, size_t iChunkIndex)
	{
	iStream.WriteString(inBaseName);
	ZAssertStop(1, iChunkIndex < 100000);
	iStream.Writef("%04d", iChunkIndex);
	iStream.WriteString(" ZAOC BEGIN\n");
	}

static void sWriteEnd(const ZStreamW& iStream, const string& inBaseName, size_t iChunkIndex)
	{
	ZAssertStop(1, iChunkIndex < 100000);

	iStream.WriteString("\"\nEND // ");
	iStream.WriteString(inBaseName);
	iStream.Writef("%04d", iChunkIndex);
	iStream.WriteString("\n\n");
	}

StreamW_Win_MultiResource::StreamW_Win_MultiResource(const string& inBaseName, size_t inChunkSize, size_t inLineLength, const ZStreamW& inStreamSink)
:	fStreamSink(inStreamSink),
	fBaseName(inBaseName),
	fChunkSize(inChunkSize),
	fLineLength(inLineLength),
	fCurrentChunkLength(0),
	fCurrentLineLength(0),
	fCurrentChunkIndex(0)
	{}

StreamW_Win_MultiResource::~StreamW_Win_MultiResource()
	{
	try
		{
		if (fCurrentChunkLength != 0)
			sWriteEnd(fStreamSink, fBaseName, fCurrentChunkIndex);

		fStreamSink.WriteString(fBaseName);
		fStreamSink.WriteString(" ZAO_ BEGIN\n");
		// Two bytes forming the big endian count of names to follow.
		fStreamSink.Writef("\t\"\\x%02X\\x%02X\",\n",
								((fCurrentChunkIndex + 1) >> 8) & 0xFF,
								(fCurrentChunkIndex + 1) & 0xFF);

		// String indicating the type of the content chunks
		fStreamSink.WriteString("\t\"ZAOC\\0\",\n");

		for (size_t x = 0; x <= fCurrentChunkIndex; ++x)
			{
			if (x != 0)
				fStreamSink.WriteString(",\n");
			fStreamSink.WriteString("\t\"");
			fStreamSink.WriteString(fBaseName);
			fStreamSink.Writef("%04d\\0\"", x);
			}
		fStreamSink.WriteString("\nEND // ");
		fStreamSink.WriteString(fBaseName);
		fStreamSink.WriteString("\n\n");
		}
	catch (...)
		{
		ZDebugStopf(0, ("StreamW_Win_MultiResource::~StreamW_Win_MultiResource, uncaught exception"));
		}
	}

void StreamW_Win_MultiResource::Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten)
	{
	if (outCountWritten)
		*outCountWritten = 0;

	if (fCurrentChunkIndex > 9999)
		return;

	const uint8* localSource = reinterpret_cast<const uint8*>(inSource);
	size_t countRemaining = inCount;
	while (countRemaining)
		{
		if (fCurrentChunkLength == fChunkSize)
			{
			fCurrentChunkLength = 0;
			fCurrentLineLength = 0;
			sWriteEnd(fStreamSink, fBaseName, fCurrentChunkIndex);
			++fCurrentChunkIndex;
			}
		else if (fCurrentLineLength == fLineLength)
			{
			fStreamSink.WriteString("\",\n");
			fCurrentLineLength = 0;
			}

		if (fCurrentChunkLength == 0)
			{
			if (fCurrentChunkIndex > 9999)
				break;
			sWriteBegin(fStreamSink, fBaseName, fCurrentChunkIndex);
			fStreamSink.WriteString("\t\"");
			}
		else if (fCurrentLineLength == 0)
			{
			fStreamSink.WriteString("\t\"");
			}

		static const char sHexDigit[] = "0123456789ABCDEF";
		static char sTempHex[4] = { '\\', 'x', ' ', ' ' };
		sTempHex[2] = sHexDigit[((*localSource) >> 4) & 0x0F];
		sTempHex[3] = sHexDigit[(*localSource) & 0x0F];
		fStreamSink.Write(sTempHex, 4);

		fCurrentChunkLength += 1;
		fCurrentLineLength += 1;
		countRemaining -= 1;
		localSource += 1;
		if (outCountWritten)
			*outCountWritten += 1;
		}
	}

void StreamW_Win_MultiResource::Imp_Flush()
	{
	fStreamSink.Flush();
	}

// =================================================================================================
#pragma mark -
#pragma mark * StreamRWPos_CWHandle

class StreamRWPos_CWHandle : public ZStreamRWPos
	{
public:
	StreamRWPos_CWHandle(CWPluginContext inContext, CWMemHandle inHandle, size_t inGrowIncrement);
	~StreamRWPos_CWHandle();

// From ZStreamR via ZStreamRWPos
	virtual void Imp_Read(void* inDest, size_t inCount, size_t* outCountRead);
	virtual void Imp_Skip(uint64 inCount, uint64* outCountSkipped);

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten);
	virtual void Imp_Flush();

// From ZStreamRPos/ZStreamWPos via ZStreamRWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 inPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 inSize);

private:
	CWPluginContext fContext;
	CWMemHandle fHandle;
	size_t fGrowIncrement;
	uint64 fPosition;
	size_t fSizeLogical;
	};

StreamRWPos_CWHandle::StreamRWPos_CWHandle(CWPluginContext inContext, CWMemHandle inHandle, size_t inGrowIncrement)
:	fContext(inContext),
	fHandle(inHandle)
	{
	fGrowIncrement = inGrowIncrement;
	fPosition = 0;
	long handleSize;
	::CWGetMemHandleSize(fContext, fHandle, &handleSize);
	fSizeLogical = handleSize;
	}

StreamRWPos_CWHandle::~StreamRWPos_CWHandle()
	{
	// Finally, make sure the MemoryBlock is the logical size, not the potentially
	// overallocated size we've been using
	::CWResizeMemHandle(fContext, fHandle, fSizeLogical);
	}

void StreamRWPos_CWHandle::Imp_Read(void* inDest, size_t inCount, size_t* outCountRead)
	{
	if (size_t countToMove = min(uint64(inCount), sDiffPosR(fSizeLogical, fPosition)))
		{
		void* address;
		::CWLockMemHandle(fContext, fHandle, false, &address);
		ZBlockCopy(reinterpret_cast<char*>(address) + fPosition, inDest, countToMove);
		::CWUnlockMemHandle(fContext, fHandle);
		fPosition += countToMove;

		if (outCountRead)
			*outCountRead = countToMove;
		}
	else if (outCountRead)
		{
		*outCountRead = 0;
		}
	}

void StreamRWPos_CWHandle::Imp_Skip(uint64 inCount, uint64* outCountSkipped)
	{
	uint64 realSkip = min(uint64(inCount), sDiffPosR(fSizeLogical, fPosition));
	fPosition += realSkip;
	if (outCountSkipped)
		*outCountSkipped = realSkip;
	}

void StreamRWPos_CWHandle::Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten)
	{
	long currentSize;
	::CWGetMemHandleSize(fContext, fHandle, &currentSize);

	uint64 neededSpace = fPosition + inCount;
	if (currentSize < neededSpace)
		{
		uint64 realSize = max(uint64(currentSize) + fGrowIncrement, neededSpace);
		if (realSize == size_t(realSize))
			{
			if (cwNoErr != ::CWResizeMemHandle(fContext, fHandle, realSize))
				throw bad_alloc();
			currentSize = realSize;
			}
		}

	if (uint64 countToCopy = min(uint64(inCount), sDiffPosR(currentSize, fPosition)))
		{
		void* address;
		::CWLockMemHandle(fContext, fHandle, false, &address);
		ZBlockCopy(inSource, reinterpret_cast<char*>(address) + fPosition, countToCopy);
		::CWUnlockMemHandle(fContext, fHandle);

		fPosition += countToCopy;

		if (fSizeLogical < fPosition)
			fSizeLogical = fPosition;

		if (outCountWritten)
			*outCountWritten = countToCopy;
		}
	else if (outCountWritten)
		{
		*outCountWritten = 0;
		}
	}

void StreamRWPos_CWHandle::Imp_Flush()
	{
	::CWResizeMemHandle(fContext, fHandle, fSizeLogical);
	}

uint64 StreamRWPos_CWHandle::Imp_GetPosition()
	{ return fPosition; }

void StreamRWPos_CWHandle::Imp_SetPosition(uint64 inPosition)
	{ fPosition = inPosition; }

uint64 StreamRWPos_CWHandle::Imp_GetSize()
	{ return fSizeLogical; }

void StreamRWPos_CWHandle::Imp_SetSize(uint64 inSize)
	{
	if (cwNoErr != ::CWResizeMemHandle(fContext, fHandle, inSize))
		throw bad_alloc();
	fSizeLogical = inSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * Main source

static ZFileSpec sGetOutputSpec(const ZFileSpec& inProjectFileSpec, const string& inSourceFileName, const string& inSuffix, string* oBaseName)
	{
	string baseName = inSourceFileName;
	size_t dotPos = inSourceFileName.rfind('.');
	if (dotPos != string::npos)
		baseName = inSourceFileName.substr(0, dotPos);

	if (oBaseName)
		*oBaseName = baseName;

	// And the returned file spec is the child called outBaseName + inSuffix of 
	// the parent of the project file (ie the project directory).
	return inProjectFileSpec.Sibling(baseName + inSuffix);
	}

static CWResult	sCompileMac(CWPluginContext inContext)
	{
	try
		{
		// Get the file number of the file we're supposed to compile.
		long sourceFileNum;
		CallReturnIfError(::CWGetMainFileNumber(inContext, &sourceFileNum));

		// Get the spec for the file we're supposed to compile.
		CWFileSpec theSourceCWFileSpec;
		CallReturnIfError(::CWGetMainFileSpec(inContext, &theSourceCWFileSpec));
		ZFileSpec theSourceFileSpec = sFromCWFileSpec(theSourceCWFileSpec);

		// Get the text of the file.
		const char* text;
		long textLength;
		CallReturnIfError(::CWGetMainFileText(inContext, &text, &textLength));

		// Get the spec for the project file, so we know where to dump the generated file.
		CWFileSpec theProjectCWFileSpec;
		CWResult err = ::CWGetProjectFile(inContext, &theProjectCWFileSpec);
		if (!CWSUCCESS(err))
			{
			::sReportMessage(inContext, messagetypeError, ZString::sFormat("CWGetProjectFile error: %d", err));
			return err;
			}

		// Take the project file spec and create a spec for the output file.
		// from it and the source file's name.
		ZFileSpec outputSpec = sGetOutputSpec(sFromCWFileSpec(theProjectCWFileSpec), theSourceFileSpec.Name(), ".zao", nil);

		// Get a streamer for the output file.
		ZRef<ZStreamerWPos> outputStreamer = outputSpec.CreateWPos(true);
		if (!outputStreamer)
			{
			::sReportMessage(inContext, messagetypeError, "Could not create and open object file \"" + outputSpec.AsString_Native() + "\"");
			return cwErrRequestFailed;
			}

		// Ensure the output file is zero-sized.
		outputStreamer->GetStreamWPos().SetSize(0);

		outputSpec.SetCreatorAndType('ZOOL', 'ZAO_');

		{
		// Buffering the output helps when building on MacOS.
		ZStreamW_Buffered bufferedStream(8192, outputStreamer->GetStreamW());
		ZASCompiler theCompiler(bufferedStream);
		if (!ZASParser::sParse(theSourceFileSpec, ZStreamRPos_Memory(text, textLength),
			theCompiler, StreamProvider_CW(inContext), ErrorHandler_CW(inContext), nil))
			{
			bufferedStream.Abandon();
			outputStreamer.Clear();
			outputSpec.Delete();
			return cwErrRequestFailed;
			}
		}

		CWFileSpec cwOutputSpec = sFromZFileSpec(outputSpec);
		CWObjectData objectData;
		ZBlockSet(&objectData, sizeof(objectData), 0);
		objectData.objectfile = &cwOutputSpec;
		objectData.idatasize = outputStreamer->GetStreamWPos().GetSize();

		outputStreamer.Clear();

#if CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS
		CWFileTime theFileTime;
		::GetDateTime(&theFileTime);
		err = ::CWSetModDate(inContext, &cwOutputSpec, &theFileTime, true);
#endif // CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS

		// Tell the IDE that this file was generated by us.
		err = ::CWStoreObjectData(inContext, sourceFileNum, &objectData);
		if (!CWSUCCESS(err))
			{
			::sReportMessage(inContext, messagetypeError, ZString::sFormat("CWStoreObjectData error: %d", err));
			return err;
			}

		::sReportMessage(inContext, messagetypeInfo, "Generated ZooLib asset object file: \"" + outputSpec.AsString_Native() + "\"");
		}
	catch (bad_alloc&)
		{
		return cwErrOutOfMemory;
		}
	catch (...)
		{
		return cwErrRequestFailed;
		}

	return cwNoErr;
	}

static CWResult	sCompileWin(CWPluginContext inContext)
	{
	try
		{
		long sourceFileNum;
		CallReturnIfError(::CWGetMainFileNumber(inContext, &sourceFileNum));

		CWFileSpec theSourceCWFileSpec;
		CallReturnIfError(::CWGetMainFileSpec(inContext, &theSourceCWFileSpec));
		ZFileSpec theSourceFileSpec = sFromCWFileSpec(theSourceCWFileSpec);

		const char* text;
		long textLength;
		CallReturnIfError(::CWGetMainFileText(inContext, &text, &textLength));

		CWFileSpec theProjectCWFileSpec;
		CWResult err = ::CWGetProjectFile(inContext, &theProjectCWFileSpec);
		if (!CWSUCCESS(err))
			{
			::sReportMessage(inContext, messagetypeError, ZString::sFormat("CWGetProjectFile error: %d", err));
			return err;
			}

		string baseName;
		ZFileSpec outputSpec = sGetOutputSpec(sFromCWFileSpec(theProjectCWFileSpec), theSourceFileSpec.Name(), ".rci", &baseName);

		ZRef<ZStreamerWPos> outputStreamer = outputSpec.CreateWPos(true);
		if (!outputStreamer)
			{
			::sReportMessage(inContext, messagetypeError, "Could not create and open rc include file \"" + outputSpec.AsString_Native() + "\"");
			return cwErrRequestFailed;
			}

		outputStreamer->GetStreamWPos().SetSize(0);

		outputSpec.SetCreatorAndType('CWIE', 'TEXT');

		{
		ZStreamW_Buffered bufferedStream(8192, outputStreamer->GetStreamW());
		StreamW_Win_MultiResource theStreamMultiResource(baseName, 64 * 1024, 16, bufferedStream);
		ZASCompiler theCompiler(theStreamMultiResource);
		if (!ZASParser::sParse(theSourceFileSpec, ZStreamRPos_Memory(text, textLength),
			theCompiler, StreamProvider_CW(inContext), ErrorHandler_CW(inContext), nil))
			{
			bufferedStream.Abandon();
			outputStreamer.Clear();
			outputSpec.Delete();
			return cwErrRequestFailed;
			}
		}

		CWFileSpec cwOutputSpec = sFromZFileSpec(outputSpec);

		CWObjectData objectData;
		ZBlockSet(&objectData, sizeof(objectData), 0);
		objectData.objectfile = &cwOutputSpec;
		objectData.idatasize = outputStreamer->GetStreamWPos().GetSize();

		outputStreamer.Clear();

#if CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS
		CWFileTime theFileTime;
		::GetDateTime(&theFileTime);
		err = ::CWSetModDate(inContext, &cwOutputSpec, &theFileTime, true);
#endif // CWPLUGIN_HOST == CWPLUGIN_HOST_MACOS

		err = ::CWStoreObjectData(inContext, sourceFileNum, &objectData);
		if (!CWSUCCESS(err))
			{
			::sReportMessage(inContext, messagetypeError, ZString::sFormat("CWStoreObjectData error: %d", err));
			return err;
			}

		::sReportMessage(inContext, messagetypeInfo, "Generated ZooLib rc include file: \"" + outputSpec.AsString_Native() + "\"");
		}
	catch (bad_alloc&)
		{
		return cwErrOutOfMemory;
		}
	catch (...)
		{
		return cwErrRequestFailed;
		}

	return cwNoErr;
	}

static CWResult	sPreprocess(CWPluginContext inContext)
	{
	try
		{
		long sourceFileNum;
		CallReturnIfError(::CWGetMainFileNumber(inContext, &sourceFileNum));

		CWFileSpec theSourceCWFileSpec;
		CallReturnIfError(::CWGetMainFileSpec(inContext, &theSourceCWFileSpec));
		ZFileSpec theSourceFileSpec = sFromCWFileSpec(theSourceCWFileSpec);

		const char* text;
		long textLength;
		CallReturnIfError(::CWGetMainFileText(inContext, &text, &textLength));

		CWMemHandle theCWHandle;
		CallReturnIfError(::CWAllocMemHandle(inContext, 0, false, &theCWHandle));

		{
		StreamRWPos_CWHandle theStreamW(inContext, theCWHandle, 1024);
		ZStrimW_StreamUTF8 theStrim(theStreamW);
		ZASParser::ParseHandler_Prettify theParseHandler(theStrim, false);
		if (!ZASParser::sParse(theSourceFileSpec, ZStreamRPos_Memory(text, textLength),
			theParseHandler, StreamProvider_CW(inContext), ErrorHandler_CW(inContext), nil))
			{
			return cwErrRequestFailed;
			}
		}

		CWNewTextDocumentInfo theInfo;
		string dumpName = theSourceFileSpec.Name() + " preprocessed";
		theInfo.documentname = dumpName.c_str();
		theInfo.text = theCWHandle;
		theInfo.markDirty = false;

		CallReturnIfError(::CWCreateNewTextDocument(inContext, &theInfo));
		}
	catch (bad_alloc&)
		{
		return cwErrOutOfMemory;
		}
	catch (...)
		{
		return cwErrRequestFailed;
		}

	return cwNoErr;
	}
