static const char rcsid[] = "@(#) $Id: ZBTree.cpp,v 1.6 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZBTree.h"
#include <stdexcept>
#include <string> // range_error may need this

using std::make_pair;
using std::pair;
using std::range_error;
using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree

ZBTree::ZBTree(ZBTree::NodeSpace* theNodeSpace)
:	fNodeSpace(theNodeSpace)
	{}

long ZBTree::GetDegree()
	{ return fNodeSpace->GetDegree(); }

void ZBTree::Insert(ZBTree::Item* inItem)
	{
	ZRef<ZBTree::Node> rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle());
// Is the root node full? Remember the degree is (order+1)/2
	if (rootNode->fItemCount < fNodeSpace->GetOrder())
		{
// NO! Then we can just do an insertNonFull on the root
		this->InsertNonFull(rootNode, inItem);
		return;
		}

// The root is full. So we have to split the root, and create a new root
	ZRef<ZBTree::Node> newRoot = fNodeSpace->CreateNode();
	ZBTree::Handle newRootHandle = newRoot->GetHandle();
	newRoot->fSubTreeHandles[0] = rootNode->GetHandle();
	newRoot->fIsLeaf = false;
	newRoot->fSubTreeSize = rootNode->fSubTreeSize;
	newRoot->ContentChanged();
// Split the rootNode, so half its stuff will end up in new root, and half in
// the new sibling that SplitChild creates
	this->SplitChild(newRoot, 0, rootNode);
// Make sure the nodespace knows we have a new root node
	fNodeSpace->SetNewRoot(newRootHandle);
// *Now* we can do an insert on our newly non-full root
	this->InsertNonFull(newRoot, inItem);
	}

bool ZBTree::Remove(const ZBTree::Item* inSearchItem)
	{
// Start at the root node
	ZRef<ZBTree::Node> rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle());
	ZRef<ZBTree::Node> currentNode = rootNode;

// Handle the two simple cases first -- an empty tree
	if (currentNode->fItemCount == 0)
		return false;
// and a tree with one item
	if (currentNode->fItemCount == 1 && currentNode->fIsLeaf)
		{
		bool isExact = false;
// We only delete the item in the tree if it is an exact match for our search item
		currentNode->FindPosition(inSearchItem, isExact);
		if (isExact)
			{
			ZBTree::Item* theItem = currentNode->fItems[0];
			currentNode->fItems[0] = nil;
			currentNode->fItemCount = 0;
			currentNode->fSubTreeSize = 0;
			currentNode->ContentChanged();
			delete theItem;
			return true;
			}
		else
			return false;
		}

// Stack for updating sub tree sizes on path from deleted node (if it exists)
// to the root
	vector<ZRef<ZBTree::Node> > stack;

// Now we get funky!
	short index = 0;
	bool found = false;
	ZRef<ZBTree::Node> nodeDescendedInto;
	enum { SEARCHING, DESCENDING } state = SEARCHING;
	while (true)
		{
		if (state == SEARCHING)
			index = currentNode->FindPosition(inSearchItem, found);
		EAction theAction;
		nodeDescendedInto = this->DescendInto(currentNode, index + 1, theAction);
		if (currentNode == rootNode && currentNode->fItemCount == 0)
			fNodeSpace->DeleteNode(currentNode);
		else
			stack.push_back(currentNode);

// If we've run out of tree, then bail
		if (!nodeDescendedInto)
			break;
		if (found)
			{
			state = DESCENDING;
			if (theAction == eActionRotateRight)
				index = 0;
			else if (theAction == eActionRotateLeft || theAction == eActionMerge)
				index = fNodeSpace->GetDegree() - 1;
			else
				break;
			}
		currentNode = nodeDescendedInto;
		}

	if (!found)
		return false;

	if (currentNode->fIsLeaf)
		{
		ZBTree::Item* itemToDelete = currentNode->fItems[index];
		currentNode->ShiftLeftAt(index + 1);
		delete itemToDelete;
		}
	else
		{
// The key is in an internal node, so we'll replace it by its in order successor
// We just walk down the the left most branch of all our children, and move
// the leftmost item from the node at the leaf of this branch into our current node
		ZRef<ZBTree::Node> aNode = nodeDescendedInto;
		while (true)
			{
			stack.push_back(aNode);
			if (aNode->fIsLeaf)
				break;
			EAction dummyAction;
			aNode = this->DescendInto(aNode, 0, dummyAction);
			}

		ZBTree::Item* itemToDelete = currentNode->fItems[index];
		currentNode->fItems[index] = aNode->fItems[0];
		aNode->ShiftLeftAt(1);
		currentNode->ContentChanged();
		delete itemToDelete;
		}
