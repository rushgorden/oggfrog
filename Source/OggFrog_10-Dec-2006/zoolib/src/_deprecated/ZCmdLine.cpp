static const char rcsid[] = "@(#) $Id: ZCmdLine.cpp,v 1.7 2006/04/10 20:44:19 agreen Exp $";

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

#include "ZCmdLine.h"
#include "ZDebug.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::ostream;
using std::string;
using std::vector;

// =================================================================================================

// Static stuff
ZCmdLine* ZCmdLine::sCurrentCmdLine = nil;

// ==================================================
ZCmdLine::ZCmdLine(bool ignoreUnknownOptions, bool ignoreUnknownArgs)
	{
	fAppName = nil;
	fIgnoreUnknownOptions = ignoreUnknownOptions;
	fIgnoreUnknownArgs = ignoreUnknownArgs;
	sCurrentCmdLine = this;
	}

const char* ZCmdLine::GetAppName() const
	{ return fAppName; }

bool ZCmdLine::Parse(int argc, char* argv[])
	{
	fAppName = argv[0];

	// First try and grab values from environment variables
	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		(*i)->SetupFromEnvironmentVariable();

	return this->Internal_Parse(argc-1, argv+1);
	}

void ZCmdLine::PrintUsage(ostream& theStream)
	{
	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		(*i)->PrintUsage(theStream);
	theStream << endl;

// Print the explanations
	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		(*i)->PrintUsageExtended(theStream);
	theStream << endl;
	}

void ZCmdLine::AddArgument(Argument* theArgument)
	{ fArguments.push_back(theArgument); }

bool ZCmdLine::Internal_Parse(int argc, char** argv)
	{
	while (argc-- > 0)
		{
		if (*argv[0] == '-')
			{
			if (!this->Internal_ParseFlags(argc, argv))
				return false;
			}
		else
			{
			if (!this->Internal_ParseArgument(*argv))
				return false;
			}
		++argv;
		}

	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		{
		if (!(*i)->HasValue() && (*i)->IsRequired())
			{
			cerr << "Missing required argument: " << (*i)->fName << endl;
			return false;
			}
		}
	return true;
	}

bool ZCmdLine::Internal_ParseArgument(const char* argPtr)
	{
	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		{
		if (!(*i)->IsArgument())
			continue;
		if ((*i)->IsUsed())
			continue;
		(*i)->SetValueFromLexeme(argPtr); 
		return true;
		}

	if (!fIgnoreUnknownArgs)
		{
		cerr << "Unexpected argument: " << argPtr;
		return false;
		}
	return true;
	}

bool ZCmdLine::Internal_ParseFlags(int& argc, char**& argv)
	{
	for (vector<Argument*>::iterator i = fArguments.begin(); i != fArguments.end(); ++i)
		{
		if ((*i)->IsArgument())
			continue;
		if ((*i)->IsUsed())
			continue;
//##		if ((*i)->IsUsed() && !((*i)->fFlags & ZCmdLine::fl_multiple))
//##			continue;
// Get the lexeme of this argument if any
		char* lexeme = nil;
		if ((strlen((*i)->GetName()) == 1) && (((*i)->GetName())[0] == argv[0][1]))
			{
// It's a matching single character option (like this: -f)
			if (argv[0][2] == '\0')
				{
// There's whitespace immediately after the option name, try to pick up the next argv string as
// the argument for this option.
				if ((*i)->HasLength())
					{
					++argv;
					--argc;
					if (argv[0] == nil)
						{
						cerr << "Missing argument for option " << (*i)->GetName() << endl;
						return false;
						}
					lexeme = argv[0];
					}
				else
					lexeme = "";
				}
			else
				lexeme = &argv[0][2];
			}
		else if (strcmp((*i)->GetName(), &argv[0][1]) == 0)
			{
// It's a matching multi-character option (like this: -aLongOptionName)
			if ((*i)->HasLength())
				{
// We require whitespace after a multi character name, so just bump on to the next argv
				++argv;
				--argc;
				if (argv[0] == nil)
					{
					cerr << "Missing argument for option " << (*i)->GetName() << endl;
					return false;
					}
				lexeme = argv[0];
				}
			else
				lexeme = "";
			}
		else
			continue;


		ZAssertStop(2, lexeme != nil);
		(*i)->SetValueFromLexeme(lexeme);
		return true;
		}

	if (!fIgnoreUnknownOptions)
		{
		cerr << "Unknown option " << argv[0] << endl;
		return false;
		}
	return true;
	}

