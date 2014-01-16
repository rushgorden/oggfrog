static const char rcsid[] = "@(#) $Id: ZUtil_Tuple.cpp,v 1.41 2006/11/02 18:25:00 agreen Exp $";

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

#include "ZUtil_Tuple.h"

#include "ZDebug.h"
#include "ZMemoryBlock.h"
#include "ZStream_HexStrim.h"
#include "ZStream_Memory.h"
#include "ZStrimR_Boundary.h"
#include "ZStrimW_Escapify.h"
#include "ZString.h"
#include "ZTuple.h"
#include "ZUtil_Strim.h"
#include "ZUtil_Time.h"

#include <cctype>
#include <cmath> // For fmod

using std::pair;
using std::string;
using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

static void sWriteIndent(const ZStrimW& iStrimW,
	size_t iCount, const ZUtil_Tuple::Options& iOptions)
	{
	while (iCount--)
		iStrimW.Write(iOptions.fIndentString);
	}

static void sWriteLFIndent(const ZStrimW& iStrimW,
	size_t iCount, const ZUtil_Tuple::Options& iOptions)
	{
	iStrimW.Write(iOptions.fEOLString);
	sWriteIndent(iStrimW, iCount, iOptions);
	}

static bool sContainsProblemChars(const string& iString)
	{
	if (iString.empty())
		{
		// An empty string can't be distinguished from no string at all, so
		// we treat it as if it has problem chars, so it will be wrapped in quotes.
		return true;
		}

	for (string::const_iterator i = iString.begin(), end = iString.end();;)
		{
		UTF32 theCP;
		if (!ZUnicode::sReadInc(i, end, theCP))
			break;
		if (!ZUnicode::sIsAlphaDigit(theCP) && '_' != theCP)
			return true;
		}

	return false;
	}

static bool sIsComplexString(const ZUtil_Tuple::Options& iOptions, const string& iString)
	{
	if (iOptions.fStringLineLength && iString.size() > iOptions.fStringLineLength)
		{
		// We have a non-zero desired line length, and the string contains
		// more characters than that. This doesn't account for increases in width
		// due to escaping etc.
		return true;
		}

	if (iOptions.fBreakStrings)
		{
		// We've been asked to break strings at line ending characters,
		// which means (here and in ZStrimW_Escapify) LF and CR. Strictly
		// speaking we should use ZUnicode::sIsEOL().
		if (string::npos != iString.find_first_of("\n\r"))
			return true;
		}

	return false;
	}

static bool sIsComplex(const ZUtil_Tuple::Options& iOptions, const ZTupleValue& iTV)
	{
	switch (iTV.TypeOf())
		{
		case eZType_Raw:
			{
			size_t theSize;
			iTV.GetRawAttributes(nil, &theSize);
			return theSize > iOptions.fRawChunkSize;
			}
		case eZType_Vector: return !iTV.GetVector().empty();
		case eZType_Tuple: return !iTV.GetTuple().Empty();
		case eZType_String: return sIsComplexString(iOptions, iTV.GetString());
		default: return false;
		}
	}

