/*  @(#) $Id: ZNode.h,v 1.6 2006/07/23 22:00:42 agreen Exp $ */

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

#ifndef __ZNode__
#define __ZNode__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZRefCount.h"
#include "ZStreamer.h"
#include "ZTrail.h"
#include "ZTuple.h"

#include <set>
#include <string>

class ZNode;
class ZNodeRep;
class ZNodeIterRep;

// =================================================================================================
#pragma mark -
#pragma mark * ZNode

class ZNode
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZNode,
    	operator_bool_generator_type, operator_bool_type);

public:
	enum Error
		{
		errorNone,
		errorGeneric,
		errorDoesntExist,
		errorAlreadyExists,
		errorInvalidNode,
		errorIllegalName,
		errorNoPermission,
		errorWrongTypeForOperation,
		errorReadPastEOF,
		errorInsufficientSpace
		};

	ZNode();
	ZNode(const ZNode& iOther);
	ZNode(const ZRef<ZNodeRep>& iRep);
	ZNode(const ZRef<ZNodeRep>& iNodeRep, const std::string& iComp);
	ZNode(ZNodeRep* iNodeRep);

	template <class I>
	ZNode(const ZRef<ZNodeRep>& iNodeRep, const I& iBegin, const I& iEnd)
	:	fNodeRep(iNodeRep),
		fComps(iBegin, iEnd)
		{}

	template <class I>
	ZNode(const ZRef<ZNodeRep>& iNodeRep, const I& iBegin, const I& iEnd,
		const std::string& iAdditional)
	:	fNodeRep(iNodeRep),
		fComps(iBegin, iEnd)
		{ fComps.push_back(iAdditional); }

	~ZNode();

	ZNode& operator=(const ZNode& iOther);
	ZNode& operator=(const ZRef<ZNodeRep>& iRep);

	bool operator==(const ZNode& iOther) const;
	bool operator<(const ZNode& iOther) const;

	operator operator_bool_type() const;

	bool GetProp(const std::string& iPropName, ZTupleValue& oTV, Error* oError = 0) const;

	bool CanHaveChildren(Error* oError = 0) const;

	std::string Name(Error* oError = 0) const;
	ZNode Ancestor(size_t iCount, Error* oError = 0) const;
	ZNode Parent(Error* oError = 0) const;
	ZNode Child(const std::string& iName, Error* oError = 0) const;
	ZNode Sibling(const std::string& iName, Error* oError = 0) const;
	ZNode Trail(const ZTrail& iTrail, Error* oError = 0) const;
	ZNode Descendant(const std::string* iComps, size_t iCount, Error* oError = 0) const;

	ZTrail TrailTo(const ZNode& iDest, Error* oError = 0) const;
	ZTrail TrailFrom(const ZNode& iSource, Error* oError = 0) const;
	ZTrail TrailFromRoot(Error* oError = 0) const;

	bool Exists(Error* oError = 0) const;

	ZNode CreateDir(Error* oError = 0) const;

	ZNode MoveTo(const ZNode& iDest, Error* oError = 0) const;
	ZNode MoveFrom(const ZNode& iSource, Error* oError = 0) const;

	bool Delete(Error* oError = 0) const;

	ZRef<ZStreamerR> OpenR(bool iPreventWriters = false, Error* oError = 0) const;
	ZRef<ZStreamerRPos> OpenRPos(bool iPreventWriters = false, Error* oError = 0) const;
	ZRef<ZStreamerW> OpenW(bool iPreventWriters = true, Error* oError = 0) const;
	ZRef<ZStreamerWPos> OpenWPos(bool iPreventWriters = true, Error* oError = 0) const;
	ZRef<ZStreamerRWPos> OpenRWPos(bool iPreventWriters = true, Error* oError = 0) const;

	ZRef<ZStreamerWPos> CreateWPos(bool iOpenExisting,
		bool iPreventWriters = true, Error* oError = 0) const;

	ZRef<ZStreamerRWPos> CreateRWPos(bool iOpenExisting,
		bool iPreventWriters = true, Error* oError = 0) const;
	
	ZRef<ZNodeRep> GetNodeRep() const;

protected:
	ZRef<ZNodeRep> pPhysicalNodeRep() const;

	ZRef<ZNodeRep> fNodeRep;
	std::vector<std::string> fComps;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeIter

class ZNodeIter
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZNodeIter,
    	operator_bool_generator_type, operator_bool_type);