// ==================================================

ZCmdLine::Argument::Argument(const char* inName, const char* inDescription, bool inIsArgument, bool inIsRequired, const char* inEnvironmentVariable) 
:	fName(inName), fDescription(inDescription), fIsArgument(inIsArgument), fIsRequired(inIsRequired), fEnvironmentVariable(inEnvironmentVariable)
	{
	fIsUsed = false;
	fHasValue = false;

	ZAssertStop(2, ZCmdLine::sCurrentCmdLine);
	ZCmdLine::sCurrentCmdLine->AddArgument(this);
	}

void ZCmdLine::Argument::PrintUsage(ostream& str)
	{
	if (!fIsRequired)
		str << "[";
	if (!fIsArgument)
		str << "-";
	str << fName << this->GetType();
//	if (fFlags & ZCmdLine::fl_array) str << " ...";
	if (!fIsRequired)
		str << "]";
	str << " ";
	}

void ZCmdLine::Argument::PrintUsageExtended(ostream& str)
	{
	str << "\t\"";
	if (!fIsArgument)
		str << "-";
	str << fName << "\"" << " ";
	str << fDescription << " ";
	if (fEnvironmentVariable != nil)
		str << " ENV: $" << fEnvironmentVariable;
	if (!fIsRequired)
		{
		str << " Default: ";
		this->PrintDefaultValue(str);
		}
	str << endl;
	}

bool ZCmdLine::Argument::IsRequired() const
	{ return fIsRequired; }

bool ZCmdLine::Argument::IsArgument() const
	{ return fIsArgument; }

const char* ZCmdLine::Argument::GetName()
	const { return fName; }


bool ZCmdLine::Argument::HasLength() const
	{ return true; }

bool ZCmdLine::Argument::IsUsed() const
	{ return fIsUsed; }

bool ZCmdLine::Argument::HasValue() const
	{ return fHasValue; }

// ==================================================

ZCmdLine::String::String(const char* inName, const char* inDescription, const char* inDefaultValue, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, inDefaultValue == nil, inEnvironmentVariable), fValue(inDefaultValue), fDefaultValue(inDefaultValue)
	{}

ZCmdLine::String::String(const char* inName, const char* inDescription, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, true, inEnvironmentVariable), fValue(nil)
	{}

ZCmdLine::String::String(const char* inName, const char* inDescription, const char* inDefaultValue, bool inIsArgument, bool inIsRequired, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, inIsRequired, inEnvironmentVariable), fValue(inDefaultValue), fDefaultValue(inDefaultValue)
	{}

void ZCmdLine::String::SetValueFromLexeme(const char* inLexeme)
	{
	fValue = inLexeme;
	fHasValue = true;
	fIsUsed = true;
	}

void ZCmdLine::String::SetupFromEnvironmentVariable()
	{
	if (fEnvironmentVariable)
		{
		const char* theEnv = ::getenv(fEnvironmentVariable);
		if (theEnv)
			{
			fValue = theEnv;
			fHasValue = true;
			}
		}
	}

const char* ZCmdLine::String::GetType()
	{ return "<string>"; }

void ZCmdLine::String::PrintValue(ostream& str) const
	{
	if (fValue)
		str << fValue;
	else
		str << "nil";
	}

void ZCmdLine::String::PrintDefaultValue(ostream& str) const
	{
	if (fDefaultValue)
		str << fDefaultValue;
	else
		str << "nil";
	}

void ZCmdLine::String::SetValue(const char* c)
	{ fValue = c; }

const char* ZCmdLine::String::operator()()
	{ return fValue; }

// ==================================================

ZCmdLine::Char::Char(const char* inName, const char* inDescription, char inDefaultValue, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, false, inEnvironmentVariable), fValue(inDefaultValue), fDefaultValue(inDefaultValue)
	{}

ZCmdLine::Char::Char(const char* inName, const char* inDescription, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, true, inEnvironmentVariable), fValue(' ')
	{}

void ZCmdLine::Char::SetValueFromLexeme(const char* inLexeme)
	{
	fValue = inLexeme[0];
	fHasValue = true;
	fIsUsed = true;
	}

void ZCmdLine::Char::SetupFromEnvironmentVariable()
	{
	if (fEnvironmentVariable)
		{
		const char* theEnv = ::getenv(fEnvironmentVariable);
		if (theEnv)
			{
			fValue = theEnv[0];
			fHasValue = true;
			}
		}
	}