// Finally, update the sub tree sizes along the search path
	for (vector<ZRef<ZBTree::Node> >::iterator x = stack.begin(); x != stack.end(); ++x)
		{
		ZAssertStop(2, (*x)->fSubTreeSize > 0);
		--(*x)->fSubTreeSize;
		(*x)->ContentChanged();
		}
	return true;
	}

long ZBTree::GetTreeSize()
	{
	ZRef<ZBTree::Node>	rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle());
	return rootNode->fSubTreeSize;
	}

ptrdiff_t ZBTree::MoveBy(ptrdiff_t inOffset, vector<pair<ZBTree::Handle, long> >& ioStack)
	{
// This method handles all the relative motion stuff for ZBTree derived iterators
	if (inOffset == 0)
		return 0;

	vector<pair<ZBTree::Handle, long> > tempStack = ioStack;
	if (inOffset < 0)
		{
// Check to see if we're at the end of the tree, if so then build a stack
// that points off the end of the last element
		if (tempStack.size() == 0)
			{
// Walk down the tree to the extreme rightmost node and item
			ZRef<ZBTree::Node> rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle());
			if (!rootNode)
				throw range_error("ZBTree::MoveBy, no root node");

			ZRef<ZBTree::Node> currentNode = rootNode;
			while (true)
				{
				tempStack.push_back(make_pair(currentNode->GetHandle(), currentNode->GetItemCount()));
				if (currentNode->GetIsLeaf())
					break;
		// Go down the rightmost branch
				currentNode = fNodeSpace->LoadNode(currentNode->GetHandleAt(currentNode->GetItemCount()));
				}
			}
// Now we can start walking backwards
		ptrdiff_t localOffset = -inOffset;
		while (tempStack.size() > 0)
			{
// If we've run off the beginning of the current node, then pop and loop again
			ZRef<ZBTree::Node> currentNode = fNodeSpace->LoadNode(tempStack.back().first);
			if (tempStack.back().second < 0)
				{
				tempStack.pop_back();
				--tempStack.back().second;
				continue;
				}
// Okay, we've got a valid stack. Now check to see if we've actually reached our target
			ZAssertStop(2, localOffset >= 0);
			if (localOffset == 0)
				break;

// We've got work to do
			if (currentNode->GetIsLeaf())
				{
				long spaceInNode = tempStack.back().second;
				if (spaceInNode >= localOffset)
					{
					tempStack.back().second -= localOffset;
					localOffset = 0;
					}
				else
					{
					localOffset -= spaceInNode;
					tempStack.pop_back();
					--tempStack.back().second;
					--localOffset;
					}
				}
			else
				{
				ZRef<ZBTree::Node> subNode = fNodeSpace->LoadNode(currentNode->GetHandleAt(tempStack.back().second));
				if (localOffset > subNode->GetSubTreeSize())
					{
// Account for the sub tree size
					localOffset -= subNode->GetSubTreeSize();
// Move to the previous element in the current node
					--tempStack.back().second;
// And for the fact that we're also pointing at the previous item in the current node
					--localOffset;
					}
				else
					tempStack.push_back(make_pair(subNode->GetHandle(), subNode->GetItemCount()));
				}
			}
// Check to see if we've moved off either end of the tree without reaching our target
		if (tempStack.size() == 0 && localOffset != 0)
			throw range_error("ZBTree::MoveBy, overflow");
		}
	else
		{
		ptrdiff_t localOffset = inOffset;
// We're moving forwards, make sure we're not off the end
		while (tempStack.size() > 0)
			{
			ZRef<ZBTree::Node> currentNode = fNodeSpace->LoadNode(tempStack.back().first);
// If we've run off the end of the current node, then pop and loop again
			if (tempStack.back().second >= currentNode->GetItemCount())
				{
				tempStack.pop_back();
				continue;
				}
// Okay, we've got a valid stack. Now check to see if we've actually reached our target
			ZAssertStop(2, localOffset >= 0);
			if (localOffset == 0)
				break;

// We've got work to do
			if (currentNode->GetIsLeaf())
				{
				long spaceInNode = currentNode->GetItemCount() - tempStack.back().second;

				if (spaceInNode > localOffset)
					{
					tempStack.back().second += localOffset;
					localOffset = 0;
					}
				else
					{
					localOffset -= spaceInNode;
					tempStack.pop_back();
					}
				}
			else
				{
				++tempStack.back().second;
				ZRef<ZBTree::Node> subNode = fNodeSpace->LoadNode(currentNode->GetHandleAt(tempStack.back().second));
				if (localOffset > subNode->GetSubTreeSize())
					{
// Account for the sub tree size
					localOffset -= subNode->GetSubTreeSize();
// And for the fact that we're also pointing at the next item in the current node
					--localOffset;
					}
				else
					{
// We need to descend into the subNode
					if (subNode->GetIsLeaf())
						{
// If it's a leaf, then we've actually moved on one entry in the sequence
						--localOffset;
						tempStack.push_back(make_pair(subNode->GetHandle(), 0L));
						}
					else
						tempStack.push_back(make_pair(subNode->GetHandle(), -1L));
					}
				}
			}
// Check to see if we've moved off either end of the tree without reaching our target
		if (tempStack.size() == 0 && localOffset != 0)
			throw range_error("ZBTree::MoveBy, overflow");
		}
	ioStack = tempStack;
	return inOffset;
	}