static void sSimpleTupleValueToStrim(const ZStrimW& s, const ZTupleValue& iTV,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions)
	{
	switch (iTV.TypeOf())
		{
		case eZType_Null: s.Write("Null"); break;
		case eZType_Type:
			{
			s.Write("Type(");
			s.Write(ZTypeAsString(iTV.GetType()));
			s.Write(")");
			break;
			}
		case eZType_ID:
			{
			s.Writef("ID(0x%0llX)", iTV.GetID());
			if (iOptions.fIDsHaveDecimalVersionComment)
				s.Writef(" /* %lld */", iTV.GetID());
			break;
			}
		case eZType_Int8: s.Writef("int8(%d)", iTV.GetInt8()); break;
		case eZType_Int16: s.Writef("int16(%d)", iTV.GetInt16()); break;
		case eZType_Int32: s.Writef("int32(%d)", iTV.GetInt32()); break;
		case eZType_Int64: s.Writef("int64(0x%0llX)", iTV.GetInt64()); break;
		case eZType_Bool:
			{
			if (iTV.GetBool())
				s.Write("true");
			else
				s.Write("false");
			break;
			}
		case eZType_Float:
			{
			// 9 decimal digits are necessary and sufficient for single precision IEEE 754.
			// "What Every Computer Scientist Should Know About Floating Point", Goldberg, 1991.
			// <http://docs.sun.com/source/806-3568/ncg_goldberg.html>
			s.Writef("float(%.9g)", iTV.GetFloat());
			break;
			}
		case eZType_Double:
			{
			// 17 decimal digits are necessary and sufficient for double precision IEEE 754.
			s.Writef("double(%.17g)", iTV.GetDouble());
			break;
			}
		case eZType_Time:
			{
			// For the moment I'm just writing times as a count of seconds, putting
			// the broken-out Gregorian version in a comment. Later we can improve
			// the parsing of dates, and then we can write them in human readable form.
			if (ZTime theTime = iTV.GetTime())
				{
				s.Writef("time(%.17g)", theTime.fVal);
				if (iOptions.fTimesHaveUserLegibleComment)
					{
					s << " /*" << ZUtil_Time::sAsStringUTC(theTime, "%Y-%m-%dZ%H:%M:");
					// We've got about 10 significant digits in year (-10,000 to +10,000),
					// month, day, hour, minutes, and seconds, and no more than 17 in a double.
					// To get a leading zero in the seconds' tens column we add 100
					// to the count of seconds, so to get up to 7 digits in the fraction we
					// need to allow ten digits overall.
					s << ZString::sFormat("%.10g", 100.0 + fmod(theTime.fVal, 60)).substr(1);
					s << "*/";
					};
				}
			else
				{
				// We're now allowing empty parens to represent invalid times.
				s.Write("time()");
				}
			break;
			}
		case eZType_Pointer: s.Writef("pointer(%08X)", iTV.GetPointer()); break;
		case eZType_Rect:
			{
			const ZRectPOD& theRect = iTV.GetRect();
			s.Writef("Rect(%d, %d, %d, %d)",
				theRect.left,
				theRect.top,
				theRect.right,
				theRect.bottom);
			break;
			}
		case eZType_Point:
			{
			const ZPointPOD& thePoint = iTV.GetPoint();
			s.Writef("Point(%d, %d)",
				thePoint.h,
				thePoint.v);
			break;
			}
		case eZType_String:
			{
			const string& theString = iTV.GetString();
			if (iOptions.fBreakStrings && iOptions.DoIndentation())
				{
				if (string::npos != theString.find_first_of("\n\r"))
					{
					// We put a newline after the opening """, which will be
					// ignored by sFromStrim, so the first line of theString
					// will be in column zero.
					s << "\"\"\"\n";
					ZStrimU_String strim_String(theString);
					ZStrimR_Boundary strim_Boundary("\"\"\"", strim_String);
					for (;;)
						{
						s.CopyAllFrom(strim_Boundary);
						if (!strim_Boundary.HitBoundary())
							{
							// We've returned without having hit the boundary, so we're done.
							break;
							}
						strim_Boundary.Reset();

						// Close the triple quotes.
						s << "\"\"\"";
						// A space to separate the triple-quote from the single quote
						s << " ";
						// An open quote
						s << "\"";
						// Three escaped quotes.
						s << "\\\"\\\"\\\"";
						// A close quote.
						s << "\"";
						// Another space, for symmetry
						s << " ";
						// And re-open triple quotes again.
						s << "\"\"\"";
						// With a newline, so the text will again
						// start in column zero.
						s << "\n";
						}
					s << "\"\"\"";
					break;
					}
				}
			string delimiter = "\"";
			bool quoteQuotes = true;
			if (string::npos != theString.find('"') && string::npos == theString.find('\''))
				{
				delimiter = "'";
				quoteQuotes = false;
				}

			s.Write(delimiter);

			ZStrimW_Escapify::Options theOptions;
			theOptions.fQuoteQuotes = quoteQuotes;
			theOptions.fEscapeHighUnicode = false;
			
			ZStrimW_Escapify(theOptions, s).Write(theString);

			s.Write(delimiter);
			break;
			}
		case eZType_RefCounted:
			s.Writef("RefCounted(%08X)", iTV.GetRefCounted().GetObject());
			break;
		case eZType_Raw:
		case eZType_Tuple:
		case eZType_Vector:
			{
			ZDebugStopf(0,
				("sSimpleTupleValueToStrim should only be called on simple tuple values"));
			break;
			}
		default:
			{
			ZDebugStopf(0, ("Unrecognized type %d", iTV.TypeOf()));
			break;
			}
		}
	}

static void sToStrim_Vector(const ZStrimW& s, const vector<ZTupleValue>& iVector,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions, bool iMayNeedInitialLF);

static void sToStrim_Tuple(const ZStrimW& s, const ZTuple& iTuple,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions, bool iMayNeedInitialLF);

