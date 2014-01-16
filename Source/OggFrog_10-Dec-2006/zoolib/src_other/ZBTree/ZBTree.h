/*  @(#) $Id: ZBTree.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZBTree__
#define __ZBTree__
#include "zconfig.h"

#include "ZRefCount.h"
#include "ZDebug.h"
#include <vector>

/* All the classes in here, except for ZBTree::Item, *must* be created appropriately.
To ensure this, we make private the default & copy constructors, and operator=. To
Make absolutely sure no-one tries to do anything other than create these classes
through the correct constructor, we include an inline definition for them that calls
ZUnimplemented(), which drops out in non-debug mode, but will drop into the
debugger when debugging.
To use this implementation of B-Trees, you must subclass ZBTree::Item and
ZBTree::NodeSpace (they include pure virtual functions, so you've got no choice
but to create your own subclasses.) */

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree

class ZBTree
	{
private:
	ZBTree(); // Not implemented
	ZBTree(const ZBTree&); // Not implemented
	ZBTree& operator=(const ZBTree& other); // Not implemented

public:
	typedef unsigned long Handle;
	enum { kInvalidHandle = 0 };

	class NodeSpace;
	class Node;
	class Item;
	class NodeWalker;

	ZBTree(ZBTree::NodeSpace* inNodeSpace);
	~ZBTree() {};

	long GetDegree();

	void Insert(ZBTree::Item* inItem);
	bool Remove(const ZBTree::Item* inSearchItem);
	long GetTreeSize();

	ptrdiff_t MoveBy(ptrdiff_t inOffset, std::vector<std::pair<ZBTree::Handle, long> >& ioStack);
	size_t CalculateRank(const std::vector<std::pair<ZBTree::Handle, long> >& inStack);

	void WalkNodes(NodeWalker& inWalker);

private:
	void Internal_WalkNodes(NodeWalker& inWalker, const ZRef<ZBTree::Node>& inNode);

	enum EAction { eActionNone, eActionRotateLeft, eActionRotateRight, eActionMerge };
	ZRef<ZBTree::Node> DescendInto(ZRef<ZBTree::Node> inCurrentNode, long inSubtreeIndex, EAction& outAction);
	ZRef<ZBTree::Node> Adjust(ZRef<ZBTree::Node> inCurrentNode, long inIndex, ZRef<ZBTree::Node> inLeftNode, ZRef<ZBTree::Node> inRightNode, EAction& outAction);
	void InsertNonFull(ZRef<ZBTree::Node> inNode, ZBTree::Item* inItem);
	ZRef<ZBTree::Node> SplitChild(ZRef<ZBTree::Node> inParentNode, long inFullChildPosition, ZRef<ZBTree::Node> inFullChildNode);

	ZBTree::NodeSpace* fNodeSpace;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::Item

class ZBTree::Item
	{
private:
	ZBTree::Item& operator=(const ZBTree::Item&); // Not implemented
	Item(const ZBTree::Item&); // Not implemented

public:
	Item() {}
	virtual ~Item() {}

// Return -1 if this < otherItem, 0 if this == otherItem, 1 if this > otherItem
	virtual long Compare(const ZBTree::Item* inOther) = 0;
	virtual long CompareForSearch(const ZBTree::Item* inOther, bool inStrictComparison);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::NodeSpace

class ZBTree::NodeSpace
	{
private:
	NodeSpace(); // Not implemented
	NodeSpace(const ZBTree::NodeSpace&); // Not implemented
	ZBTree::NodeSpace& operator=(const ZBTree::NodeSpace&); // Not implemented

public:
	NodeSpace(long inDegree);
	NodeSpace(long inDegree, ZBTree::Handle inRootHandle);
	virtual ~NodeSpace();

	virtual ZRef<ZBTree::Node> LoadNode(ZBTree::Handle inHandle) = 0;
	virtual long GetNodeSubTreeSize(ZBTree::Handle inHandle);
	virtual ZRef<ZBTree::Node> CreateNode() = 0;
	virtual void DeleteNode(ZRef<ZBTree::Node> inNode) = 0;
	virtual void NodeBeingFinalized(ZBTree::Node* inNode) = 0;
	virtual void SetNewRoot(ZBTree::Handle inHandle);

	virtual void Create();

	long GetOrder() const { return 2 * fDegree - 1; }
	long GetDegree() const { return fDegree; }

	ZBTree::Handle GetRootHandle() const { return fRootNodeHandle; }

private:
	long fDegree;
	ZBTree::Handle fRootNodeHandle;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::NodeWalker

class ZBTree::Node : public ZRefCountedWithFinalization
	{
	Node(); // Not implemented
	Node(const ZBTree::Node& other); // Not implemented
	ZBTree::Node& operator=(const ZBTree::Node& other); // Not implemented

public:
	Node(ZBTree::NodeSpace* inNodeSpace, ZBTree::Handle inHandle);
	virtual ~Node();

	ZBTree::NodeSpace* GetNodeSpace() const { return fNodeSpace; }
	ZBTree::Handle GetHandle() const { return fHandle; }
	bool GetIsLeaf() const { return fIsLeaf; }
	long GetItemCount() const { return fItemCount; }
	long GetSubTreeSize() const { return fSubTreeSize; }
	ZBTree::Item* GetItemAt(long inIndex) const { return fItems[inIndex]; }
	ZBTree::Handle GetHandleAt(long inIndex) const	{ return fSubTreeHandles[inIndex]; }

// This should only be called by a nodespace that is marking the node as deleted.
// Deletion of the in-memory node itself will take place when all references go out of scope
	void SetHandle(ZBTree::Handle inHandle) { fHandle = inHandle; }

	virtual void Finalize();
	virtual void ContentChanged();

	long FindPosition(const ZBTree::Item* inComparitor, bool& outExactFind);
	long FindPosition_LowestEqualOrGreater(const ZBTree::Item* inComparitor, long& outResult);
	long FindPosition_LowestGreater(const ZBTree::Item* inComparitor, long& outResult);

private:
	void ShiftRightAt(long inIndex);
	void ShiftLeftAt(long inIndex);
	void MoveSubNode(ZRef<ZBTree::Node> inOther, long inOtherPos, long inOurPos, long inKeys);

protected:
	ZBTree::NodeSpace* fNodeSpace;
	ZBTree::Handle fHandle;

	bool fIsLeaf;
	long fItemCount;
	ZBTree::Item** fItems;
	ZBTree::Handle* fSubTreeHandles;
	long fSubTreeSize;

	friend class ZBTree;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::NodeWalker

class ZBTree::NodeWalker
	{
public:
	virtual void NodeEntered(const ZRef<ZBTree::Node>& inNode) = 0;
	virtual void DumpItem(ZBTree::Item* inItem) = 0;
	virtual void NodeExited(const ZRef<ZBTree::Node>& inNode) = 0;
	};

// =================================================================================================
#endif // __ZBTree__
