static const char rcsid[] = "@(#) $Id: ZASCompilerCL.cpp,v 1.18 2006/04/10 20:44:19 agreen Exp $";

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

#include "ZDebug.h"
#include "ZFile.h"
#include "ZASCompiler.h"
#include "ZMain.h"
#include "ZStream_POSIX.h"
#include "ZStrim_Stream.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// =================================================================================================
#pragma mark -
#pragma mark * Utility methods

static void sSkipWrite(const ZStreamW& inStream, size_t inCount)
	{
	for (size_t x = 0; x < inCount; ++x)
		inStream.WriteUInt8(0);
	}

static string sGetBaseFileName(const string& inFileName)
	{
	string baseName = inFileName;
	size_t pos;
	pos = baseName.rfind('.');
	if (pos != string::npos)
		baseName = baseName.substr(0, pos);
	return baseName;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ParseHandler_MakeMake

class IncludeHandler_MakeMake : public ZASParser::IncludeHandler
	{
public:
	IncludeHandler_MakeMake(const ZStreamW& inStream, const string& inSourceFile);
	~IncludeHandler_MakeMake();

	virtual void NotifyInclude(const ZFileSpec& iFileSpec);
protected:
	const ZStreamW& fStreamW;
	};

IncludeHandler_MakeMake::IncludeHandler_MakeMake(const ZStreamW& inStream, const string& inSourceFile)
:	fStreamW(inStream)
	{
	string baseName = sGetBaseFileName(inSourceFile);
	fStreamW.WriteString(baseName);
	fStreamW.WriteString(".zao : ");
	fStreamW.WriteString(inSourceFile);
	}

IncludeHandler_MakeMake::~IncludeHandler_MakeMake()
	{
	fStreamW.WriteString("\n");
	}

void IncludeHandler_MakeMake::NotifyInclude(const ZFileSpec& iFileSpec)
	{
	fStreamW.WriteString(" \\\n");
	fStreamW.WriteString(iFileSpec.AsString());
	}

// =================================================================================================
#pragma mark -
#pragma mark * StreamProvider_Includes

class StreamProvider_Includes : public ZASParser::StreamProvider
	{
public:
	StreamProvider_Includes(const vector<string>& inPaths);

	virtual ZRef<ZStreamerR> ProvideStreamSource(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const;

	virtual ZRef<ZStreamerR> ProvideStreamBinary(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const;

protected:
	ZRef<ZStreamerR> ProvideStream(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const;

	vector<ZFileSpec> fSpecs;
	};

StreamProvider_Includes::StreamProvider_Includes(const vector<string>& inPaths)
	{
	for (vector<string>::const_iterator i = inPaths.begin(); i != inPaths.end(); ++i)
		fSpecs.push_back(ZFileSpec(*i));
	}

ZRef<ZStreamerR> StreamProvider_Includes::ProvideStreamSource(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const
	{
	return this->ProvideStream(iCurrent, iPath, iSearchUserDirectories, oFileSpec);
	}

ZRef<ZStreamerR> StreamProvider_Includes::ProvideStreamBinary(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const
	{
	return this->ProvideStream(iCurrent, iPath, iSearchUserDirectories, oFileSpec);
	}

ZRef<ZStreamerR> StreamProvider_Includes::ProvideStream(const ZFileSpec& iCurrent, const string& iPath,
								bool iSearchUserDirectories, ZFileSpec& oFileSpec) const
	{
	if (!iPath.empty())
		{
		if (iCurrent)
			{
			oFileSpec = iCurrent.Parent().Trail(iPath);
			if (ZRef<ZStreamerR> theStreamer = oFileSpec.OpenRPos())
				return theStreamer;
			}
		else
			{
			oFileSpec = ZFileSpec(iPath);
			if (ZRef<ZStreamerR> theStreamer = oFileSpec.OpenRPos())
				return theStreamer;
			}

		for (vector<ZFileSpec>::const_iterator i = fSpecs.begin(); i != fSpecs.end(); ++i)
			{
			oFileSpec = (*i).Trail(iPath);
			if (ZRef<ZStreamerR> theStreamer = oFileSpec.OpenRPos())
				return theStreamer;
			}

		}
	return ZRef<ZStreamerR>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ErrorHandler_Console

class ErrorHandler_Console : public ZASParser::ErrorHandler
	{
public:
	virtual void ReportError(const vector<pair<ZFileSpec, int> >& inSources, const string& inMessage) const;
	virtual void ReportError(const string& iMessage) const;
	};

void ErrorHandler_Console::ReportError(const vector<pair<ZFileSpec, int> >& inSources, const string& inMessage) const
	{
	if (inSources.empty())
		{
		fprintf(stderr, "zac error: %s\n", inMessage.c_str());
		}
	else
		{
		fprintf(stderr, "zac error: %s in \"%s\" at line %d\n", inMessage.c_str(), inSources.front().first.AsString().c_str(), inSources.front().second + 1);
		for (vector<pair<ZFileSpec, int> >::const_iterator iter = inSources.begin() + 1; iter != inSources.end(); ++iter)
			{
			fprintf(stderr, "\t included from \"%s\" at line %d\n", (*iter).first.AsString().c_str(), (*iter).second + 1);
			}
		}
	}

void ErrorHandler_Console::ReportError(const string& inMessage) const
	{
	fprintf(stderr, "zac error: %s\n", inMessage.c_str());
	}

// =================================================================================================
#pragma mark -
#pragma mark * Main source

static const char sMagicText[] = "ZooLib Appended Data v1.0";
static const size_t sMagicTextSize = sizeof(sMagicText);

static void sRemoveExistingAssets(const ZStreamRWPos& iStream)
	{
	size_t theSize = iStream.GetSize();
	size_t trailerSize = sMagicTextSize + sizeof(int32) + sizeof(int32);
	// Check for there being at least enough room for an empty trailer
	if (theSize >= trailerSize)
		{
		// Look for our trailer
		iStream.SetPosition(theSize - trailerSize);
		size_t offsetOfData = iStream.ReadInt32();
		/*size_t offsetOfTOC =*/ iStream.ReadInt32();
		if (theSize >= trailerSize + offsetOfData)
			{
			char checkedText[sMagicTextSize];
			iStream.Read(checkedText, sMagicTextSize);
			if (0 == memcmp(checkedText, sMagicText, sMagicTextSize))
				{
				iStream.SetSize(theSize - trailerSize - offsetOfData);
				return;
				}
			}
		}
	}

static bool sDoRealAppend(const ZStreamW& iStream, const vector<string>& iSourceFiles)
	{
	ZStreamW_Count theOSC(iStream);
	vector<string> zaoNames;
	vector<size_t> zaoOffsets;
	vector<size_t> zaoSizes;
	for (size_t x = 0; x < iSourceFiles.size(); ++x)
		{
		ZFileSpec theFileSpec = ZFileSpec(iSourceFiles[x]);

		// Open up the source file
		ZRef<ZStreamerRPos> theStreamer = theFileSpec.OpenRPos();
		if (!theStreamer)
			{
			fprintf(stderr, "zac error: Could not open source file \"%s\"\n", theFileSpec.AsString().c_str());
			return false;
			}

		// Get the source file's name without directories or file extension
		string baseName = sGetBaseFileName(theFileSpec.Name());
		// Move on to a 32 bit boundary, if necessary
		::sSkipWrite(theOSC, ((theOSC.GetCount() + 3) & ~3) - theOSC.GetCount());
		// Remember the name and its starting offset
		zaoNames.push_back(baseName);
		zaoOffsets.push_back(theOSC.GetCount());
		zaoSizes.push_back(theStreamer->GetStreamRPos().GetSize());
		theOSC.CopyAllFrom(theStreamer->GetStreamRPos());
		}
	// Write the table of contents, remembering where we started
	size_t startOfTOC = theOSC.GetCount();
	theOSC.WriteInt32(zaoNames.size());
	for (size_t x = 0; x < zaoNames.size(); ++x)
		{
		theOSC.WriteInt32(zaoOffsets[x]);
		theOSC.WriteInt32(zaoSizes[x]);
		theOSC.WriteInt32(zaoNames[x].size());
		theOSC.WriteString(zaoNames[x]);
		}
	size_t startOfTrailer = theOSC.GetCount();
	theOSC.WriteInt32(startOfTrailer - 0);
	theOSC.WriteInt32(startOfTrailer - startOfTOC);
	theOSC.Write(sMagicText, sMagicTextSize);
	return true;
	}

static bool sAppend(const string& iOutputFile, const vector<string>& iSourceFiles)
	{
	if (iOutputFile.empty())
		{
		return sDoRealAppend(ZStreamW_FILE(stdout), iSourceFiles);
		}
	else
		{
		// See if the file already exists
		ZRef<ZStreamerWPos> theStreamerWPos;
		if (ZRef<ZStreamerRWPos> theStreamerRWPos = ZFileSpec(iOutputFile).OpenRWPos())
			{
			theStreamerWPos = theStreamerRWPos;
			sRemoveExistingAssets(theStreamerRWPos->GetStreamRWPos());
			}
		else
			{
			theStreamerWPos = ZFileSpec(iOutputFile).CreateWPos(true);
			if (!theStreamerWPos)
				{
				fprintf(stderr, "zac error: Could not open or create output file \"%s\"\n", iOutputFile.c_str());
				return false;
				}
			}

		const ZStreamWPos& theOutputStream = theStreamerWPos->GetStreamWPos();
		size_t originalSize = theOutputStream.GetSize();
		theOutputStream.SetPosition(originalSize);
		// Force output stream to start with a size that is aligned on a 32 bit boundary
		::sSkipWrite(theOutputStream, ((originalSize + 3) & ~3) - originalSize);
		if (sDoRealAppend(theOutputStream, iSourceFiles))
			return true;
		theOutputStream.SetSize(originalSize);
		return false;
		}
	}

static bool sDoRealCompile(const ZStreamW& iStream, const string& iSourceFile, const vector<string>& iIncludePaths)
	{
	ZASCompiler theParseHandler(iStream);
	return ZASParser::sParse(iSourceFile, theParseHandler, StreamProvider_Includes(iIncludePaths), ErrorHandler_Console(), nil);
	}

static bool sCompile(const string& iOutputFile, const string& iSourceFile, const vector<string>& iIncludePaths)
	{
	if (iOutputFile.empty())
		{
		return sDoRealCompile(ZStreamW_FILE(stdout), iSourceFile, iIncludePaths);
		}
	else
		{
		ZRef<ZStreamerWPos> theStreamer = ZFileSpec(iOutputFile).CreateWPos(true);
		if (!theStreamer)
			{
			fprintf(stderr, "zac error: Could not create output file \"%s\"\n", iOutputFile.c_str());
			throw runtime_error("Couldn't create output file");
			}

		const ZStreamWPos& theOutputStream = theStreamer->GetStreamWPos();
		if (sDoRealCompile(theOutputStream, iSourceFile, iIncludePaths))
			{
			theOutputStream.Truncate();
			return true;
			}
		theOutputStream.SetSize(0);
		theStreamer.Clear();
		ZFileSpec(iOutputFile).Delete();
		return false;
		}
	}

static bool sDoRealPrettify(const ZStrimW& iStrim, const string& iSourceFile, const vector<string>& iIncludePaths, bool inShowBinaries)
	{
	ZASParser::ParseHandler_Prettify theParseHandler(iStrim, inShowBinaries);
	return ZASParser::sParse(iSourceFile, theParseHandler, StreamProvider_Includes(iIncludePaths), ErrorHandler_Console(), nil);
	}

static bool  sPrettify(const string& iOutputFile, const string& iSourceFile, const vector<string>& iIncludePaths, bool inShowBinaries)
	{
	if (iOutputFile.empty())
		{
		return sDoRealPrettify(ZStrimW_StreamUTF8(ZStreamW_FILE(stdout)), iSourceFile, iIncludePaths, inShowBinaries);
		}
	else
		{
		ZRef<ZStreamerWPos> theStreamer = ZFileSpec(iOutputFile).CreateWPos(true);
		if (!theStreamer)
			{
			fprintf(stderr, "zac error: Could not create output file \"%s\"\n", iOutputFile.c_str());
			throw runtime_error("Couldn't create output file");
			}

		const ZStreamWPos& theOutputStream = theStreamer->GetStreamWPos();
		if (sDoRealPrettify(ZStrimW_StreamUTF8(theOutputStream), iSourceFile, iIncludePaths, inShowBinaries))
			{
			theOutputStream.Truncate();
			return true;
			}
		theOutputStream.SetSize(0);
		theStreamer.Clear();
		ZFileSpec(iOutputFile).Delete();
		return false;
		}
	}

static bool sDoRealMakeMake(const ZStreamW& iStream, const string& iSourceFile, const vector<string>& iIncludePaths)
	{
	ZASParser::ParseHandler theParseHandler;
	IncludeHandler_MakeMake theIncludeHandler(iStream, iSourceFile);
	return ZASParser::sParse(iSourceFile, theParseHandler, StreamProvider_Includes(iIncludePaths), ErrorHandler_Console(), &theIncludeHandler);
	}

static bool sMakeMake(const string& iOutputFile, const string& iSourceFile, const vector<string>& iIncludePaths)
	{
	if (iOutputFile.empty())
		{
		return sDoRealMakeMake(ZStreamW_FILE(stdout), iSourceFile, iIncludePaths);
		}
	else
		{
		ZRef<ZStreamerWPos> theStreamer = ZFileSpec(iOutputFile).CreateWPos(true);
		if (!theStreamer)
			{
			fprintf(stderr, "zac error: Could not create output file \"%s\"\n", iOutputFile.c_str());
			throw runtime_error("Couldn't create output file");
			}

		const ZStreamWPos& theOutputStream = theStreamer->GetStreamWPos();
		if (sDoRealMakeMake(theOutputStream, iSourceFile, iIncludePaths))
			{
			theOutputStream.Truncate();
			return true;
			}
		theOutputStream.SetSize(0);
		theStreamer.Clear();
		ZFileSpec(iOutputFile).Delete();
		return false;
		}
	}

static void sShowUsage()
	{
	fprintf(stderr,
		"ZooLib Asset Compiler\n"
		"Usage:\n"
		"zac -c [addl-options] <zas-file>\n"
		"  Compile <zas-file> into a zao byte stream.\n"
		"\n"
		"zac -M [addl-options] <zas-file>\n"
		"  Write gmake-compatible dependency information rather than\n"
		"  the actual byte stream.\n"
		"\n"
		"zac -p (0|1) [addl-options] <zas-file>\n"
		"  Write re-generated and prettified source rather than the\n"
		"  byte stream. When the param is 1 write a hex-dump of\n"
		"  binaries. When it is 0 write only the size of the binaries,\n"
		"  making for shorter and more readable output.\n"
		"\n"
		"For the -c, -M and -p variants the following two options exist:\n"
		"  -o <file>  Write to <file> rather than stdout.\n"
		"  -I <dir>   Add <dir> to the list to search for included files.\n"
		"\n"
		"zac -a [-o <file>] (zao-file)+\n"
		"  Append one ore more zao byte streams to each other. If the -o\n"
		"  option references an existing file any extant zao byte streams\n"
		"  are removed from it before appending the new ones.\n"
		"\n"
		"zac -h|--help\n"
		"  Show this message.\n"
		);
	}

static void sTerminate()
	{
	ZDebugStop(0);
	}

int ZMain(int argc, char** argv)
	{
	set_terminate(sTerminate);

	bool okay = true;
	enum { eNone, eCompile, eMakeMake, ePrettify, eAppend };
	int theAction = eNone;
	string outputFile;
	bool showBinaries = false;
	vector<string> includePaths;
	vector<string> sourceFiles;
	int x = 1;

	for (; x < argc && okay; ++x)
		{
		if (argv[x][0] != '-')
			{
			sourceFiles.push_back(argv[x]);
			}
		else if (argv[x][1] == '\0')
			{
			okay = false;
			}
		else
			{
			switch (argv[x][1])
				{
				case '-':
					{
					if (string("help") == &argv[x][2])
						{
						sShowUsage();
						return 0;
						}
					else
						{
						fprintf(stderr, "zac error: unrecognized long option \"%s\".\n", argv[x]);
						okay = false;
						}
					break;
					}
				case 'h':
					{
					sShowUsage();
					return 0;
					}
				case 'c':
					{
					if (theAction != eNone)
						{
						fprintf(stderr, "zac error: must only specify one of -c|-M|-p|-a, got '-c'.\n", argv[x]);
						okay = false;
						}
					theAction = eCompile;
					break;
					}
				case 'M':
					{
					if (theAction != eNone)
						{
						fprintf(stderr, "zac error: must only specify one of -c|-M|-p|-a, got '-M'.\n", argv[x]);
						okay = false;
						}
					if (argv[x][2] != '\0')
						{
						fprintf(stderr, "zac error: -M option takes no parameters.\n");
						okay = false;
						}
					theAction = eMakeMake;
					break;
					}
				case 'p':
					{
					if (theAction != eNone)
						{
						fprintf(stderr, "zac error: must only specify one of -c|-M|-p|-a, got '-p'.\n", argv[x]);
						okay = false;
						}
					if (argv[x][2] == '\0')
						{
						if (x < argc - 1 && argv[x + 1][0] != '-')
							{
							if (atoi(argv[x + 1]))
								showBinaries = true;
							++x;
							}
						else
							{
							fprintf(stderr, "zac error: Missing parameter for -p option.\n");
							okay = false;
							}
						}
					else
						{
						if (atoi(&argv[x][2]))
							showBinaries = true;
						}
					theAction = ePrettify;
					break;
					}
				case 'a':
					{
					if (theAction != eNone)
						{
						fprintf(stderr, "zac error: must only specify one of -c|-M|-p|-a, got '-a'.\n", argv[x]);
						okay = false;
						}
					theAction = eAppend;
					break;
					}
				case 'I':
					{
					if (argv[x][2] == '\0')
						{
						if (x < argc - 1 && argv[x + 1][0] != '-')
							{
							includePaths.push_back(&argv[x + 1][0]);
							++x;
							}
						else
							{
							fprintf(stderr, "zac error: Missing path for -I option.\n");
							okay = false;
							}
						}
					else
						{
						includePaths.push_back(&argv[x][2]);
						}
					break;
					}
				case 'o':
					{
					if (argv[x][2] == '\0')
						{
						if (x < argc - 1 && argv[x + 1][0] != '-')
							{
							outputFile = argv[x + 1];
							++x;
							}
						else
							{
							fprintf(stderr, "zac error: Missing filename for -o option.\n");
							okay = false;
							}
						}
					else
						{
						if (outputFile.size())
							{
							fprintf(stderr, "zac error: multiple -o options given.\n");
							okay = false;
							}
						else
							{
							outputFile = &argv[x][2];
							}
						}
					break;
					}
				}
			}
		}

	if (okay && theAction == eNone)
		{
		fprintf(stderr, "zac error: no action specified (one of -c|-M|-p|-a)\n");
		okay = false;
		}
	else if (okay && sourceFiles.size() == 0)
		{
		fprintf(stderr, "zac error: no source file specified\n");
		okay = false;
		}
	else if (okay)
		{
		try
			{
			switch (theAction)
				{
				case eCompile:
					{
					if (sourceFiles.size() > 1)
						{
						fprintf(stderr, "zac error: more than one source file specified.\n");
						okay = false;
						}
					else
						{
						if (sCompile(outputFile, sourceFiles[0], includePaths))
							return 0;
						}
					break;
					}
				case eMakeMake:
					{
					if (sourceFiles.size() > 1)
						{
						fprintf(stderr, "zac error: more than one source file specified.\n");
						okay = false;
						}
					else
						{
						if (sMakeMake(outputFile, sourceFiles[0], includePaths))
							return 0;
						}
					break;
					}
				case ePrettify:
					{
					if (sourceFiles.size() > 1)
						{
						fprintf(stderr, "zac error: more than one source file specified.\n");
						okay = false;
						}
					else
						{
						if (sPrettify(outputFile, sourceFiles[0], includePaths, showBinaries))
							return 0;
						}
					break;
					}
				case eAppend:
					{
					if (sAppend(outputFile, sourceFiles))
						return 0;
					break;
					}
				}
			}
		catch (...)
			{
			}
		}
	return 1;
	}