static void sToStrim_TupleValue(const ZStrimW& s, const ZTupleValue& iTV,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions, bool iMayNeedInitialLF)
	{
	switch (iTV.TypeOf())
		{
		case eZType_Vector:
			{
			sToStrim_Vector(s, iTV.GetVector(), iLevel, iOptions, iMayNeedInitialLF);
			break;
			}
		case eZType_Tuple:
			{
			sToStrim_Tuple(s, iTV.GetTuple(), iLevel, iOptions, iMayNeedInitialLF);
			break;
			}
		case eZType_Raw:
			{
			const void* theData;
			size_t theSize;
			iTV.GetRawAttributes(&theData, &theSize);
			if (theSize == 0)
				{
				// we've got an empty Raw
				s.Write("()");
				}
			else
				{
				ZStreamRPos_Memory dataStream(theData, theSize);

				if (iOptions.DoIndentation() && theSize > iOptions.fRawChunkSize)
					{
					if (iMayNeedInitialLF)
						sWriteLFIndent(s, iLevel, iOptions);
					
					s.Writef("( // %d bytes", theSize);
					sWriteLFIndent(s, iLevel, iOptions);
					if (iOptions.fRawAsASCII)
						{
						for (;;)
							{
							uint64 lastPos = dataStream.GetPosition();
							uint64 countCopied;
							ZStreamW_HexStrim(iOptions.fRawByteSeparator, "", 0, s)
								.CopyFrom(dataStream, iOptions.fRawChunkSize, &countCopied, nil);

							if (countCopied == 0)
								break;

							dataStream.SetPosition(lastPos);
							if (size_t extraSpaces = iOptions.fRawChunkSize - countCopied)
								{
								// We didn't write a complete line of bytes, so pad it out.
								while (extraSpaces--)
									{
									// Two spaces for the two nibbles
									s.Write("  ");
									// And then the separator sequence
									s.Write(iOptions.fRawByteSeparator);
									}
								}

							s.Write(" // ");
							while (countCopied--)
								{
								char theChar = dataStream.ReadInt8();
								if (theChar < 0x20 || theChar > 0x7E)
									s.WriteCP('.');
								else
									s.WriteCP(theChar);
								}
							sWriteLFIndent(s, iLevel, iOptions);
							}
						}
					else
						{
						string eol = iOptions.fEOLString;
						for (size_t x = 0; x < iLevel; ++x)
							eol += iOptions.fIndentString;

						ZStreamW_HexStrim(iOptions.fRawByteSeparator,
							eol, iOptions.fRawChunkSize, s).CopyAllFrom(dataStream);

						sWriteLFIndent(s, iLevel, iOptions);
						}

					s.Write(")");
					}
				else
					{
					s.Write("(");

					ZStreamW_HexStrim(iOptions.fRawByteSeparator, "", 0, s)
						.CopyAllFrom(dataStream);

					if (iOptions.fRawAsASCII)
						{
						dataStream.SetPosition(0);
						s.Write(" /* ");
						while (theSize--)
							{
							char theChar = dataStream.ReadInt8();
							if (theChar < 0x20 || theChar > 0x7E)
								s.WriteCP('.');
							else
								s.WriteCP(theChar);
							}
						s.Write(" */");
						}
					s.Write(")");
					}
				}
			break;
			}
		default:
			{
			// We've got something other than a tuple or a vector.
			sSimpleTupleValueToStrim(s, iTV, iLevel, iOptions);
			break;
			}
		}
	}

static void sToStrim_Vector(const ZStrimW& s, const vector<ZTupleValue>& iVector,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions, bool iMayNeedInitialLF)
	{
	if (iVector.empty())
		{
		// We've got an empty vector.
		s.Write("[]");
		return;
		}
		
	vector<ZTupleValue>::const_iterator theBegin = iVector.begin();
	vector<ZTupleValue>::const_iterator theEnd = iVector.end();
	bool needsIndentation = false;
	if (iOptions.DoIndentation())
		{
		// We're supposed to be indenting if we're complex, ie if any element is:
		// 1. A non-empty vector.
		// 2. A non-empty tuple.
		// or if iOptions.fBreakStrings is true, any element is a string with embedded
		// line breaks or more than iOptions.fStringLineLength characters.
		for (vector<ZTupleValue>::const_iterator i = theBegin; i != theEnd; ++i)
			{
			if (sIsComplex(iOptions, *i))
				{
				needsIndentation = true;
				break;
				}
			}
		}

	if (needsIndentation)
		{
		// We need to indent.
		if (iMayNeedInitialLF)
			{
			// We were invoked by a tuple which has already issued the property
			// name and equals sign, so we need to start a fresh line.
			sWriteLFIndent(s, iLevel, iOptions);
			}

		s.Write("[");
		for (vector<ZTupleValue>::const_iterator i = theBegin;;)
			{
			sWriteLFIndent(s, iLevel, iOptions);
			sToStrim_TupleValue(s, *i, iLevel, iOptions, false);
			++i;
			if (i == theEnd)
				break;
			s.Write(", ");
			}
		sWriteLFIndent(s, iLevel, iOptions);
		s.Write("]");
		}
	else
		{
		// We're not indenting, so we can just dump everything out on
		// one line, with just some spaces to keep things legible.
		s.Write("[");
		for (vector<ZTupleValue>::const_iterator i = theBegin;;)
			{
			sToStrim_TupleValue(s, *i, iLevel, iOptions, false);
			++i;
			if (i == theEnd)
				break;
			s.Write(", ");
			}
		s.Write("]");
		}
	}

