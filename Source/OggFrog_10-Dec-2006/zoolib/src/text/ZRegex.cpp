static const char rcsid[] = "@(#) $Id: ZRegex.cpp,v 1.4 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZRegex.h"

/*
To use ZRegex you'll need to add an access path or search path that points
at the boost header root directory. You'll also need to add the following
source files from boost/libs/regex/src:
  c_regex_traits_common.cpp
  instances.cpp
  regex.cpp
  regex_synch.cpp

On MacOS X and Linux also add
  c_regex_traits.cpp

and on Windows,
  w32_regex_traits.cpp
*/


#if ZCONFIG_API_Enabled(Regex)

#include <boost/regex.hpp>

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZRegexRep

class ZRegexRep : public ZRefCounted
	{
public:
	ZRegexRep(const std::string& iRegex, int iStrength);
	virtual ~ZRegexRep();

	bool IsValid();
	bool Matches(const string& iTarget);

private:
	boost::regex fRegex;
	};

ZRegexRep::ZRegexRep(const std::string& iRegex, int iStrength)
:	fRegex(iRegex, iStrength ? boost::regex::icase : 0)
	{}

ZRegexRep::~ZRegexRep()
	{}

bool ZRegexRep::IsValid()
	{ return true; }

bool ZRegexRep::Matches(const string& iTarget)
	{ return boost::regex_match(iTarget, fRegex); }

// =================================================================================================
#pragma mark -
#pragma mark * ZRegex

ZRegex::ZRegex()
	{}

ZRegex::ZRegex(const ZRegex& iOther)
:	fRep(iOther.fRep)
	{}

ZRegex::ZRegex(const string& iRegex, int iStrength)
	{
	try
		{
		fRep = new ZRegexRep(iRegex, iStrength);
		}
	catch (...)
		{}
	}

ZRegex& ZRegex::operator=(const ZRegex& iOther)
	{
	fRep = iOther.fRep;
	return *this;
	}

ZRegex::~ZRegex()
	{}

ZRegex::operator operator_bool_type() const
	{ return operator_bool_generator_type::translate(fRep && fRep->IsValid()); }

bool ZRegex::Matches(const string& iTarget) const
	{
	if (fRep)
		return fRep->Matches(iTarget);
	return false;
	}

#endif // ZCONFIG_API_Enabled(Regex)