size_t ZBTree::CalculateRank(const vector<pair<ZBTree::Handle, long> >& inStack)
	{
	if (inStack.size() == 0)
		{
		if (ZRef<ZBTree::Node> rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle()))
			return rootNode->fSubTreeSize;
		return 0;
		}

// For the current node, we add the sub tree to the left (if any) plus our offset
	long stackIndex = inStack.size() - 1;
	size_t total = 0;
// Walk our stack, adding sub tree sizes.
	bool isLowestNode = true;
	while (stackIndex >= 0)
		{
		ZRef<ZBTree::Node> currentNode = fNodeSpace->LoadNode(inStack[stackIndex].first);
		long currentOffset = inStack[stackIndex].second;
		total += currentOffset;
		if (!currentNode->GetIsLeaf())
			{
			long subTreesToCount = isLowestNode ? currentOffset+1 : currentOffset;
			for (long x = 0; x < subTreesToCount; ++x)
				total += fNodeSpace->GetNodeSubTreeSize(currentNode->GetHandleAt(x));
			}
		--stackIndex;
		isLowestNode = false;
		}
	return total;
	}

void ZBTree::WalkNodes(NodeWalker& inWalker)
	{
	ZRef<ZBTree::Node> rootNode = fNodeSpace->LoadNode(fNodeSpace->GetRootHandle());
	this->Internal_WalkNodes(inWalker, rootNode);
	}

void ZBTree::Internal_WalkNodes(NodeWalker& inWalker, const ZRef<ZBTree::Node>& inNode)
	{
	inWalker.NodeEntered(inNode);

	if (long itemCount = inNode->GetItemCount())
		{
		for (long x = 0; x < itemCount; ++x)
			{
			if (!inNode->GetIsLeaf())
				{
				ZBTree::Handle theHandle = inNode->GetHandleAt(x);
				ZRef<ZBTree::Node> theNode = fNodeSpace->LoadNode(theHandle);
				this->Internal_WalkNodes(inWalker, theNode);
				}
			inWalker.DumpItem(inNode->GetItemAt(x));
			}
		if (!inNode->GetIsLeaf())
			{
			ZBTree::Handle rightMostHandle = inNode->GetHandleAt(itemCount);
			ZRef<ZBTree::Node> rightMostNode = fNodeSpace->LoadNode(rightMostHandle);
			this->Internal_WalkNodes(inWalker, rightMostNode);
			}
		}

	inWalker.NodeExited(inNode);
	}

