/*  @(#) $Id: ZTextCoder_ICU.h,v 1.6 2006/07/23 22:01:06 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2005 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZTextCoder_ICU__
#define __ZTextCoder_ICU__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"
#include "ZCONFIG_SPI.h"

#ifndef ZCONFIG_API_Avail__TextCoder_ICU
#	define ZCONFIG_API_Avail__TextCoder_ICU ZCONFIG_SPI_Enabled(ICU)
#endif

#ifndef ZCONFIG_API_Desired__TextCoder_ICU
#	define ZCONFIG_API_Desired__TextCoder_ICU 1
#endif

#include "ZTextCoder.h"

#if ZCONFIG_API_Enabled(TextCoder_ICU)

struct UConverter;

// =================================================================================================
#pragma mark -
#pragma mark * ZTextDecoder_ICU

/// Puts a ZTextDecoder interface over the \a ICU library.

class ZTextDecoder_ICU : public ZTextDecoder
	{
public:
	ZTextDecoder_ICU(const std::string& iSourceName);
	ZTextDecoder_ICU(const char* iSourceName);
	virtual ~ZTextDecoder_ICU();

	using ZTextDecoder::Decode;

	virtual bool Decode(
		const void* iSource, size_t iSourceBytes, size_t* oSourceBytes, size_t* oSourceBytesSkipped,
		UTF32* iDest, size_t iDestCU, size_t* oDestCU);

	virtual void Reset();

private:
	UConverter* fConverter;
	size_t fCountSkipped;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTextEncoder_ICU

/// Puts a ZTextEncoder interface over the \a ICU library.

class ZTextEncoder_ICU : public ZTextEncoder
	{
public:
	ZTextEncoder_ICU(const std::string& iDestName);
	ZTextEncoder_ICU(const char* iDestName);
	virtual ~ZTextEncoder_ICU();

	using ZTextEncoder::Encode;

	virtual void Encode(const UTF32* iSource, size_t iSourceCU, size_t* oSourceCU,
		void* iDest, size_t iDestBytes, size_t* oDestBytes);

	virtual void Reset();

private:
	UConverter* fConverter;
	};

#endif // ZCONFIG_API_Enabled(TextCoder_ICU)

#endif // __ZTextCoder_ICU__