static void sToStrim_Tuple(const ZStrimW& s, const ZTuple& iTuple,
	size_t iLevel, const ZUtil_Tuple::Options& iOptions, bool iMayNeedInitialLF)
	{
	if (iTuple.Empty())
		{
		// We've got an empty tuple.
		s.Write("{}");
		return;
		}

	const ZTuple::const_iterator theBegin = iTuple.begin();
	const ZTuple::const_iterator theEnd = iTuple.end();
	bool needsIndentation = false;
	if (iOptions.DoIndentation())
		{
		for (ZTuple::const_iterator i = theBegin; i != theEnd; ++i)
			{
			if (sIsComplex(iOptions, iTuple.GetValue(i)))
				{
				needsIndentation = true;
				break;
				}
			}
		}

	if (needsIndentation)
		{
		if (iMayNeedInitialLF)
			{
			// We're going to be indenting, but need to start
			// a fresh line to have our { and contents line up.
			sWriteLFIndent(s, iLevel, iOptions);
			}
		s.Write("{");
		for (ZTuple::const_iterator i = theBegin; i != theEnd; ++i)
			{
			sWriteLFIndent(s, iLevel, iOptions);

			ZUtil_Tuple::sWrite_PropName(s, iTuple.NameOf(i));

			s << " = ";

			sToStrim_TupleValue(s, iTuple.GetValue(i), iLevel + 1, iOptions, true);
			s.Write(";");
			}

		sWriteLFIndent(s, iLevel, iOptions);

		s.Write("}");
		}
	else
		{
		s.Write("{");
		for (ZTuple::const_iterator i = theBegin; i != theEnd; ++i)
			{
			s.Write(" ");

			ZUtil_Tuple::sWrite_PropName(s, iTuple.NameOf(i));

			s << " = ";

			sToStrim_TupleValue(s, iTuple.GetValue(i), iLevel + 1, iOptions, true);
			s.Write(";");
			}

		s.Write(" }");
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * Static parsing methods.

static int64 sMustRead_GenericInteger(const ZStrimU& iStrimU)
	{
	using namespace ZUtil_Strim;
	int64 theInteger;
	if (!sTryRead_GenericInteger(iStrimU, theInteger))
		throw ParseException("Expected an integer");
	return theInteger;
	}

static void sMustRead_WSCommaWS(const ZStrimU& iStrimU)
	{
	using namespace ZUtil_Strim;
	sSkip_WSAndCPlusPlusComments(iStrimU);

	if (!sTryRead_CP(iStrimU, ','))
		throw ParseException("Expected a ','");

	sSkip_WSAndCPlusPlusComments(iStrimU);
	}

static void sReadQuotedString_Quote(const ZStrimU& iStrimU, ZTupleValue& oTupleValue)
	{
	using namespace ZUtil_Strim;

	string theString;
	theString.reserve(100);
	ZStrimW_String theStrimW(theString);

	for (;;)
		{
		// We've read, and could un-read, a quote mark.
		if (sTryRead_CP(iStrimU, '"'))
			{
			// We've now seen a second quote, abutting the first.
			if (sTryRead_CP(iStrimU, '"'))
				{
				// We have three quotes in a row, which opens a verbatim string.
				// If the next character is an EOL then absorb it, so the verbatim
				// text can start on a fresh line, but not be parsed as
				// beginning with an EOL.
				UTF32 theCP = iStrimU.ReadCP();
				if (!ZUnicode::sIsEOL(theCP))
					iStrimU.Unread();

				// Now copy everything till we see three quotes in a row again.
				ZStrimR_Boundary theStrimR_Boundary("\"\"\"", iStrimU);
				theStrimW.CopyAllFrom(theStrimR_Boundary);
				if (!theStrimR_Boundary.HitBoundary())
					throw ParseException("Expected \"\"\" to close a string");

				if (sTryRead_CP(iStrimU, '"'))
					{
					// We have another quote, so there were at least four in a row,
					// which we get with a quote in the text immediately followed
					// by the triple quote. So emit a quote.
					theStrimW.WriteCP('"');
					if (sTryRead_CP(iStrimU, '"'))
						{
						// Same again -- five quotes in a row, which is two content
						// quotes followed by the closing triple.
						theStrimW.WriteCP('"');
						// This is why it's essential that when using triple quotes
						// you put whitespace before the opening, and after the closing
						// triple, so we don't mistake included quotes for ones that
						// are (say) opening a subsequent regular quoted sequence.
						}
					}
				}
			else
				{
				// We have two quotes in a row, followed by something else, so
				// we had an empty string segment.
				}
			}
		else
			{
			sCopy_EscapedString(iStrimU, '"', theStrimW);
			if (!sTryRead_CP(iStrimU, '"'))
				throw ParseException("Expected \" to close a string");
			}

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '"'))
			break;
		}

	oTupleValue.SetString(theString);
	}

