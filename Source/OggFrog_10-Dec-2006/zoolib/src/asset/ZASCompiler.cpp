static const char rcsid[] = "@(#) $Id: ZASCompiler.cpp,v 1.9 2006/04/10 20:44:19 agreen Exp $";

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

#include "ZASCompiler.h"

#include "ZCompat_algorithm.h" // For sort
#include "ZMemory.h"
#include "ZStream_Memory.h"
#include "ZStreamRWPos_RAM.h"

using std::runtime_error;
using std::string;
using std::vector;

static void sWriteCount(const ZStreamW& inStream, uint32 inCount)
	{
	if (inCount < 0xFF)
		{
		inStream.WriteUInt8(uint8(inCount));
		}
	else
		{
		inStream.WriteUInt8(0xFF);
		inStream.WriteUInt32(inCount);
		}
	}

static void sWriteStringWithSizePrefixAndZeroTerminator(const ZStreamW& inStream, const string& inString)
	{
	sWriteCount(inStream, inString.size());
	if (inString.size())
		inStream.Write(inString.data(), inString.size());
	inStream.WriteUInt8(0);
	}

static void sSkipWrite(const ZStreamW& inStream, size_t inCount)
	{
	for (size_t x = 0; x < inCount; ++x)
		inStream.WriteUInt8(0);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZASCompiler::NameEntry

class ZASCompiler::NameEntry
	{
public:
	struct ReverseCompareUseCounts { bool operator()(NameEntry* a, NameEntry* b) { return a->fUseCount > b->fUseCount; } };
	struct CompareName { bool operator()(NameEntry* a, const string& b) { return a->fName < b; } };

	string fName;
	int fUseCount;
	int fNameIndex;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZASCompiler::Node

class ZASCompiler::Node
	{
public:
	Node(NameEntry* inNameEntry);
	~Node();

	void FindOrAllocate(NameEntry** inNameEntries, size_t inNameCount, size_t inDataOffset, size_t inDataSize);
	void FindOrAllocate(NameEntry** inNameEntries, size_t inNameCount, const vector<NameEntry*>& inUnionEntries);
	void WriteTo(const ZStreamW& inStream);

	size_t AssignUseCounts();
private:
	NameEntry* fNameEntry;
	Node* fChild;
	Node* fSibling;
	size_t fDataOffset;
	size_t fDataSize;
	vector<NameEntry*> fUnionEntries;
	};

ZASCompiler::Node::Node(NameEntry* inNameEntry)
	{
	fNameEntry = inNameEntry;
	fChild = nil;
	fSibling = nil;
	fDataOffset = 0;
	fDataSize = 0;
	}

ZASCompiler::Node::~Node()
	{
	Node* currentChild = fChild;
	while (currentChild)
		{
		Node* nextChild = currentChild->fSibling;
		delete currentChild;
		currentChild = nextChild;
		}
	}

void ZASCompiler::Node::FindOrAllocate(NameEntry** inNameEntries, size_t inNameCount, size_t inDataOffset, size_t inDataSize)
	{
	if (inNameCount == 0)
		{
		fDataOffset = inDataOffset;
		fDataSize = inDataSize;
		return;
		}

	Node* priorNode = nil;
	Node* currentNode = fChild;
	while (currentNode)
		{
		if (currentNode->fNameEntry == inNameEntries[0])
			{
			currentNode->FindOrAllocate(inNameEntries + 1, inNameCount - 1, inDataOffset, inDataSize);
			return;
			}
		priorNode = currentNode;
		currentNode = currentNode->fSibling;
		}

	Node* newNode = new Node(inNameEntries[0]);
	newNode->fSibling = currentNode;
	if (priorNode)
		priorNode->fSibling = newNode;
	else
		fChild = newNode;

	newNode->FindOrAllocate(inNameEntries + 1, inNameCount - 1, inDataOffset, inDataSize);
	}

void ZASCompiler::Node::FindOrAllocate(NameEntry** inNameEntries, size_t inNameCount, const vector<NameEntry*>& inUnionEntries)
	{
	if (inNameCount == 0)
		{
		for (vector<NameEntry*>::const_iterator i = inUnionEntries.begin(); i != inUnionEntries.end(); ++i)
			fUnionEntries.push_back(*i);
		return;
		}

	Node* priorNode = nil;
	Node* currentNode = fChild;
	while (currentNode)
		{
		if (currentNode->fNameEntry == inNameEntries[0])
			{
			currentNode->FindOrAllocate(inNameEntries + 1, inNameCount - 1, inUnionEntries);
			return;
			}
		priorNode = currentNode;
		currentNode = currentNode->fSibling;
		}

	Node* newNode = new Node(inNameEntries[0]);
	newNode->fSibling = currentNode;
	if (priorNode)
		priorNode->fSibling = newNode;
	else
		fChild = newNode;

	newNode->FindOrAllocate(inNameEntries + 1, inNameCount - 1, inUnionEntries);
	}

size_t ZASCompiler::Node::AssignUseCounts()
	{
	for (vector<NameEntry*>::iterator i = fUnionEntries.begin(); i != fUnionEntries.end(); ++i)
		++(*i)->fUseCount;

	// Start at one to count ourselves
	size_t childTreeSize = 1;
	Node* currentChild = fChild;
	while (currentChild)
		{
		childTreeSize += currentChild->AssignUseCounts();
		++childTreeSize;
		currentChild = currentChild->fSibling;
		}
	if (fNameEntry)
		fNameEntry->fUseCount += childTreeSize;
	return childTreeSize;
	}

void ZASCompiler::Node::WriteTo(const ZStreamW& inStream)
	{
	if (fUnionEntries.size())
		{
		// Write our type code
		inStream.WriteUInt8(2);
		sWriteCount(inStream, fUnionEntries.size());
		for (vector<NameEntry*>::iterator i = fUnionEntries.begin(); i != fUnionEntries.end(); ++i)
			sWriteCount(inStream, (*i)->fNameIndex);
		}
	else if (fDataOffset || fDataSize)
		{
		// Write our type code
		inStream.WriteUInt8(1);
		// Write the size and offset of our data (if any)
		sWriteCount(inStream, fDataSize);
		if (fDataSize)
			inStream.WriteUInt32(fDataOffset);
		}
	else
		{
		// Write our type code
		inStream.WriteUInt8(0);

		// Figure out and write the count of children we have
		size_t countOfChildren = 0;
		Node* currentChild = fChild;
		while (currentChild)
			{
			++countOfChildren;
			currentChild = currentChild->fSibling;
			}
		sWriteCount(inStream, countOfChildren);

		// Write each child
		currentChild = fChild;
		while (currentChild)
			{
			sWriteCount(inStream, currentChild->fNameEntry->fNameIndex);
			currentChild->WriteTo(inStream);
			currentChild = currentChild->fSibling;
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZASCompiler

ZASCompiler::ZASCompiler(const ZStreamW& inStream)
:	fStreamW(inStream)
	{
	fRoot = nil;
	}

ZASCompiler::~ZASCompiler()
	{
	delete fRoot;
	for (vector<NameEntry*>::iterator i = fNameEntries.begin(); i != fNameEntries.end(); ++i)
		delete *i;
	}

void ZASCompiler::StartParse()
	{
	ZAssert(fRoot == nil);
	fRoot = new Node(nil);
	}

static const char sMagicText[] = "ZAO v1.0";
static size_t sMagicTextSize = sizeof(sMagicText);

void ZASCompiler::EndParse()
	{
	ZAssert(fRoot);

	// Walk our tree and have the nodes set up our name entries' use counts
	fRoot->AssignUseCounts();

	// Sort our name entries by use count
	sort(fNameEntries.begin(), fNameEntries.end(), NameEntry::ReverseCompareUseCounts());

	// Find the first with a zero use count
	vector<NameEntry*>::iterator firstZero;
	for (firstZero = fNameEntries.begin(); (firstZero != fNameEntries.end()) && ((*firstZero)->fUseCount != 0); ++firstZero)
		{}

	// Set the names' indices for use by the nodes as they write themselves.
	for (vector<NameEntry*>::iterator i = fNameEntries.begin(); i != firstZero; ++i)
		(*i)->fNameIndex = i - fNameEntries.begin();

	uint64 startOfHeader = fStreamW.GetCount();

	// Write out the table of contents for the name table
	fStreamW.WriteUInt32(firstZero - fNameEntries.begin());

	size_t offset = 0;
	for (vector<NameEntry*>::iterator i = fNameEntries.begin(); i != firstZero; ++i)
		{
		fStreamW.WriteInt32(offset);
		offset += (*i)->fName.size() + 1;
		}
	// Write the offset of the end of the last name
	fStreamW.WriteInt32(offset);

	// Write out the names, including terminating zero
	for (vector<NameEntry*>::iterator i = fNameEntries.begin(); i != firstZero; ++i)
		{
		if ((*i)->fName.size())
			fStreamW.Write((*i)->fName.data(), (*i)->fName.size());
		fStreamW.WriteUInt8(0);
		}

	// Walk the node tree
	fRoot->WriteTo(fStreamW);

	// Write the overall size of the header
	fStreamW.WriteUInt32(fStreamW.GetCount() - startOfHeader);

	// Write out the magic text
	fStreamW.Write(sMagicText, sMagicTextSize);

	delete fRoot;
	fRoot = nil;
	for (vector<NameEntry*>::iterator i = fNameEntries.begin(); i != fNameEntries.end(); ++i)
		delete *i;
	fNameEntries.clear();
	}

void ZASCompiler::EnterName(const string& inName, bool iWasInBlock, bool iNowInBlock)
	{
	fNameStack.push_back(inName);
	}

void ZASCompiler::ExitName(bool iWasInBlock, bool iNowInBlock)
	{
	fNameStack.pop_back();
	}

void ZASCompiler::ParsedString(const string& inValue)
	{
	// We add the data of the string including its zero terminator, hence the '+ 1' below.
	this->AddAsset(fNameStack, "!string", ZStreamRPos_Memory(inValue.c_str(), inValue.size() + 1));
	}

void ZASCompiler::ParsedStringTable(const vector<string>& inValues)
	{
	ZStreamRWPos_RAM theStream;
	sWriteCount(theStream, inValues.size());
	for (size_t x = 0; x < inValues.size(); ++x)
		sWriteStringWithSizePrefixAndZeroTerminator(theStream, inValues[x]);
	theStream.SetPosition(0);
	this->AddAsset(fNameStack, "!stringtable", theStream);
	}

void ZASCompiler::ParsedBinary(const ZStreamR& inStream)
	{
	this->AddAsset(fNameStack, "!binary", inStream);
	}

void ZASCompiler::ParsedUnion(const vector<string>& iNames)
	{
	// Turn the list of names into a list of NameEntries
	vector<NameEntry*> nameEntries;
	for (size_t x = 0; x < fNameStack.size(); ++x)
		nameEntries.push_back(this->AllocateName(fNameStack[x]));

	vector<NameEntry*> unionEntries;
	for (size_t x = 0; x < iNames.size(); ++x)
		unionEntries.push_back(this->AllocateName(iNames[x]));
	fRoot->FindOrAllocate(&nameEntries[0], nameEntries.size(), unionEntries);
	}

void ZASCompiler::AddAsset(const vector<string>& inNameStack, const string& inType, const ZStreamR& inStream)
	{
	// Turn the list of names into a list of NameEntries
	vector<NameEntry*> nameEntries;
	for (size_t x = 0; x < inNameStack.size(); ++x)
		nameEntries.push_back(this->AllocateName(inNameStack[x]));
	// Add the type to the end
	nameEntries.push_back(this->AllocateName(inType));

	// Transcribe the data
	uint64 startPosition = fStreamW.GetCount();
	uint64 countRead;
	uint64 countWritten;
	fStreamW.CopyAllFrom(inStream, &countRead, &countWritten);
	if (countRead != countWritten)
		throw runtime_error("ZASCompiler::AddAsset, unexpected end of output stream");

	uint64 endPosition = fStreamW.GetCount();

	// Find the node and tell it its data info
	fRoot->FindOrAllocate(&nameEntries[0], nameEntries.size(), startPosition, endPosition - startPosition);

	// Align the end of the data at the next four byte boundary. Note that we're aligned relative to where
	// we wrote our first byte in the stream, _not_ relative to the start of whatever underlies the stream.
	::sSkipWrite(fStreamW, ((endPosition + 3) & ~3) - endPosition);
	}

ZASCompiler::NameEntry* ZASCompiler::AllocateName(const string& inName)
	{
	if (inName.empty())
		throw runtime_error("ZASCompiler::AllocateName, empty name");

	vector<NameEntry*>::iterator iter = lower_bound(fNameEntries.begin(), fNameEntries.end(), inName, NameEntry::CompareName());
	if (iter != fNameEntries.end() && (*iter)->fName == inName)
		return *iter;

	NameEntry* theEntry = new NameEntry;
	theEntry->fName = inName;
	theEntry->fUseCount = 0;
	fNameEntries.insert(iter, theEntry);
	return theEntry;
	}