ZRef<ZBTree::Node> ZBTree::DescendInto(ZRef<ZBTree::Node> inCurrentNode, long inSubTreeIndex, EAction& outAction)
	{
// By default, NoAction
	outAction = eActionNone;

// If we're at a leaf, there's nothing to descend into
	if (inCurrentNode->fIsLeaf)
		return ZRef<ZBTree::Node>();

// Otherwise, grab the node at the index in question
	ZRef<ZBTree::Node> child = fNodeSpace->LoadNode(inCurrentNode->fSubTreeHandles[inSubTreeIndex]);
// If it's nice and full, then return it
	if (child->fItemCount >= fNodeSpace->GetDegree())
		return child;

// Otherwise, now's the time to do some merging -- remember, we merge nodes on the
// way *down* the tree, rather than doing it on the way back up
	ZRef<ZBTree::Node> adjustedNode;
// Decide which node is the right node, and which the left
	if (inSubTreeIndex == 0)
		{
		ZRef<ZBTree::Node> sibling = fNodeSpace->LoadNode(inCurrentNode->fSubTreeHandles[1]);
		adjustedNode = this->Adjust(inCurrentNode, 0, child, sibling, outAction);
		}
	else
		{
		ZRef<ZBTree::Node> sibling = fNodeSpace->LoadNode(inCurrentNode->fSubTreeHandles[inSubTreeIndex - 1]);
		adjustedNode = this->Adjust(inCurrentNode, inSubTreeIndex - 1, sibling, child, outAction);
		}
	return adjustedNode;
	}

ZRef<ZBTree::Node> ZBTree::Adjust(ZRef<ZBTree::Node> inCurrentNode, long inIndex,
							ZRef<ZBTree::Node> inLeftNode, ZRef<ZBTree::Node> inRightNode, EAction& outAction)
	{
// We must have both nodes (paranoia)
	ZAssertStop(2, inLeftNode && inRightNode);
// One or other (or both) must be "half" full
	ZAssertStop(2, inLeftNode->fItemCount == fNodeSpace->GetDegree() - 1 || inRightNode->fItemCount == fNodeSpace->GetDegree() - 1);

// If both our nodes are half full (our max size is always odd, so half is floor(order/2))
	if (inLeftNode->fItemCount == fNodeSpace->GetDegree() - 1 && inRightNode->fItemCount == fNodeSpace->GetDegree() - 1)
		{
// Merge the two nodes
		inLeftNode->fItems[fNodeSpace->GetDegree() - 1] = inCurrentNode->fItems[inIndex];
		inLeftNode->MoveSubNode(inRightNode, 0, fNodeSpace->GetDegree(), fNodeSpace->GetDegree() - 1);
		inLeftNode->fItemCount = fNodeSpace->GetOrder();
		inLeftNode->fSubTreeSize += inRightNode->fSubTreeSize+1;
		fNodeSpace->DeleteNode(inRightNode);

		if (inCurrentNode->fItemCount > 1)
			{
			inCurrentNode->ShiftLeftAt(inIndex + 1);
			inCurrentNode->fSubTreeHandles[inIndex] = inLeftNode->GetHandle();
			}
		else
			{
			inCurrentNode->fItemCount = 0;
			fNodeSpace->SetNewRoot(inLeftNode->GetHandle());
			}
		inLeftNode->ContentChanged();
		inCurrentNode->ContentChanged();
		outAction = eActionMerge;
		return inLeftNode;
		}

	if (inLeftNode->fItemCount >= fNodeSpace->GetDegree())
		{
		// Rotate right
		inRightNode->ShiftRightAt(0);
		inRightNode->fItems[0] = inCurrentNode->fItems[inIndex];
		inRightNode->fSubTreeHandles[0] = inLeftNode->fSubTreeHandles[inLeftNode->fItemCount];
		inCurrentNode->fItems[inIndex] = inLeftNode->fItems[inLeftNode->fItemCount - 1];
		++inRightNode->fItemCount;
		--inLeftNode->fItemCount;

		long transferCount = 1;
		if (inRightNode->fSubTreeHandles[0] != kInvalidHandle)
			{
			ZRef<ZBTree::Node> aNode = fNodeSpace->LoadNode(inRightNode->fSubTreeHandles[0]);
			transferCount = aNode->fSubTreeSize+1;
			}

		inRightNode->fSubTreeSize += transferCount;
		inLeftNode->fSubTreeSize -= transferCount;

		inRightNode->ContentChanged();
		inLeftNode->ContentChanged();
		inCurrentNode->ContentChanged();
		outAction = eActionRotateRight;
		return inRightNode;
		}
	else
		{
		// inRightNode->keyCount >= order, so rotate left
		inLeftNode->fItems[fNodeSpace->GetDegree() - 1] = inCurrentNode->fItems[inIndex];
		inLeftNode->fSubTreeHandles[fNodeSpace->GetDegree()] = inRightNode->fSubTreeHandles[0];
		++inLeftNode->fItemCount;
		inCurrentNode->fItems[inIndex] = inRightNode->fItems[0];

		long transferCount = 1;
		if (inLeftNode->fSubTreeHandles[fNodeSpace->GetDegree()] != kInvalidHandle)
			{
			ZRef<ZBTree::Node> aNode = fNodeSpace->LoadNode(inLeftNode->fSubTreeHandles[fNodeSpace->GetDegree()]);
			transferCount = aNode->fSubTreeSize+1;
			}

		inRightNode->fSubTreeSize -= transferCount;
		inLeftNode->fSubTreeSize += transferCount;

		inRightNode->ShiftLeftAt(1);

		inLeftNode->ContentChanged();
		inRightNode->ContentChanged();
		inCurrentNode->ContentChanged();
		outAction = eActionRotateLeft;
		return inLeftNode;
		}
	}

