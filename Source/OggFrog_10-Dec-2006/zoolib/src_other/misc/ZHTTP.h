/*  @(#) $Id: ZHTTP.h,v 1.16 2006/07/23 21:57:38 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2004 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZHTTP__
#define __ZHTTP__ 1
#include "zconfig.h"

#include "ZStream.h"
#include "ZStreamer.h"
#include "ZTrail.h"
#include "ZTuple.h"

#include <vector>

namespace ZHTTP {

class StreamR_Chunked;
class StreamW_Chunked;

// ==================================================
// Response generation

class Response
	{
public:
	Response();

	void Clear();

	void SetResult(int iResult);
	void SetResult(int iResult, const std::string& iMessage);

	void Set(const std::string& iName, const std::string& iValue);
	void Set(const std::string& iName, int iValue);
	void Set(const std::string& iName, uint64 iValue);

	void Send(const ZStreamW& iStreamW) const;

private:
	std::vector<std::pair<std::string, std::string> > fHeaders;
	int fResult;
	std::string fMessage;
	};

// ==================================================
// Higer level parsing

bool sOrganizeRanges(size_t iSourceSize, const ZTupleValue& iRangeParam, std::vector<std::pair<size_t, size_t> >& oRanges);

bool sReadRequest(const ZStreamR& iStreamR, std::string* oMethod, std::string* oURL, std::string* oErrorString);

bool sReadResponse(const ZStreamU& iStream, int32* oResultCode, std::string* oResultMessage);

bool sReadHeader(const ZStreamR& iStream, ZTuple* oFields);
bool sReadHeaderLine(const ZStreamU& iStream, ZTuple* ioFields);

// Temporary home for this.
void sParseParam(const std::string& iString, ZTuple& oParam);

bool sParseQuery(const std::string& iString, ZTuple& oTuple);
bool sParseQuery(const ZStreamU& iStream, ZTuple& oTuple);

bool sDecodeComponent(const ZStreamU& s, std::string& oComponent);
ZTrail sDecodeTrail(const ZStreamU& s);
ZTrail sDecodeTrail(const std::string& iURL);

std::string sEncodeComponent(const std::string& iString);
std::string sEncodeTrail(const ZTrail& iTrail);

std::string sGetString0(const ZTupleValue& iTV);

ZRef<ZStreamerR> sMakeContentStreamer(const ZTuple& iHeader, ZRef<ZStreamerR> iStreamerR);
ZRef<ZStreamerR> sMakeContentStreamer(const ZTuple& iHeader, const ZStreamR& iStreamR);

// ==================================================
// Request headers
bool sRead_accept(const ZStreamU& iStream, ZTuple* ioFields);
//bool sRead_accept_charset(const ZStreamU& iStream, ZTuple* ioFields); // Not done
//bool sRead_accept_encoding(const ZStreamU& iStream, ZTuple* ioFields); // Not done
bool sRead_accept_language(const ZStreamU& iStream, ZTuple* ioFields);
//bool sRead_authorization(const ZStreamU& iStream, ZTuple* ioFields); // Not done
//bool sRead_from(const ZStreamU& iStream, ZTuple* ioFields); // Not done
//bool sRead_host(const ZStreamU& iStream, ZTuple* ioFields); // Not done

bool sRead_range(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_range(const ZStreamU& iStream, ZTuple& oRange);

//bool sRead_referer(const ZStreamU& iStream, ZTuple* ioFields); // Not done

// ==================================================
// Response headers
bool sRead_www_authenticate(const ZStreamU& iStream, ZTuple* ioFields); // Not done

// ==================================================
// Request or response headers
bool sRead_transfer_encoding(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_transfer_encoding(const ZStreamU& iStream, std::string& oEncoding);

// ==================================================
// Entity headers
bool sRead_content_disposition(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_content_disposition(const ZStreamU& iStream, ZTuple& oTuple);

bool sRead_content_encoding(const ZStreamU& iStream, ZTuple* ioFields); // Not done
bool sRead_content_language(const ZStreamU& iStream, ZTuple* ioFields); // Not done

bool sRead_content_length(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_content_length(const ZStreamU& iStream, int64& oLength);

bool sRead_content_location(const ZStreamU& iStream, ZTuple* ioFields); // Not done
bool sRead_content_md5(const ZStreamU& iStream, ZTuple* ioFields); // Not done

bool sRead_content_range(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_content_range(const ZStreamU& iStream, int64& oBegin, int64& oEnd, int64& oMaxLength);

bool sRead_content_type(const ZStreamU& iStream, ZTuple* ioFields);
bool sRead_content_type(const ZStreamU& iStream, std::string& oType, std::string& oSubType, ZTuple& oParameters);


// ==================================================

bool sReadHTTPVersion(const ZStreamU& iStream, int32* oVersionMajor, int32* oVersionMinor);

bool sReadURI(const ZStreamU& iStream, std::string* oURI);

bool sReadFieldName(const ZStreamU& iStream, std::string* oName, std::string* oNameExact);

bool sReadParameter(const ZStreamU& iStream, std::string* oName, std::string* oValue, std::string* oNameExact);

bool sReadMediaType(const ZStreamU& iStream, std::string* oType, std::string* oSubtype, ZTuple* oParameters, std::string* oTypeExact, std::string* oSubtypeExact);

bool sReadLanguageTag(const ZStreamU& iStream, std::string* oLanguageTag);

// ==================================================
// Lower level parsing

bool sParseURL(const std::string& iURL, std::string* oPath, std::string* oQuery);

bool sReadToken(const ZStreamU& iStream, std::string* oTokenLC, std::string* oTokenExact);

bool sReadQuotedString(const ZStreamU& iStream, std::string* oStringLC, std::string* oStringExact);

bool sReadChar(const ZStreamU& iStream, char iChar);

bool sReadChars(const ZStreamU& iStream, const char* iString);

void sSkipLWS(const ZStreamU& iStream);

bool sReadDecodedChars(const ZStreamU& iStream, std::string& ioString);

// ==================================================
// Lexical classification

bool sIs_CHAR(char iChar);
bool sIs_UPALPHA(char iChar);
bool sIs_LOALPHA(char iChar);
bool sIs_ALPHA(char iChar);
bool sIs_DIGIT(char iChar);
bool sIs_CTL(char iChar);
bool sIs_CR(char iChar);
bool sIs_LF(char iChar);
bool sIs_SP(char iChar);
bool sIs_HT(char iChar);
bool sIs_QUOTE(char iChar);

bool sIs_LWS(char iChar);
bool sIs_TEXT(char iChar);
bool sIs_HEX(char iChar);

bool sIs_tspecials(char iChar);
bool sIs_token(char iChar);

bool sIs_ctext(char iChar);

bool sIs_qdtext(char iChar);

} // namespace ZHTTP

// =================================================================================================
#pragma mark -
#pragma mark * ZHTTP::StreamR_Chunked

class ZHTTP::StreamR_Chunked : public ZStreamR
	{
public:
	StreamR_Chunked(const ZStreamR& iStreamSource);
	~StreamR_Chunked();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();

private:
	const ZStreamR& fStreamSource;
	uint64 fChunkSize;
	bool fHitEnd;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZHTTP::StreamW_Chunked

class ZHTTP::StreamW_Chunked : public ZStreamW
	{
public:
	StreamW_Chunked(size_t iBufferSize, const ZStreamW& iStreamSink);
	StreamW_Chunked(const ZStreamW& iStreamSink);
	~StreamW_Chunked();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

private:
	void Internal_Flush();

	const ZStreamW& fStreamSink;
	uint8* fBuffer;
	size_t fBufferSize;
	size_t fBufferUsed;
	};

#endif // __ZHTTP__
