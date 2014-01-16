/*  @(#) $Id: ZCmdLine.h,v 1.5 2004/05/05 15:52:28 agreen Exp $ */

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

#ifndef __ZCmdLine__
#define __ZCmdLine__ 1
#include "zconfig.h"

#include "ZDebug.h"

#include <cstdlib>
#include <string>
#include <vector>

class ZCmdLine
	{
public:
// Forward declaration of the argument base class
	class Argument;
	class Char;
	class Toggle;
	class Integer;
	class IntegerMulti;
	class String;
	class StringMulti;

	ZCmdLine(bool ignoreUnknownOptions = false, bool ignoreUnknownArgs = false);
	const char* GetAppName() const;

	bool Parse(int argc, char* argv[]);
	void PrintUsage(std::ostream& theStream);
protected:
	void PrintUsageAndExit(const std::string& reasonString, int exitCode = -1);

	void AddArgument(Argument* arg);

	bool Internal_Parse(int argc, char** argv);
	bool Internal_ParseArgument(const char* argPtr);
	bool Internal_ParseFlags(int& argc, char**& argv);

// Data members
	static ZCmdLine* sCurrentCmdLine;
	std::vector<Argument*> fArguments;

	char* fAppName;
	bool fIgnoreUnknownOptions;
	bool fIgnoreUnknownArgs;
	friend class ZCmdLine::Argument;
	};

// ==================================================

class ZCmdLine::Argument
	{
public:
	Argument(const char* inName, const char* inDescription, bool inIsArgument, bool inIsRequired, const char* inEnvironmentVariable);

	void PrintUsage(std::ostream& str);
	void PrintUsageExtended(std::ostream& str);

	bool IsRequired() const;
	bool IsArgument() const;
	const char* GetName() const;

	virtual bool HasLength() const;

// Subclasses must redefine these
	virtual void SetValueFromLexeme(const char* inLexeme) = 0;
	virtual void SetupFromEnvironmentVariable() = 0;
	virtual const char* GetType() = 0;
	virtual void PrintValue(std::ostream&) const = 0;
	virtual void PrintDefaultValue(std::ostream& str) const = 0;
	bool HasValue() const;

protected:
	bool IsUsed() const;

	const char* fName;
	const char* fDescription;
	const char* fEnvironmentVariable;

	bool fIsArgument;
	bool fIsRequired;

	bool fIsUsed;
	bool fHasValue;
	friend class ZCmdLine;
	};

// ----------

class ZCmdLine::String : public ZCmdLine::Argument
	{
public:
	String(const char* inName, const char* inDescription, const char* inDefaultValue, bool inIsArgument = false, const char* inEnvironmentVariable = nil);
	String(const char* inName, const char* inDescription, bool inIsArgument = false, const char* inEnvironmentVariable = nil);
	String(const char* inName, const char* inDescription, const char* inDefaultValue, bool inIsArgument = false, bool inIsRequired = false, const char* inEnvironmentVariable = nil);

	virtual void SetValueFromLexeme(const char* inLexeme);
	virtual void SetupFromEnvironmentVariable();
	virtual const char* GetType();
	virtual void PrintValue(std::ostream& str) const;
	virtual void PrintDefaultValue(std::ostream& str) const;

	void SetValue(const char* c);
	const char* operator()();
protected:
	const char* fValue;
	const char* fDefaultValue;
	};

// ----------

class ZCmdLine::StringMulti : public ZCmdLine::Argument
	{
public:
	StringMulti(const char* inName, const char* inDescription, bool inIsArgument = false);

	virtual void SetValueFromLexeme(const char* inLexeme);
	virtual void SetupFromEnvironmentVariable();
	virtual const char* GetType();
	virtual void PrintValue(std::ostream& str) const;
	virtual void PrintDefaultValue(std::ostream& str) const;

	const char* operator[](size_t i);
	size_t Size();
protected:
	const char* fValue;
	std::vector<const char*>fValues;
	};

// ----------

class ZCmdLine::Char : public ZCmdLine::Argument
	{
public:
	Char(const char* inName, const char* inDescription, char inDefaultValue, bool inIsArgument = false, const char* inEnvironmentVariable = nil);
	Char(const char* inName, const char* inDescription, bool inIsArgument = false, const char* inEnvironmentVariable = nil);

	virtual void SetValueFromLexeme(const char* inLexeme);
	virtual void SetupFromEnvironmentVariable();
	virtual const char* GetType();
	virtual void PrintValue(std::ostream& str) const;
	virtual void PrintDefaultValue(std::ostream& str) const;

	void SetValue(char c);
	char operator()();
protected:
	char fValue; 
	char fDefaultValue;
	};

// ----------

class ZCmdLine::Integer : public ZCmdLine::Argument
	{
public:
	Integer(const char* inName, const char* inDescription, int inDefaultValue, bool inIsArgument = false, const char* inEnvironmentVariable = nil);
	Integer(const char* inName, const char* inDescription, bool inIsArgument = false, const char* inEnvironmentVariable = nil);

	virtual void SetValueFromLexeme(const char* inLexeme);
	virtual void SetupFromEnvironmentVariable();
	virtual const char* GetType();
	virtual void PrintValue(std::ostream& str) const;
	virtual void PrintDefaultValue(std::ostream& str) const;

	void SetValue(int c);
	int operator()();

protected:
	int fValue;
	int fDefaultValue;
	};

// ----------

class ZCmdLine::Toggle : public ZCmdLine::Argument
	{
public:
// This cannot be a required parameter -- if present then toggled from the default value to its inverse.
	Toggle(const char* inName, const char* inDescription, bool inDefaultValue, const char* inEnvironmentVariable = nil);

	virtual bool HasLength() const;
	virtual void SetValueFromLexeme(const char* inLexeme);
	virtual void SetupFromEnvironmentVariable();
	virtual const char* GetType();
	virtual void PrintValue(std::ostream& str) const;
	virtual void PrintDefaultValue(std::ostream& str) const;

	void SetValue(bool c);
	bool operator()();
protected:
	bool fValue;
	bool fDefaultValue;
	};

// ----------

std::ostream& operator<<(std::ostream&, const ZCmdLine::Argument&);

#endif // __ZCmdLine__