void ZBTree::InsertNonFull(ZRef<ZBTree::Node> theNode, ZBTree::Item* theItem)
	{
	ZAssertStop(2, theNode);
// We know that theNode is not full, that's why we're being called
	ZRef<ZBTree::Node> parentNode(theNode);

	vector<ZRef<ZBTree::Node> > stack;
// Reserve some space -- we're unlikely to ever be 10 elements deep, but this will cut
// memory allocations.
	stack.reserve(10);

	long pos;
	while (true)
		{
		bool dummyExactFind;
		pos = parentNode->FindPosition(theItem, dummyExactFind);
		stack.push_back(parentNode);
		if (parentNode->fIsLeaf)
			break;
		++pos;
		ZRef<ZBTree::Node> childNode = fNodeSpace->LoadNode(parentNode->fSubTreeHandles[pos]);
		ZAssert(childNode && childNode->GetRefCount() > 0);
// If child is full
		if (childNode->fItemCount == fNodeSpace->GetOrder())
			{
			this->SplitChild(parentNode, pos, childNode);
			if (theItem->Compare(parentNode->fItems[pos]) >= 0)
				{
				++pos;
				childNode = fNodeSpace->LoadNode(parentNode->fSubTreeHandles[pos]);
				}
			}
		parentNode = childNode;
		}

// We allow insertions with duplication, so we don't bother to check the value of found
	if (parentNode->fItemCount > 0)
		{
		parentNode->ShiftRightAt(pos + 1);
		parentNode->fItems[pos+1] = theItem;
		}
	else
		parentNode->fItems[0] = theItem;
	++parentNode->fItemCount;
	parentNode->ContentChanged();
// Walk back up the tree incrementing the sub tree sizes by one
	for (vector<ZRef<ZBTree::Node> >::iterator x = stack.begin(); x != stack.end(); ++x)
		{
		ZAssertStop(2, (*x)->fSubTreeSize >= 0);
		++(*x)->fSubTreeSize;
		(*x)->ContentChanged();
		}
	}