static void sReadQuotedString_Apos(const ZStrimU& iStrimU, ZTupleValue& oTupleValue)
	{
	using namespace ZUtil_Strim;

	string theString;
	theString.reserve(100);
	ZStrimW_String theStrimW(theString);

	for (;;)
		{
		sCopy_EscapedString(iStrimU, '\'', theStrimW);
		if (!sTryRead_CP(iStrimU, '\''))
			throw ParseException("Expected ' to close a string");

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '\''))
			break;
		}

	oTupleValue.SetString(theString);
	}

static void sFromStrim_BodyOfVector(const ZStrimU& iStrimU, vector<ZTupleValue>& oVector);
static void sFromStrim_BodyOfTuple(const ZStrimU& iStrimU, ZTuple& oTuple);

static bool sFromStrim_TupleValue(const ZStrimU& iStrimU, ZTupleValue& oTupleValue)
	{
	using namespace ZUtil_Strim;

	sSkip_WSAndCPlusPlusComments(iStrimU);

	if (sTryRead_CP(iStrimU, '['))
		{
		sFromStrim_BodyOfVector(iStrimU, oTupleValue.SetMutableVector());

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, ']'))
			throw ParseException("Expected ']' to close a vector");
		}
	else if (sTryRead_CP(iStrimU, '{'))
		{
		// It's a tuple.
		sFromStrim_BodyOfTuple(iStrimU, oTupleValue.SetMutableTuple());

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '}'))
			throw ParseException("Expected '}' to close a tuple");
		}
	else if (sTryRead_CP(iStrimU, '"'))
		{
		// It's a string, delimited by ".
		sReadQuotedString_Quote(iStrimU, oTupleValue);
		}
	else if (sTryRead_CP(iStrimU, '\''))
		{
		// It's a string, delimited by '.
		sReadQuotedString_Apos(iStrimU, oTupleValue);
		}
	else if (sTryRead_CP(iStrimU, '('))
		{
		// It's a raw.
		ZMemoryBlock theMemoryBlock;
		ZStreamR_HexStrim(iStrimU).CopyAllTo(ZStreamRWPos_MemoryBlock(theMemoryBlock));
		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, ')'))
			throw ParseException("Expected ')' to close a raw");
		oTupleValue.SetRaw(theMemoryBlock);
		}
	else
		{
		string theTypeLC, theType;
		if (!ZUtil_Tuple::sRead_Identifier(iStrimU, &theTypeLC, &theType))
			{
			// We couldn't find any of the special characters, nor could
			// we read a type designator, so we fail to read a tuplevalue,
			// which is not a parse error at this stage -- it might be for our caller.
			return false;
			}

		if (theTypeLC == "null")
			{
			oTupleValue.SetNull();
			}
		else if (theTypeLC == "false")
			{
			oTupleValue.SetBool(false);
			}
		else if (theTypeLC == "true")
			{
			oTupleValue.SetBool(true);
			}
		else
			{
			sSkip_WSAndCPlusPlusComments(iStrimU);
			if (!sTryRead_CP(iStrimU, '('))
				throw ParseException("Expected '(' following a type designator");

			sSkip_WSAndCPlusPlusComments(iStrimU);

			if (theTypeLC == "type")
				{
				string typeValueLC, typeValue;
				if (!ZUtil_Tuple::sRead_Identifier(iStrimU, &typeValueLC, &typeValue))
					throw ParseException("Expected a type name");

				if (typeValueLC == "null") oTupleValue.SetType(eZType_Null);
				else if (typeValueLC == "string") oTupleValue.SetType(eZType_String);
				else if (typeValueLC == "cstring") oTupleValue.SetType(eZType_CString);
				else if (typeValueLC == "int8") oTupleValue.SetType(eZType_Int8);
				else if (typeValueLC == "int16") oTupleValue.SetType(eZType_Int16);
				else if (typeValueLC == "int32") oTupleValue.SetType(eZType_Int32);
				else if (typeValueLC == "int64") oTupleValue.SetType(eZType_Int64);
				else if (typeValueLC == "float") oTupleValue.SetType(eZType_Float);
				else if (typeValueLC == "double") oTupleValue.SetType(eZType_Double);
				else if (typeValueLC == "time") oTupleValue.SetType(eZType_Time);
				else if (typeValueLC == "bool") oTupleValue.SetType(eZType_Bool);
				else if (typeValueLC == "pointer") oTupleValue.SetType(eZType_Pointer);
				else if (typeValueLC == "raw") oTupleValue.SetType(eZType_Raw);
				else if (typeValueLC == "tuple") oTupleValue.SetType(eZType_Tuple);
				else if (typeValueLC == "refcounted") oTupleValue.SetType(eZType_RefCounted);
				else if (typeValueLC == "rect") oTupleValue.SetType(eZType_Rect);
				else if (typeValueLC == "point") oTupleValue.SetType(eZType_Point);
				else if (typeValueLC == "region") oTupleValue.SetType(eZType_Region);
				else if (typeValueLC == "id") oTupleValue.SetType(eZType_ID);
				else if (typeValueLC == "vector") oTupleValue.SetType(eZType_Vector);
				else if (typeValueLC == "type") oTupleValue.SetType(eZType_Type);
				else if (typeValueLC == "time") oTupleValue.SetType(eZType_Time);
				else
					throw ParseException("Unknown type name '" + typeValue + "'");
				}
			else if (theTypeLC == "id")
				{
				oTupleValue.SetID(sMustRead_GenericInteger(iStrimU));
				}
			else if (theTypeLC == "int8")
				{
				oTupleValue.SetInt8(sMustRead_GenericInteger(iStrimU));
				}
			else if (theTypeLC == "int16")
				{
				oTupleValue.SetInt16(sMustRead_GenericInteger(iStrimU));
				}
			else if (theTypeLC == "int32")
				{
				oTupleValue.SetInt32(sMustRead_GenericInteger(iStrimU));
				}
			else if (theTypeLC == "int64")
				{
				oTupleValue.SetInt64(sMustRead_GenericInteger(iStrimU));
				}
			else if (theTypeLC == "bool")
				{
				string theBool;
				if (!ZUtil_Tuple::sRead_Identifier(iStrimU, &theBool, nil))
					throw ParseException("Expected 'false' or 'true'");

				if (theBool == "true")
					oTupleValue.SetBool(true);
				else if (theBool == "false")
					oTupleValue.SetBool(false);
				else
					throw ParseException("Expected 'false' or 'true'");
				}
			else if (theTypeLC == "float")
				{
				double theDouble;
				if (!sTryRead_SignedDouble(iStrimU, theDouble))
					throw ParseException("Expected a floating point number");
				oTupleValue.SetFloat(theDouble);
				}
			else if (theTypeLC == "double")
				{
				double theDouble;
				if (!sTryRead_SignedDouble(iStrimU, theDouble))
					throw ParseException("Expected a floating point number");
				oTupleValue.SetDouble(theDouble);
				}
			else if (theTypeLC == "time")
				{
				if (sTryRead_CP(iStrimU, ')'))
					{
					// It's a time with no content, hence an invalid time.
					oTupleValue.SetTime(ZTime());
					
					// We'll take an early exit so the normal code that
					// looks for a closing parenthesis doesn't choke.
					return true;
					}
				// Try to read a double, which is how we're representing
				// times in text streams for now.
				double theDouble;
				if (!sTryRead_SignedDouble(iStrimU, theDouble))
					throw ParseException("Expected a floating point time");
				oTupleValue.SetTime(theDouble);
				}
			else if (theTypeLC == "rect")
				{
				ZRectPOD theRect;

				theRect.left = sMustRead_GenericInteger(iStrimU);

				sMustRead_WSCommaWS(iStrimU);

				theRect.top = sMustRead_GenericInteger(iStrimU);

				sMustRead_WSCommaWS(iStrimU);

				theRect.right = sMustRead_GenericInteger(iStrimU);

				sMustRead_WSCommaWS(iStrimU);

				theRect.bottom = sMustRead_GenericInteger(iStrimU);

				oTupleValue.SetRect(theRect);
				}
			else if (theTypeLC == "point")
				{
				ZPointPOD thePoint;

				thePoint.h = sMustRead_GenericInteger(iStrimU);

				sMustRead_WSCommaWS(iStrimU);

				thePoint.v = sMustRead_GenericInteger(iStrimU);

				oTupleValue.SetPoint(thePoint);
				}
			else
				{
				throw ParseException("Unknown type designator '" + theType + "'");
				}

			sSkip_WSAndCPlusPlusComments(iStrimU);

			if (!sTryRead_CP(iStrimU, ')'))
				throw ParseException("Expected ')' to close a value");
			}
		}
	return true;
	}

