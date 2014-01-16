static const char rcsid[] = "@(#) $Id: ZUtil_TBSpec.cpp,v 1.5 2006/10/26 15:12:26 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

#include "ZUtil_TBSpec.h"

#include "ZStrim.h"
#include "ZTBSpec.h"
#include "ZUtil_Strim.h"
#include "ZUtil_Tuple.h"

// =================================================================================================
#pragma mark -
#pragma mark * Static functions

static void sWrite_Comparator(const ZStrimW& s, const ZTBSpec::Comparator& iComparator)
	{
	switch (iComparator.fRel)
		{
		case ZTBSpec::eRel_Less:
			s << "<";
			break;
		case ZTBSpec::eRel_LessEqual:
			s << "<=";
			break;
		case ZTBSpec::eRel_Equal:
			s << "==";
			break;
		case ZTBSpec::eRel_Greater:
			s << ">";
			break;
		case ZTBSpec::eRel_GreaterEqual:
			s << ">=";
			break;
		case ZTBSpec::eRel_StringContains:
			s << "~";
			break;
		case ZTBSpec::eRel_Regex:
			s << "re";
			break;
		default:
			s << "!unsupported rel!";
			break;
		}
	if (iComparator.fStrength)
		s.Writef("%d", iComparator.fStrength);
	}

static void sWrite_Criterion(const ZStrimW& iStrimW, const ZTBSpec::Criterion& iCriterion)
	{
	ZUtil_Tuple::sWrite_PropName(iStrimW, iCriterion.GetPropName().AsString());

	iStrimW << " ";

	sWrite_Comparator(iStrimW, iCriterion.GetComparator());

	iStrimW << " " << iCriterion.GetTupleValue();
	}

static bool sRead_Rel(const ZStrimU& iStrimU, ZTBSpec::ERel& oRel)
	{
	using namespace ZUtil_Strim;

	if (sTryRead_CP(iStrimU, '<'))
		{
		if (sTryRead_CP(iStrimU, '='))
			{
			oRel = ZTBSpec::eRel_LessEqual;
			return true;
			}
		else
			{
			oRel = ZTBSpec::eRel_Less;
			return true;
			}
		}
	else if (sTryRead_CP(iStrimU, '>'))
		{
		if (sTryRead_CP(iStrimU, '='))
			{
			oRel = ZTBSpec::eRel_GreaterEqual;
			return true;
			}
		else
			{
			oRel = ZTBSpec::eRel_Greater;
			return true;
			}
		}
	else if (sTryRead_CP(iStrimU, '='))
		{
		if (sTryRead_CP(iStrimU, '='))
			{
			oRel = ZTBSpec::eRel_Equal;
			return true;
			}
		else
			{
			throw ParseException("Expected '=' after '='");
			}
		}
	else if (sTryRead_CP(iStrimU, '~'))
		{
		oRel = ZTBSpec::eRel_StringContains;
		return true;
		}
	else if (sTryRead_CP(iStrimU, 'r') || sTryRead_CP(iStrimU, 'R'))
		{
		if (sTryRead_CP(iStrimU, 'e') || sTryRead_CP(iStrimU, 'E'))
			{
			oRel = ZTBSpec::eRel_Regex;
			return true;
			}
		else
			{
			throw ParseException("Expected 're'");
			}
		}

	return false;
	}

static bool sRead_Comparator(const ZStrimU& iStrimU, ZTBSpec::Comparator& oComparator)
	{
	if (!sRead_Rel(iStrimU, oComparator.fRel))
		return false;

	int64 theInteger;
	if (!ZUtil_Strim::sTryRead_GenericInteger(iStrimU, theInteger))
		oComparator.fStrength = 0;
	else
		oComparator.fStrength = theInteger;

	return true;
	}

static bool sRead_Union(const ZStrimU& iStrimU, ZTBSpec& oSpec);

static bool sRead_Criterion(const ZStrimU& iStrimU, ZTBSpec& oSpec)
	{
	using namespace ZUtil_Strim;

	sSkip_WSAndCPlusPlusComments(iStrimU);

	if (sTryRead_CP(iStrimU, '('))
		{
		if (!sRead_Union(iStrimU, oSpec))
			throw ParseException("Expected an expression after '('");

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, ')'))
			throw ParseException("Expected ')' after expression");
		}
	else
		{
		string thePropertyName;
		string thePropertyNameLC;
		bool wasIdentifier = false;
		if (!sTryRead_EscapedString(iStrimU, '"', thePropertyName))
			{
			if (!sTryRead_EscapedString(iStrimU, '\'', thePropertyName))
				{				
				if (!ZUtil_Tuple::sRead_Identifier(iStrimU, &thePropertyNameLC, &thePropertyName))
					return false;
				wasIdentifier = true;
				}
			}

		sSkip_WSAndCPlusPlusComments(iStrimU);

		ZTBSpec::Comparator theComparator;
		if (!sRead_Comparator(iStrimU, theComparator))
			{
			if (wasIdentifier)
				{
				if (thePropertyNameLC == "any")
					{
					oSpec = ZTBSpec::sAny();
					return true;
					}
				else if (thePropertyNameLC == "none")
					{
					oSpec = ZTBSpec::sNone();
					return true;
					}
				}
			throw ParseException("Expected a relationship after a property name");
			}

		sSkip_WSAndCPlusPlusComments(iStrimU);

		ZTupleValue theTV;
		if (!ZUtil_Tuple::sFromStrim(iStrimU, theTV))
			throw ParseException("Expected a value after a relationship");

		oSpec = ZTBSpec(thePropertyName, theComparator, theTV);
		}
	return true;
	}

