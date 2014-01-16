static const char rcsid[] = "@(#) $Id: ZUtil_JSON.cpp,v 1.9 2006/10/13 20:03:26 agreen Exp $";

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

#include "ZUtil_JSON.h"

#include "ZStrimW_Escapify.h"
#include "ZTuple.h"
#include "ZUtil_Strim.h"

using std::string;
using std::vector;

/*! \namespace ZUtil_JSON
JSON is JavaScript Object Notation. See <http://www.crockford.com/JSON/index.html>.
This code reads and writes ZTupleValue, ZTuple and vector<ZTupleValue> in the syntax used
by JavaScript. This can be a convenient way to move structured data from code running in a
web page over an XMLHttpRequest to a server and back again.
*/

// =================================================================================================
#pragma mark -
#pragma mark * Helpers

static bool sTryRead_JSONString(const ZStrimU& s, string& oString)
	{
	using namespace ZUtil_Strim;

	if (sTryRead_CP(s, '"'))
		{
		// We've got a string, delimited by ".
		for (;;)
			{
			string tempString;
			sRead_EscapedString(s, '"', tempString);

			if (!sTryRead_CP(s, '"'))
				throw ParseException("Expected '\"' to close a string");

			oString += tempString;

			sSkip_WSAndCPlusPlusComments(s);

			if (!sTryRead_CP(s, '"'))
				return true;
			}
		}
	else if (sTryRead_CP(s, '\''))
		{
		// We've got a string, delimited by '.
		for (;;)
			{
			string tempString;
			sRead_EscapedString(s, '\'', tempString);

			if (!sTryRead_CP(s, '\''))
				throw ParseException("Expected \"'\" to close a string");

			oString += tempString;

			sSkip_WSAndCPlusPlusComments(s);

			if (!sTryRead_CP(s, '\''))
				return true;
			}
		}

	return false;
	}

static bool sTryRead_Name(const ZStrimU& s, string& oString)
	{
	for (bool gotAny = false; /*no test*/; gotAny = true)
		{
		UTF32 theCP;
		if (!s.ReadCP(theCP))
			return gotAny;

		if (theCP == ':'
			|| theCP == ','
			|| theCP == ']'
			|| theCP == '}'
			|| theCP == '/'
			|| (theCP != ' ' && ZUnicode::sIsWhitespace(theCP)))
			{
			s.Unread();
			return gotAny;
			}
		oString += theCP;
		}
	}

// Read the mantissa, putting it into oInt64 and oDouble and setting oIsDouble if
// it won't fit in 64 bits or is an inf. We consume any sign and set oIsNegative appropriately.
static bool sTryRead_Mantissa(const ZStrimU& s, int64& oInt64, double& oDouble, bool& oIsNegative, bool& oIsDouble)
	{
	using namespace ZUtil_Strim;

	oInt64 = 0;
	oDouble = 0;
	oIsNegative = false;
	oIsDouble = false;

	bool hadPrefix = true;
	if (sTryRead_CP(s, '-'))
		oIsNegative = true;
	else if (!sTryRead_CP(s, '+'))
		hadPrefix = false;

	// Unlike ZUtil_Strim, we don't read nan here. The first char of nan is the same
	// as that of null, and so we'd throw a parse error. Instead we handle it in sFromStrim.
	if (sTryRead_CP(s, 'i') || sTryRead_CP(s, 'I'))
		{
		if (sTryRead_CP(s, 'n') || sTryRead_CP(s, 'F'))
			{
			if (sTryRead_CP(s, 'f') || sTryRead_CP(s, 'F'))
				{
				// It's an inf
				oIsDouble = true;
				oDouble = INFINITY;
				return true;
				}
			}
		throw ParseException("Illegal character when trying to read a double");
		}

	for (bool gotAny = false; /*no test*/; gotAny = true)
		{
		UTF32 curCP;
		if (!s.ReadCP(curCP) || curCP == '.' || !ZUnicode::sIsDigit(curCP))
			{
			if (curCP == '.')
				{
				s.Unread();
				oIsDouble = true;
				return true;
				}

			if (gotAny)
				{	
				return true;
				}
			else
				{
				if (hadPrefix)
					{
					// We've already absorbed a plus or minus sign, hence we have a parse exception.
					if (oIsNegative)
						throw ParseException("Expected a valid number after '-' prefix");
					else
						throw ParseException("Expected a valid number after '+' prefix");
					}
				return false;
				}
			}

		int digit = curCP - '0';
		if (!oIsDouble)
			{
			int64 priorInt64 = oInt64;
			oInt64 *= 10;
			oInt64 += digit;
			if (oInt64 < priorInt64)
				oIsDouble = true;
			}
		oDouble *= 10;
		oDouble += digit;
		}
	}