static void sFromStrim_BodyOfVector(const ZStrimU& iStrimU, vector<ZTupleValue>& oVector)
	{
	using namespace ZUtil_Strim;
	for (;;)
		{
		sSkip_WSAndCPlusPlusComments(iStrimU);

		oVector.push_back(ZTupleValue());
		if (!sFromStrim_TupleValue(iStrimU, oVector.back()))
			{
			// Lose the unread tupleValue.
			oVector.pop_back();
			break;
			}

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, ';') && !sTryRead_CP(iStrimU, ','))
			break;
		}
	}

static void sFromStrim_BodyOfTuple(const ZStrimU& iStrimU, ZTuple& oTuple)
	{
	using namespace ZUtil_Strim;
	for (;;)
		{
		sSkip_WSAndCPlusPlusComments(iStrimU);

		string propertyName;
		if (!sTryRead_EscapedString(iStrimU, '"', propertyName))
			{
			if (!sTryRead_EscapedString(iStrimU, '\'', propertyName))
				{
				if (!ZUtil_Tuple::sRead_Identifier(iStrimU, nil, &propertyName))
					break;
				}
			}

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '='))
			throw ParseException("Expected '=' after property name");

		sSkip_WSAndCPlusPlusComments(iStrimU);

		ZTupleValue& theTupleValue = oTuple.SetMutableNull(propertyName);
		if (!sFromStrim_TupleValue(iStrimU, theTupleValue))
			throw ParseException("Expected a property value after '='");

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, ';') && !sTryRead_CP(iStrimU, ','))
			throw ParseException("Expected a ';' or a ',' after property value");
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * Options