ZRef<ZBTree::Node> ZBTree::SplitChild(ZRef<ZBTree::Node> parentNode,
										long fullChildPosition, ZRef<ZBTree::Node> fullChildNode)
	{
// Note that parentNode must *not* be full.
// Node that will be split is the child node. We take its median key, bump it up into parentNode, all keys
// to the right of that key are placed in a new node, which is created as a new child of parentNode

// So, we go from a parent node with one (full) child, to a parent node with two half-full nodes,
// and an additional item in the parent node
	ZRef<ZBTree::Node> newChildNode = fNodeSpace->CreateNode();
// Our new node will be a leaf if the split node is a leaf
	newChildNode->fIsLeaf = fullChildNode->fIsLeaf;

	newChildNode->MoveSubNode(fullChildNode, fNodeSpace->GetDegree(), 0, fNodeSpace->GetDegree() - 1);
// Now, shuffle the parent's content along to the right by one
	parentNode->ShiftRightAt(fullChildPosition);
// Place the median content item from fullChildNode into parentNode, at offset fullChildPosition
	parentNode->fItems[fullChildPosition] = fullChildNode->fItems[fNodeSpace->GetDegree() - 1];
// And put the pointer to the new child just after it
	parentNode->fSubTreeHandles[fullChildPosition+1] = newChildNode->GetHandle();
// nil out the fullChild's item ptr -- just so we know it's gone when debugging
	fullChildNode->fItems[fNodeSpace->GetDegree() - 1] = nil;
// Finally, update the children's item counts, now that we've grabbed all their data
	++parentNode->fItemCount;
	fullChildNode->fItemCount = fNodeSpace->GetDegree() - 1;
	newChildNode->fItemCount = fNodeSpace->GetDegree() - 1;

// Finally, finally. Update the sub tree sizes for all three nodes

	long theSize = 0;
	if (!newChildNode->fIsLeaf)
		{
		for (long j = 0; j <= newChildNode->fItemCount; j++)
			{
			ZRef<ZBTree::Node> aNode = fNodeSpace->LoadNode(newChildNode->fSubTreeHandles[j]);
			theSize += aNode->fSubTreeSize;
			}
		}
	theSize += newChildNode->fItemCount;
	newChildNode->fSubTreeSize = theSize;
	fullChildNode->fSubTreeSize -= theSize+1;

	parentNode->ContentChanged();
	fullChildNode->ContentChanged();
	newChildNode->ContentChanged();
	return newChildNode;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::Item

long ZBTree::Item::CompareForSearch(const ZBTree::Item* inOther, bool inStrictComparison)
	{ return this->Compare(inOther); }

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::NodeSpace

ZBTree::NodeSpace::NodeSpace(long inDegree)
:	fDegree(inDegree), fRootNodeHandle(kInvalidHandle)
	{}

ZBTree::NodeSpace::NodeSpace(long inDegree, ZBTree::Handle inRootHandle)
:	fDegree(inDegree), fRootNodeHandle(inRootHandle)
	{}

ZBTree::NodeSpace::~NodeSpace()
	{}

void ZBTree::NodeSpace::SetNewRoot(ZBTree::Handle inHandle)
	{ fRootNodeHandle = inHandle; }

void ZBTree::NodeSpace::Create()
	{
	if (fRootNodeHandle == kInvalidHandle)
		{
		ZRef<ZBTree::Node> theRoot = this->CreateNode();
		fRootNodeHandle = theRoot->GetHandle();
		}
	}
long ZBTree::NodeSpace::GetNodeSubTreeSize(ZBTree::Handle inHandle)
	{
	if (ZRef<ZBTree::Node> theNode = this->LoadNode(inHandle))
		return theNode->GetSubTreeSize();
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBTree::Node

ZBTree::Node::Node(ZBTree::NodeSpace* inNodeSpace, ZBTree::Handle inHandle)
:	fNodeSpace(inNodeSpace), fHandle(inHandle)
	{
// We'll assume we're a leaf to start with
	fSubTreeSize = 0;
	fIsLeaf = true;
	long nodespaceOrder = inNodeSpace->GetOrder();
	fItems = new ZBTree::Item*[nodespaceOrder];
	fSubTreeHandles = new ZBTree::Handle[inNodeSpace->GetOrder() + 1];
	for (long x = 0; x < nodespaceOrder; x++)
		{
		fItems[x] = nil;
		fSubTreeHandles[x] = kInvalidHandle;
		}
	fSubTreeHandles[nodespaceOrder] = kInvalidHandle;
	fItemCount = 0;
	}

ZBTree::Node::~Node()
	{
	if (fItems)
		{
		for (long x = 0; x < fItemCount; x++)
			delete fItems[x];
		delete[] fItems;
		}
	if (fSubTreeHandles)
		delete[] fSubTreeHandles;
	}

void ZBTree::Node::Finalize()
	{ fNodeSpace->NodeBeingFinalized(this); }

void ZBTree::Node::ContentChanged()
	{}

long ZBTree::Node::FindPosition(const ZBTree::Item* inComparitor, bool& outExactFind)
	{
	if (fItemCount <= 7)
		{
		// Small number of keys, do a linear search
		if (fItemCount == 0)
			{
			outExactFind = false;
			return -1;
			}
		long result;
		long i;
		for (i = 0; i < fItemCount; i++)
			{
			result = fItems[i]->Compare(inComparitor);
			if (result >= 0)
				break;
			}
		if (result == 0)
			{
			outExactFind = true;
			return i;
			}
		else
			{
			outExactFind = false;
			return i-1;
			}
		}
	// Do a binary search
	long result;
	long lo = 0, hi = fItemCount - 1, mid;
	while (lo <= hi)
		{
		mid = (lo + hi) / 2;
		result = fItems[mid]->Compare(inComparitor);
		if (result == 0)
			{
			outExactFind = true;
			return mid;
			}
		if (result < 0)
			lo = mid + 1;
		else
			hi = mid - 1;
		}
	outExactFind = false;
	return (result <= 0) ? (mid) : mid - 1;
	}

long ZBTree::Node::FindPosition_LowestEqualOrGreater(const ZBTree::Item* inComparitor, long& outResult)
	{
	outResult = -1;
	if (fItemCount == 0)
		return -1;
// We have to declare this here, because we want to return it below
	long i;
	for (i = 0; i < fItemCount; ++i)
		{
// We're looking for an item that is equal to or greater than the comparitor.
// So tell the items that we need a strict comparison. Just because we have
// equal prefixes does not mean that there are not items further down the tree
// that also have equal prefixes, but which are closer in value to what we're looking for.
		outResult = fItems[i]->CompareForSearch(inComparitor, true);
		if (outResult >= 0)
			break;
		}
	return i;
	}

long ZBTree::Node::FindPosition_LowestGreater(const ZBTree::Item* inComparitor, long& outResult)
	{
	outResult = -1;
	if (fItemCount == 0)
		return -1;
// We have to declare this here, because we want to return it below
	long i;
	for (i = 0; i < fItemCount; ++i)
		{
// pass false for the strictComparison flag. We're looking for nodes whose
// *prefixes* are greater than our "prefix"
		outResult = fItems[i]->CompareForSearch(inComparitor, false);
		if (outResult > 0)
			break;
		}
	return i;
	}

void ZBTree::Node::ShiftRightAt(long inIndex)
	{
	for (long i = fItemCount - 1; i >= inIndex; --i)
		{
		fItems[i + 1] = fItems[i];
		fSubTreeHandles[i + 1 + 1] = fSubTreeHandles[i + 1];
		}
	fSubTreeHandles[inIndex + 1] = fSubTreeHandles[inIndex];
	fItems[inIndex] = nil;
	this->ContentChanged();
	}

void ZBTree::Node::ShiftLeftAt(long inIndex)
	{
	for (long i = inIndex; i < fItemCount; ++i)
		{
		fItems[i - 1] = fItems[i];
		fSubTreeHandles[i - 1] = fSubTreeHandles[i];
		}
	fSubTreeHandles[fItemCount - 1] = fSubTreeHandles[fItemCount];
	fSubTreeHandles[fItemCount] = kInvalidHandle;
	fItems[fItemCount - 1] = nil;
	--fItemCount;
	this->ContentChanged();
	}

void ZBTree::Node::MoveSubNode(ZRef<ZBTree::Node> inOther, long inOtherPos, long inOurPos, long inKeys)
	{
	long i, j;
	for (i = inOurPos, j = inOtherPos; i < inOurPos + inKeys; ++i, ++j)
		{
		fItems[i] = inOther->fItems[j];
		fSubTreeHandles[i] = inOther->fSubTreeHandles[j];
		inOther->fItems[j] = nil;
		inOther->fSubTreeHandles[j] = kInvalidHandle;
		}
	fSubTreeHandles[inOurPos + inKeys] = inOther->fSubTreeHandles[inOtherPos + inKeys];
	inOther->fSubTreeHandles[inOtherPos + inKeys] = kInvalidHandle;
	this->ContentChanged();
	}