static bool sTryRead_Number(const ZStrimU& s, ZTupleValue& oTV)
	{
	using namespace ZUtil_Strim;

	int64 asInt64;
	double asDouble;
	bool isNegative;
	bool isDouble;
	if (!sTryRead_Mantissa(s, asInt64, asDouble, isNegative, isDouble))
		return false;

	if (sTryRead_CP(s, '.'))
		{
		isDouble = true;
		double fracPart = 0.0;
		double divisor = 1.0;
		for (;;)
			{
			UTF32 curCP;
			if (!s.ReadCP(curCP))
				break;
			divisor *= 10;
			fracPart *= 10;
			fracPart += (curCP - '0');
			}
		asDouble += fracPart / divisor;
		}

	if (isNegative)
		{
		asInt64 = -asInt64;
		asDouble = -asDouble;
		}

	if (sTryRead_CP(s, 'e') || sTryRead_CP(s, 'E'))
		{
		isDouble = true;
		int64 exponent;
		if (!sTryRead_SignedDecimalInteger(s, exponent))
			throw ParseException("Expected a valid exponent after 'e'");
		asDouble = asDouble * pow(10.0, int(exponent));
		}

	if (isDouble)
		oTV.SetDouble(asDouble);
	else
		oTV.SetInt64(asInt64);

	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_JSON

void ZUtil_JSON::sToStrim(const ZStrimW& s, const ZTupleValue& iTV)
	{
	switch (iTV.TypeOf())
		{
		case eZType_Tuple:
			{
			sToStrim(s, iTV.GetTuple());
			break;
			}
		case eZType_Vector:
			{
			sToStrim(s, iTV.GetVector());
			break;
			}
		case eZType_String:
			{
			s << "\"";
			ZStrimW_Escapify(s) << iTV.GetString();
			s << "\"";
			break;
			}
		case eZType_Int8:
			{
			s.Writef("%d", iTV.GetInt8());
			break;
			}
		case eZType_Int16:
			{
			s.Writef("%d", iTV.GetInt16());
			break;
			}
		case eZType_Int32:
			{
			s.Writef("%d", iTV.GetInt32());
			break;
			}
		case eZType_Int64:
			{
			s.Writef("%lld", iTV.GetInt64());
			break;
			}
		case eZType_Float:
			{
			// 9 decimal digits are necessary and sufficient for single precision IEEE 754.
			// "What Every Computer Scientist Should Know About Floating Point", Goldberg, 1991.
			// <http://docs.sun.com/source/806-3568/ncg_goldberg.html>
			s.Writef("%.9g", iTV.GetFloat());
			break;
			}
		case eZType_Double:
			{
			// 17 decimal digits are necessary and sufficient for double precision IEEE 754.
			s.Writef("%.17g", iTV.GetDouble());
			break;
			}
		case eZType_Bool:
			{
			if (iTV.GetBool())
				s << "true";
			else
				s << "false";
			break;
			}
		case eZType_Null:
			{
			s << "null";
			break;
			}
		}
	}

void ZUtil_JSON::sToStrim(const ZStrimW& s, const ZTuple& iTuple)
	{
	s << "{";
	if (iTuple)
		{
		const ZTuple::const_iterator theBegin = iTuple.begin();
		const ZTuple::const_iterator theEnd = iTuple.end();

		bool isFirst = true;
		for (ZTuple::const_iterator i = theBegin; i != theEnd; ++i)
			{
			switch (iTuple.TypeOf(i))
				{
				case eZType_Tuple:
				case eZType_Vector:
				case eZType_String:
				case eZType_Int8:
				case eZType_Int16:
				case eZType_Int32:
				case eZType_Int64:
				case eZType_Float:
				case eZType_Double:
				case eZType_Bool:
				case eZType_Null:
					{
					if (!isFirst)
						s.Write(", ");
					isFirst = false;

					s << "\"";
					ZStrimW_Escapify(s) << iTuple.NameOf(i);
					s << "\":";
					sToStrim(s, iTuple.GetValue(i));
					}
				}
			}
		}
	s << "}";
	}

void ZUtil_JSON::sToStrim(const ZStrimW& s, const vector<ZTupleValue>& iVector)
	{
	s << "[";
	bool isFirst = true;
	for (vector<ZTupleValue>::const_iterator i = iVector.begin(); i != iVector.end(); ++i)
		{
		switch ((*i).TypeOf())
			{
			case eZType_Tuple:
			case eZType_String:
			case eZType_Vector:
			case eZType_Int8:
			case eZType_Int16:
			case eZType_Int32:
			case eZType_Int64:
			case eZType_Float:
			case eZType_Double:
			case eZType_Bool:
			case eZType_Null:
				{
				if (!isFirst)
					s.Write(", ");
				isFirst = false;

				sToStrim(s, *i);
				}
			}
		}
	s << "]";
	}

bool ZUtil_JSON::sFromStrim(const ZStrimU& s, ZTupleValue& oTV)
	{
	using namespace ZUtil_Strim;

	sSkip_WSAndCPlusPlusComments(s);

	if (sTryRead_CP(s, '{'))
		{
		// We've got an object (a tuple)
		ZTuple& theTuple = oTV.SetMutableTuple();
		for (;;)
			{
			sSkip_WSAndCPlusPlusComments(s);

			string name;
			if (!sTryRead_JSONString(s, name) && !sTryRead_Name(s, name))
				break;
			
			sSkip_WSAndCPlusPlusComments(s);
			
			if (!sTryRead_CP(s, ':'))
				throw ParseException("Expected ':' after a member name");

			sSkip_WSAndCPlusPlusComments(s);

			if (!ZUtil_JSON::sFromStrim(s, theTuple.SetMutableNull(name)))
				throw ParseException("Expected a value after a member name");

			sSkip_WSAndCPlusPlusComments(s);
			
			if (!sTryRead_CP(s, ','))
				break;
			}

		sSkip_WSAndCPlusPlusComments(s);

		if (!sTryRead_CP(s, '}'))
			throw ParseException("Expected '}' to close an object");
		}
	else if (sTryRead_CP(s, '['))
		{
		// We've got an array (a vector)
		sSkip_WSAndCPlusPlusComments(s);

		vector<ZTupleValue>& theVector = oTV.SetMutableVector();
		for (;;)
			{
			theVector.push_back(ZTupleValue());
			if (!sFromStrim(s, theVector.back()))
				{
				// Lose the unread tupleValue.
				theVector.pop_back();
				break;
				}

			sSkip_WSAndCPlusPlusComments(s);

			if (!sTryRead_CP(s, ','))
				break;
			}

		sSkip_WSAndCPlusPlusComments(s);

		if (!sTryRead_CP(s, ']'))
			throw ParseException("Expected ']' to close an array");
		}
	else
		{
		string theString;
		if (sTryRead_JSONString(s, theString))
			{
			oTV.SetString(theString);
			}
		else
			{
			if (!sTryRead_Name(s, theString))
				return false;

			if (!sTryRead_Number(ZStrimU_String(theString), oTV))
				{
				string lowerString = ZUnicode::sToLower(theString);
				if (lowerString == "nan")
					{
					oTV.SetDouble(NAN);
					}
				else if (lowerString == "null")
					{
					oTV.SetNull();
					}
				else if (lowerString == "false")
					{
					oTV.SetBool(false);
					}
				else if (lowerString == "true")
					{
					oTV.SetBool(true);
					}
				else
					{
					oTV.SetString(theString);
					}
				}
			}
		}
	return true;
	}

void ZUtil_JSON::sNormalize(ZTupleValue& ioTV)
	{
	sNormalize(false, true, ioTV);
	}

bool ZUtil_JSON::sNormalize(bool iPreserveTuples, bool iPreserveVectors, ZTupleValue& ioTV)
	{
	switch (ioTV.TypeOf())
		{
		case eZType_Tuple:
			{
			ZTuple& theTuple = ioTV.GetMutableTuple();
			for (ZTuple::const_iterator i = theTuple.begin(); i != theTuple.end(); /*no increment*/)
				{
				if (!sNormalize(iPreserveTuples, iPreserveVectors, theTuple.GetMutableValue(i)) && !iPreserveTuples)
					i = theTuple.EraseAndReturn(i);
				}
			return true;
			}
		case eZType_Vector:
			{
			vector<ZTupleValue>& theVector = ioTV.GetMutableVector();
			for (vector<ZTupleValue>::iterator i = theVector.begin(); i != theVector.end(); /*no increment*/)
				{
				if (!sNormalize(iPreserveTuples, iPreserveVectors, *i) && !iPreserveVectors)
					i = theVector.erase(i);
				else
					++i;
				}
			return true;
			}
		case eZType_String:
		case eZType_Int64:
		case eZType_Double:
		case eZType_Bool:
		case eZType_Null:
			{
			return true;
			}
		case eZType_Int8:
			{
			ioTV.SetInt64(ioTV.GetInt8());
			return true;
			}
		case eZType_Int16:
			{
			ioTV.SetInt64(ioTV.GetInt16());
			return true;
			}
		case eZType_Int32:
			{
			ioTV.SetInt64(ioTV.GetInt32());
			return true;
			}
		case eZType_Float:
			{
			ioTV.SetDouble(ioTV.GetFloat());
			return true;
			}
		}
	return false;
	}

ZTupleValue ZUtil_JSON::sNormalized(const ZTupleValue& iTV)
	{
	ZTupleValue result;
	if (!sNormalized(false, true, iTV, result))
		result.SetNull();
	return result;
	}

ZTupleValue ZUtil_JSON::sNormalized(bool iPreserveTuples, bool iPreserveVectors, const ZTupleValue& iTV)
	{
	ZTupleValue result;
	if (!sNormalized(iPreserveTuples, iPreserveVectors, iTV, result))
		result.SetNull();
	return result;
	}

void ZUtil_JSON::sNormalized(const ZTupleValue& iTV, ZTupleValue& oTV)
	{ sNormalized(false, true, iTV, oTV); }

bool ZUtil_JSON::sNormalized(bool iPreserveTuples, bool iPreserveVectors, const ZTupleValue& iTV, ZTupleValue& oTV)
	{
	ZAssert(&iTV != &oTV);
	switch (iTV.TypeOf())
		{
		case eZType_Tuple:
			{
			const ZTuple& sourceTuple = iTV.GetTuple();
			ZTuple& destTuple = oTV.SetMutableTuple();
			for (ZTuple::const_iterator i = sourceTuple.begin(); i != sourceTuple.end(); ++i)
				{
				string name = sourceTuple.NameOf(i);
				ZTupleValue& destTV = destTuple.SetMutableNull(name);
				if (!sNormalized(iPreserveTuples, iPreserveVectors, sourceTuple.GetValue(i), destTV))
					{
					if (!iPreserveTuples)
						destTuple.Erase(name);
					}
				}
			return true;
			}
		case eZType_Vector:
			{
			const vector<ZTupleValue>& sourceVector = iTV.GetVector();
			vector<ZTupleValue>& destVector = oTV.SetMutableVector();
			for (vector<ZTupleValue>::const_iterator i = sourceVector.begin(); i != sourceVector.end(); ++i)
				{
				destVector.push_back(ZTupleValue());
				if (!sNormalized(iPreserveTuples, iPreserveVectors, *i, destVector.back()))
					{
					if (!iPreserveVectors)
						destVector.pop_back();
					}
				}
			return true;
			}
		case eZType_String:
		case eZType_Int64:
		case eZType_Double:
		case eZType_Bool:
		case eZType_Null:
			{
			oTV = iTV;
			return true;
			}
		case eZType_Int8:
			{
			oTV.SetInt64(iTV.GetInt8());
			return true;
			}
		case eZType_Int16:
			{
			oTV.SetInt64(iTV.GetInt16());
			return true;
			}
		case eZType_Int32:
			{
			oTV.SetInt64(iTV.GetInt32());
			return true;
			}
		case eZType_Float:
			{
			oTV.SetDouble(iTV.GetFloat());
			return true;
			}
		}
	return false;
	}