ZUtil_Tuple::Options::Options(bool iDoIndentation)
:	fRawChunkSize(16),
	fRawByteSeparator(" "),
	fRawAsASCII(iDoIndentation),
	fBreakStrings(true),
	fStringLineLength(80),
	fIDsHaveDecimalVersionComment(iDoIndentation),
	fTimesHaveUserLegibleComment(true)
	{
	if (iDoIndentation)
		{
		fEOLString = "\n";
		fIndentString = "  ";
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * Format

ZUtil_Tuple::Format::Format(const ZTupleValue& iTupleValue, int iInitialIndent, const ZUtil_Tuple::Options& iOptions)
:	fTupleValue(iTupleValue),
	fInitialIndent(iInitialIndent),
	fOptions(iOptions)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Tuple, writing and parsing pieces

void ZUtil_Tuple::sWrite_PropName(const ZStrimW& iStrimW, const string& iPropName)
	{
	if (sContainsProblemChars(iPropName))
		{
		iStrimW << "\"";
		ZStrimW_Escapify(iStrimW) << iPropName;
		iStrimW << "\"";
		}
	else
		{
		iStrimW << iPropName;
		}
	}

bool ZUtil_Tuple::sRead_Identifier(const ZStrimU& iStrimU, string* oStringLC, string* oStringExact)
	{
	bool gotAny = false;
	for (;;)
		{
		UTF32 theCP;
		if (!iStrimU.ReadCP(theCP))
			break;
		if (!ZUnicode::sIsAlphaDigit(theCP) && theCP != '_')
			{
			iStrimU.Unread();
			break;
			}

		gotAny = true;

		if (oStringLC)
			*oStringLC += ZUnicode::sToLower(theCP);
		if (oStringExact)
			*oStringExact += theCP;
		}
	return gotAny;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleValue

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const ZTupleValue& iTV)
	{ sToStrim_TupleValue(s, iTV, 0, Options(), false); }

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const ZTupleValue& iTV,
	size_t iInitialIndent, const Options& iOptions)
	{ sToStrim_TupleValue(s, iTV, iInitialIndent, iOptions, false); }

string ZUtil_Tuple::sAsString(const ZTupleValue& iTV)
	{
	string theString;
	sToStrim(ZStrimW_String(theString), iTV);
	return theString;
	}

bool ZUtil_Tuple::sFromStrim(const ZStrimU& iStrimU, ZTupleValue& oTupleValue)
	{ return sFromStrim_TupleValue(iStrimU, oTupleValue); }

bool ZUtil_Tuple::sFromString(const string& iString, ZTupleValue& oTupleValue)
	{ return sFromStrim_TupleValue(ZStrimU_String(iString), oTupleValue); }

// =================================================================================================
#pragma mark -
#pragma mark * vector<ZTupleValue>

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const vector<ZTupleValue>& iVector)
	{ sToStrim_Vector(s, iVector, 0, Options(), false); }

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const vector<ZTupleValue>& iVector,
	size_t iInitialIndent, const Options& iOptions)
	{ sToStrim_Vector(s, iVector, iInitialIndent, iOptions, false); }

