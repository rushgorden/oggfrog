/*  @(#) $Id: ZStrimW_ML.h,v 1.21 2006/04/27 03:19:32 agreen Exp $ */

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

#ifndef __ZStrimW_ML__
#define __ZStrimW_ML__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZDebug.h"
#include "ZStrim.h"
#include "ZStrimmer.h"

#include <vector>

class ZTuple;
class ZTupleValue;

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_ML

/// A write filter strim to help generate well-formed ML-type data (XML, HTML etc).

class ZStrimW_ML : public ZStrimW_NativeUTF8, ZooLib::NonCopyable
	{
public:
	// The copy constructor is deliberately not implemented. See docs for reason.
	ZStrimW_ML(const ZStrimW_ML&);

	explicit ZStrimW_ML(const ZStrimW& iStrimSink);

	ZStrimW_ML(bool iIndent, const ZStrimW& iStrimSink);
	ZStrimW_ML(const string8& iEOL, const string8& iIndent, const ZStrimW& iStrimSink);
	~ZStrimW_ML();

// From ZStrimW
	virtual void Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU);

	virtual void Imp_Flush();

// Our protocol

	/// Close off any pending tag and return the sink stream.
	const ZStrimW& Raw() const;

	/// Write an nbsp entity.
	void WriteNBSP() const;
	void NBSP() const { this->WriteNBSP(); }

	/// Write an arbitrary entity.
	void WriteEntity(const string8& iEntity) const;
	void WriteEntity(const UTF8* iEntity) const;

	/// Add a normal begin tag to the top of the stack.
	void Begin(const string8& iTag) const;

	/** \brief Check that the tag on the top of the stack matches iTag,
	and emit the end tag in form </XXX>. */
	void End(const string8& iTag) const;

	/** \brief If the stack is empty trip an error. Otherwise emit
	the tag on the top of the stack in the form </XXX>. */
	void End() const;

	/// Write a tag of the form <XXX a1="xxx" a2="xxx"/>.
	void Empty(const string8& iTag) const;

	/// Write a tag of the form <?XXX a1="xxx" a2="xxx"?>.
	void PI(const string8& iTag) const;

	/** \brief Write a tag of the form <XXX a1="xxx" a2="xxx">, without
	requiring that an end tag be subsequently written. */
	void Tag(const string8& iTag) const;

	/// Add a boolean attribute (one with no value) to the currently pending tag.
	void Attr(const string8& iValue) const;

	/// Add an attribute with the name \a iName and the value \a iValue to the pending tag.
	void Attr(const string8& iName, const string8& iValue) const;

	/// Add an attribute with the name \a iName and the value \a iValue to the pending tag.
	void Attr(const string8& iName, const UTF8* iValue) const;

	/** Add an attribute named \a iName to the currently pending tag. The value
	will be a string containing the base 10 digits of the integer \a iValue. */
	void Attr(const string8& iName, int iValue) const;

	/** Add an attribute with the name \a iName and the printf-formatted
	string iValue etc to the currently pending tag. */
	void Attrf(const string8& iName, const UTF8* iValue, ...) const;

	/** Add an attribute named \a iName to the currently pending tag. The value
	will be a string containing a sensible textual version of \a iValue. */
	void Attr(const string8& iName, const ZTupleValue& iValue) const;

	/** Add attributes to the currently pending tag, taking the names and values from
	properties of iTuple. String values are added as you would expect, null values
	are added as boolean attributes. This convention is compatible with that
	used by ZML::Reader. */
	void Attrs(const ZTuple& iTuple) const;

	/// Set indent enable, and return previous value.
	bool Indent(bool iIndent) const;

	/// Toss any pending tag and its attributes, and clear the stack.
	void Abandon();

	/// Close off any pending tag, emit end tags for everything on the stack and empty the stack.
	void EndAll();

	/// Close off all tags until \a iTag is reached in the stack.
	void EndAll(const string8& iTag);

	/** If Begin, Empty, PI or Tag has been called, and no call to Write or End
	has been made, then the tag has not actually been written out as we're
	still allowing attributes to be added. Write any such pending tag, and
	switch to the same state we'd be if we'd called Write. */
	void WritePending();

protected:
	enum ETagType
		{ eTagTypeNone, eTagTypeNormal, eTagTypeEmpty, eTagTypePI, eTagTypeNoEnd };

	void pBegin(const string8& iTag, ETagType iTagType);
	void pWritePending();
	void pAttr(const string8& iName, string8* iValue);
	void pEnd();

	const ZStrimW& fStrimSink;
	
	ETagType fTagType;
	bool fWrittenSinceLastTag;
	bool fLastWasEOL;
	bool fIndentEnabled;
	string8 fString_EOL;
	string8 fString_Indent;
	std::vector<string8> fTags;
	std::vector<string8> fAttributeNames;
	std::vector<string8*> fAttributeValues;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimmerW_ML

/// A write filter strimmer encapsulating a ZStrimW_ML.

class ZStrimmerW_ML : public ZStrimmerW
	{
public:
	ZStrimmerW_ML(ZRef<ZStrimmerW> iStrimmer);
	ZStrimmerW_ML(bool iIndent, ZRef<ZStrimmerW> iStrimmer);
	ZStrimmerW_ML(const string8& iEOL, const string8& iIndent, ZRef<ZStrimmerW> iStrimmer);
	virtual ~ZStrimmerW_ML();

// From ZStrimmerW
	virtual const ZStrimW& GetStrimW();

protected:
	ZRef<ZStrimmerW> fStrimmer;
	ZStrimW_ML fStrim;
	};

#endif // __ZStrimW_ML__