public:
	ZNodeIter();
	ZNodeIter(const ZNodeIter& iOther);
	ZNodeIter(const ZNode& iAsset);
	~ZNodeIter();
	ZNodeIter& operator=(const ZNodeIter& iOther);

	operator operator_bool_type() const;
	ZNodeIter& Advance();
	ZNode Current() const;
	std::string CurrentName() const;

private:
	ZRef<ZNodeIterRep> fRep;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeTreeIter

/// An iterator that provides access to every node descending from a ZNode.

class ZNodeTreeIter
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZNodeTreeIter,
    	operator_bool_generator_type, operator_bool_type);
public:
	ZNodeTreeIter();
	ZNodeTreeIter(const ZNodeTreeIter& iOther);
	ZNodeTreeIter(const ZNode& iRoot);
	~ZNodeTreeIter();
	ZNodeTreeIter& operator=(const ZNodeTreeIter& iOther);

	operator operator_bool_type() const;
	ZNodeTreeIter& Advance();
	ZNode Current() const;
	std::string CurrentName() const;

private:
	ZNode fCurrent;
	std::vector<ZNodeIter> fStack;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeRep

class ZNodeRep : public ZRefCountedWithFinalization
	{
protected:
	ZNodeRep();

public:
	typedef ZNode::Error Error;

	virtual ~ZNodeRep();

	virtual ZRef<ZNodeIterRep> CreateIterRep();

	virtual bool GetProp(const std::string& iPropName, ZTupleValue& oTV, Error* oError);
	virtual bool CanHaveChildren(Error* oError);

	virtual std::string GetName(Error* oError) = 0;
	virtual ZTrail TrailTo(ZRef<ZNodeRep> iDest, Error* oError) = 0;

	virtual ZRef<ZNodeRep> GetAncestor(size_t iCount, Error* oError) = 0;
	virtual ZRef<ZNodeRep> GetDescendant(const std::string* iComps,
		size_t iCount, Error* oError) = 0;

	virtual bool Exists(Error* oError) = 0;

	virtual ZRef<ZNodeRep> CreateDir(Error* oError);

	virtual ZRef<ZNodeRep> MoveTo(ZRef<ZNodeRep> iDest, Error* oError);
	virtual ZRef<ZNodeRep> MoveFrom(ZRef<ZNodeRep> iSource, Error* oError);

	virtual bool Delete(Error* oError);

	virtual ZRef<ZStreamerR> OpenR(bool iPreventWriters, Error* oError);
	virtual ZRef<ZStreamerRPos> OpenRPos(bool iPreventWriters, Error* oError);
	virtual ZRef<ZStreamerW> OpenW(bool iPreventWriters, Error* oError);
	virtual ZRef<ZStreamerWPos> OpenWPos(bool iPreventWriters, Error* oError);
	virtual ZRef<ZStreamerRWPos> OpenRWPos(bool iPreventWriters, Error* oError);

	virtual ZRef<ZStreamerWPos> CreateWPos(bool iOpenExisting,
		bool iPreventWriters, Error* oError);

	virtual ZRef<ZStreamerRWPos> CreateRWPos(bool iOpenExisting,
		bool iPreventWriters, Error* oError);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeIterRep

class ZNodeIterRep : public ZRefCountedWithFinalization
	{
protected:
	ZNodeIterRep();

public:
	virtual ~ZNodeIterRep();

	virtual bool HasValue() = 0;
	virtual void Advance() = 0;
	virtual ZRef<ZNodeRep> Current() = 0;
	virtual std::string CurrentName() = 0;

	virtual ZRef<ZNodeIterRep> Clone() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeIterRep_Std

class ZNodeIterRep_Std : public ZNodeIterRep
	{
public:
	class RealRep;

	ZNodeIterRep_Std(ZRef<RealRep> iRealRep, size_t iIndex);
	virtual ~ZNodeIterRep_Std();

// From ZNodeIterRep
	virtual bool HasValue();
	virtual void Advance();
	virtual ZRef<ZNodeRep> Current();
	virtual std::string CurrentName();

	virtual ZRef<ZNodeIterRep> Clone();

private:
	ZRef<RealRep> fRealRep;
	size_t fIndex;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNodeIterRep_Std::RealRep

class ZNodeIterRep_Std::RealRep : public ZRefCountedWithFinalization
	{
protected:
	RealRep() {};

public:
	virtual bool HasValue(size_t iIndex) = 0;
	virtual ZRef<ZNodeRep> GetNodeRep(size_t iIndex) = 0;
	virtual std::string GetName(size_t iIndex) = 0;
	};

#endif // __ZNode__
