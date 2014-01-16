/*  @(#) $Id: ZUtil_Tuple.h,v 1.12 2006/10/13 20:00:29 agreen Exp $ */

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

#ifndef __ZUtil_Tuple__
#define __ZUtil_Tuple__ 1
#include "zconfig.h"

#include <string>
#include <vector>

class ZStrimR;
class ZStrimU;
class ZStrimW;

class ZTuple;
class ZTupleValue;

namespace ZUtil_Tuple {

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple::Options

struct Options
	{
	Options(bool iDoIndentation = false);

	std::string fEOLString;
	std::string fIndentString;

	int fRawChunkSize;
	std::string fRawByteSeparator;
	bool fRawAsASCII;

	bool fBreakStrings;
	int fStringLineLength;

	bool fIDsHaveDecimalVersionComment;

	bool fTimesHaveUserLegibleComment;

	bool DoIndentation() const { return !fIndentString.empty(); }
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple::Format

struct Format
	{
	Format(const ZTupleValue& iTupleValue, int iInitialIndent, const Options& iOptions);

	const ZTupleValue& fTupleValue;
	int fInitialIndent;
	const Options& fOptions;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple, writing and parsing pieces

void sWrite_PropName(const ZStrimW& iStrimW, const string& iPropName);

bool sRead_Identifier(const ZStrimU& iStrimU, string* oStringLC, string* oStringExact);

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple, TupleValue

void sToStrim(const ZStrimW& iStrimW, const ZTupleValue& iTupleValue);

void sToStrim(const ZStrimW& iStrimW, const ZTupleValue& iTupleValue,
	size_t iInitialIndent, const Options& iOptions);

std::string sAsString(const ZTupleValue& iTupleValue);

bool sFromStrim(const ZStrimU& iStrimU, ZTupleValue& oTupleValue);

bool sFromString(const std::string& iString, ZTupleValue& oTupleValue);

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple, vector<ZTupleValue>

void sToStrim(const ZStrimW& iStrimW, const std::vector<ZTupleValue>& iVector);

void sToStrim(const ZStrimW& iStrimW, const std::vector<ZTupleValue>& iVector,
	size_t iInitialIndent, const Options& iOptions);

std::string sAsString(const std::vector<ZTupleValue>& iVector);

bool sFromStrim(const ZStrimU& iStrimU, std::vector<ZTupleValue>& oVector);

bool sFromString(const std::string& iString, std::vector<ZTupleValue>& oVector);

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple, ZTuple

void sToStrim(const ZStrimW& iStrimW, const ZTuple& iTuple);

void sToStrim(const ZStrimW& iStrimW, const ZTuple& iTuple,
	size_t iInitialIndent, const Options& iOptions);

std::string sAsString(const ZTuple& iTuple);

std::string sAsString(const ZTuple& iTuple,
	size_t iInitialIndent, const Options& iOptions);

bool sFromStrim(const ZStrimU& iStrimU, ZTuple& oTuple);

bool sFromString(const std::string& iString, ZTuple& oTuple);

}  // namespace ZUtil_Tuple

// =================================================================================================
#pragma mark -
#pragma mark * operator<< overloads

const ZStrimW& operator<<(const ZStrimW& s, const ZTupleValue& iTupleValue);

const ZStrimW& operator<<(const ZStrimW& s, const ZUtil_Tuple::Format& iFormat);

#endif // __ZUtil_Tuple__