const char* ZCmdLine::Char::GetType()
	{ return "<char>"; }

void ZCmdLine::Char::PrintValue(ostream& str) const
	{ str << fValue; }

void ZCmdLine::Char::PrintDefaultValue(ostream& str) const
	{ str << fDefaultValue; }

void ZCmdLine::Char::SetValue(char c)
	{ fValue = c; }

char ZCmdLine::Char::operator()()
	{ return fValue; }

// ==================================================

ZCmdLine::Integer::Integer(const char* inName, const char* inDescription, int inDefaultValue, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, false, inEnvironmentVariable), fValue(inDefaultValue), fDefaultValue(inDefaultValue)
	{}

ZCmdLine::Integer::Integer(const char* inName, const char* inDescription, bool inIsArgument, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, inIsArgument, true, inEnvironmentVariable), fValue(0)
	{}

void ZCmdLine::Integer::SetValueFromLexeme(const char* inLexeme)
	{
	fValue = ::atoi(inLexeme);
	fHasValue = true;
	fIsUsed = true;
	}

void ZCmdLine::Integer::SetupFromEnvironmentVariable()
	{
	if (fEnvironmentVariable)
		{
		const char* theEnv = ::getenv(fEnvironmentVariable);
		if (theEnv)
			{
			fValue = ::atoi(theEnv);
			fHasValue = true;
			}
		}
	}

const char* ZCmdLine::Integer::GetType()
	{ return "<integer>"; }

void ZCmdLine::Integer::PrintValue(ostream& str) const
	{ str << fValue; }

void ZCmdLine::Integer::PrintDefaultValue(ostream& str) const
	{ str << fDefaultValue; }

void ZCmdLine::Integer::SetValue(int c)
	{ fValue = c; }

int ZCmdLine::Integer::operator()()
	{ return fValue; }

// ==================================================

ZCmdLine::StringMulti::StringMulti(const char* inName, const char* inDescription, bool inIsArgument)
:	Argument(inName, inDescription, inIsArgument, false, nil)
	{}

void ZCmdLine::StringMulti::SetValueFromLexeme(const char* inLexeme)
	{
	fValues.push_back(inLexeme);
	fHasValue = true;
	}

void ZCmdLine::StringMulti::SetupFromEnvironmentVariable()
	{}

const char* ZCmdLine::StringMulti::GetType()
	{ return "<string>..."; }

void ZCmdLine::StringMulti::PrintValue(ostream& str) const
	{
	for (size_t i = 0; i < fValues.size(); ++i)
		{
		if (i >= 1)
			str << ", ";
		str << fValues[i];
		}
	}

void ZCmdLine::StringMulti::PrintDefaultValue(ostream& str) const
	{}

const char* ZCmdLine::StringMulti::	operator[](size_t i)
	{
	if (i < fValues.size())
		return fValues[i];
	return nil;
	}

size_t ZCmdLine::StringMulti::Size()
	{ return fValues.size(); }

// ===================================================

ZCmdLine::Toggle::Toggle(const char* inName, const char* inDescription, bool inDefaultValue, const char* inEnvironmentVariable)
:	Argument(inName, inDescription, false, false, inEnvironmentVariable), fValue(inDefaultValue), fDefaultValue(inDefaultValue)
	{}

bool ZCmdLine::Toggle::HasLength() const
	{ return false; }

void ZCmdLine::Toggle::SetValueFromLexeme(const char* inLexeme)
	{
	fValue = !fValue;
	fHasValue = true;
	}

void ZCmdLine::Toggle::SetupFromEnvironmentVariable()
	{
	if (fEnvironmentVariable)
		{
		const char* theEnv = ::getenv(fEnvironmentVariable);
		if (theEnv)
			{
			fValue = !fValue;
			fHasValue = true;
			}
		}
	}

const char* ZCmdLine::Toggle::GetType()
	{ return ""; }

void ZCmdLine::Toggle::PrintValue(ostream& str) const
	{
	if (fValue)
		str << "on";
	else
		str << "off";
	}

void ZCmdLine::Toggle::PrintDefaultValue(ostream& str) const
	{
	if (fDefaultValue)
		str << "on";
	else
		str << "off";
	}

void ZCmdLine::Toggle::SetValue(bool c)
	{ fValue = c; }

bool ZCmdLine::Toggle::operator()()
	{ return fValue; }

// ==================================================

ostream& operator<<(ostream& str, const ZCmdLine::Argument& a)
	{
	a.PrintValue(str);
	return str;
	}

// =================================================================================================