static bool sRead_Intersection(const ZStrimU& iStrimU, ZTBSpec& oSpec)
	{
	using namespace ZUtil_Strim;

	oSpec = ZTBSpec::sAny();
	bool gotAny = false;
	for (;;)
		{
		ZTBSpec theSpec;
		if (!sRead_Criterion(iStrimU, theSpec))
			break;

		gotAny = true;
		oSpec &= theSpec;

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '&'))
			break;
		}
	return gotAny;
	}

static bool sRead_Union(const ZStrimU& iStrimU, ZTBSpec& oSpec)
	{
	using namespace ZUtil_Strim;

	oSpec = ZTBSpec::sNone();
	bool gotAny = false;
	for (;;)
		{
		ZTBSpec theSpec;
		if (!sRead_Intersection(iStrimU, theSpec))
			break;

		gotAny = true;
		oSpec |= theSpec;

		sSkip_WSAndCPlusPlusComments(iStrimU);

		if (!sTryRead_CP(iStrimU, '|'))
			break;
		}
	return gotAny;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_TBSpec

/**
ZUtil_TBSpec holds functions that operate on and with ZTBSpec instances, but which
should not be part of ZTBSpec itself because they would then create spurious dependencies.
*/
namespace ZUtil_TBSpec {

void sToStrim(const ZStrimW& iStrimW, const ZTBSpec& iTBSpec)
	{
	typedef ZTBSpec::CriterionUnion TBU;
	typedef ZTBSpec::CriterionSect TBS;

	const TBU& theTBU = iTBSpec.GetCriterionUnion();

	if (theTBU.empty())
		{
		iStrimW << "none";
		}
	else
		{
		for (TBU::const_iterator iterU = theTBU.begin(); iterU != theTBU.end(); ++iterU)
			{
			if (iterU != theTBU.begin())
				iStrimW << " | ";

			if ((*iterU).empty())
				{
				iStrimW << "any";
				}
			else if ((*iterU).size() == 1)
				{
				sWrite_Criterion(iStrimW, *(*iterU).begin());
				}
			else
				{
				if (theTBU.size() > 1)
					iStrimW << "(";
				for (TBS::const_iterator iterS = (*iterU).begin(); iterS != (*iterU).end(); ++iterS)
					{
					if (iterS != (*iterU).begin())
						iStrimW << " & ";

					sWrite_Criterion(iStrimW, *iterS);
					}
				if (theTBU.size() > 1)
					iStrimW << ")";
				}
			}
		}
	}

/**
\brief Reads the text representation of a ZTBSpec from \a iStrimU, placing the result in \a oTBSpec.

\return true if a valid ZTBSpec was found.
\return false if no ZTBSpec could be read, for example if the strim contains no text.
\throw ZUtil_Strim::ParseException if some text was read, but was syntactically invalid.

The syntax is very simple. An expression consists of criteria conjoined by infix '&' and '|'
operators, precedence can be controlled by use of parentheses.
\verbatim
// Optional whitespace
ows = { whitespace, or C/C++ comment, as consumed by ZUtil_Strim::sSkip_WSAndCPlusPlusComments }

spec = intersection
spec = intersection ows '|' ows spec

intersection = criterion
intersection = criterion ows '&' ows intersection

criterion = '(' ows spec ows ')'
criterion = propertyName ows rel ows tupleValue

// A propertyName is a string wrapped in single or double quotes, or a
// string consumed by ZUtil_Tuple::sRead_Identifier.
propertyName = '"' [anyChar]* '"'
propertyName = '\'\ [anyChar]* '\''
propertyName = ['_' a-z A-Z 0-9]+

rel = '=='
rel = '<'
rel = '>'
rel = '<='
rel = '>='

tupleValue = { the syntax used by ZUtil_Tuple }
\endverbatim
*/
bool sFromStrim(const ZStrimU& iStrimU, ZTBSpec& oTBSpec)
	{
	return sRead_Union(iStrimU, oTBSpec);
	}

} // namespace ZUtil_TBSpec

// =================================================================================================
#pragma mark -
#pragma mark * operator<< overloads

const ZStrimW& operator<<(const ZStrimW& s, const ZTBSpec& iTBSpec)
	{
	ZUtil_TBSpec::sToStrim(s, iTBSpec);
	return s;
	}
