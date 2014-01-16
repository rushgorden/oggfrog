/*  @(#) $Id: ZTextCoder_Win.h,v 1.8 2006/05/12 15:59:35 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZTextCoder_Win__
#define __ZTextCoder_Win__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"
#include "ZCONFIG_SPI.h"

#ifndef ZCONFIG_API_Avail__TextCoder_Win
#	define ZCONFIG_API_Avail__TextCoder_Win ZCONFIG(OS, Win32)
#endif

#ifndef ZCONFIG_API_Desired__TextCoder_Win
#	define ZCONFIG_API_Desired__TextCoder_Win 1
#endif

#include "ZTextCoder.h"

#if ZCONFIG_API_Enabled(TextCoder_Win)

#include "ZWinHeader.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZTextDecoder_Win

class ZTextDecoder_Win : public ZTextDecoder
	{
public:
	ZTextDecoder_Win(const char* iName);
	ZTextDecoder_Win(const string& iName);
	ZTextDecoder_Win(UINT iSourceCodePage);
	virtual ~ZTextDecoder_Win();

	using ZTextDecoder::Decode;

	virtual bool Decode(
		const void* iSource, size_t iSourceBytes, size_t* oSourceBytes, size_t* oSourceBytesSkipped,
		UTF32* iDest, size_t iDestCU, size_t* oDestCU);

private:
	void Init(UINT iSourceCodePage);
	UINT fSourceCodePage;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTextEncoder_Win

class ZTextEncoder_Win : public ZTextEncoder
	{
public:
	ZTextEncoder_Win(const char* iName);
	ZTextEncoder_Win(const string& iName);
	ZTextEncoder_Win(UINT iDestCodePage);
	virtual ~ZTextEncoder_Win();

	using ZTextEncoder::Encode;

	virtual void Encode(const UTF32* iSource, size_t iSourceCU, size_t* oSourceCU,
		void* iDest, size_t iDestBytes, size_t* oDestBytes);
private:
	void Init(UINT iDestCodePage);
	UINT fDestCodePage;
	};

#endif // ZCONFIG_API_Enabled(TextCoder_Win)

#endif // __ZTextCoder_Win__