string ZUtil_Tuple::sAsString(const vector<ZTupleValue>& iVector)
	{
	string theString;
	sToStrim(ZStrimW_String(theString), iVector);
	return theString;
	}

bool ZUtil_Tuple::sFromStrim(const ZStrimU& iStrimU, vector<ZTupleValue>& oVector)
	{
	using namespace ZUtil_Strim;
	sSkip_WSAndCPlusPlusComments(iStrimU);

	UTF32 theCP;
	if (!iStrimU.ReadCP(theCP))
		return false;

	if (theCP != '[')
		throw ParseException("Expected '[' to open vector");

	sFromStrim_BodyOfVector(iStrimU, oVector);

	sSkip_WSAndCPlusPlusComments(iStrimU);

	if (!sTryRead_CP(iStrimU, ']'))
		throw ParseException("Expected ']' to close vector");

	return true;
	}

bool ZUtil_Tuple::sFromString(const string& iString, vector<ZTupleValue>& oVector)
	{ return sFromStrim(ZStrimU_String(iString), oVector); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const ZTuple& iTuple)
	{ sToStrim_Tuple(s, iTuple, 0, Options(), false); }

void ZUtil_Tuple::sToStrim(const ZStrimW& s, const ZTuple& iTuple,
	size_t iInitialIndent, const Options& iOptions)
	{ sToStrim_Tuple(s, iTuple, iInitialIndent, iOptions, false); }

string ZUtil_Tuple::sAsString(const ZTuple& iTuple)
	{
	string theString;
	sToStrim(ZStrimW_String(theString), iTuple);
	return theString;
	}

string ZUtil_Tuple::sAsString(const ZTuple& iTuple,
	size_t iInitialIndent, const Options& iOptions)
	{
	string theString;
	sToStrim(ZStrimW_String(theString), iTuple, iInitialIndent, iOptions);
	return theString;
	}

bool ZUtil_Tuple::sFromStrim(const ZStrimU& iStrimU, ZTuple& oTuple)
	{
	using namespace ZUtil_Strim;
	sSkip_WSAndCPlusPlusComments(iStrimU);

	UTF32 theCP;
	if (!iStrimU.ReadCP(theCP))
		return false;

	if (theCP != '{')
		throw ParseException("Expected '{' to open tuple");

	sFromStrim_BodyOfTuple(iStrimU, oTuple);

	sSkip_WSAndCPlusPlusComments(iStrimU);

	if (!sTryRead_CP(iStrimU, '}'))
		throw ParseException("Expected '}' to close tuple");

	return true;
	}

bool ZUtil_Tuple::sFromString(const string& iString, ZTuple& oTuple)
	{ return sFromStrim(ZStrimU_String(iString), oTuple); }

// =================================================================================================
#pragma mark -
#pragma mark * operator<< overloads

const ZStrimW& operator<<(const ZStrimW& s, const ZTupleValue& iTupleValue)
	{
	ZUtil_Tuple::sToStrim(s, iTupleValue);
	return s;
	}

const ZStrimW& operator<<(const ZStrimW& s, const ZUtil_Tuple::Format& iFormat)
	{
	ZUtil_Tuple::sToStrim(s, iFormat.fTupleValue, iFormat.fInitialIndent, iFormat.fOptions);
	return s;
	}
