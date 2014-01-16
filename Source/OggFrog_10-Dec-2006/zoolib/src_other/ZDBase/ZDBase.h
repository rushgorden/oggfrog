/*  @(#) $Id: ZDBase.h,v 1.22 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZDBase__
#define __ZDBase__ 1
#include "zconfig.h"

#include "ZBlockStore.h"
#include "ZBTree.h"
#include "ZRefCount.h"
#include "ZThread.h"
#include "ZTuple.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>

// =================================================================================================
class ZProgressWatcher;

class ZDBTreeNodeSpace;
class ZDBTreeNode;
class ZDBTreeItem;
class ZDBSearch;
class ZDBField;
class ZDBRecord;
class ZDBTable;
class ZDBIndex;
class ZDBTreeItem;

// =================================================================================================
#pragma mark -
#pragma mark * ZDBRecordID

// ZDBRecordID used to be a typedef-ed Z64BitInt, a class which has gone away. I'll
// be ditching ZDBRecordID per se in favor of just a uint64 sometime soon.

class ZDBRecordID
	{
public:
	uint64 fValue;

	ZDBRecordID() { fValue = 0; }
	ZDBRecordID(uint64 inValue) { fValue = inValue; }
	~ZDBRecordID() {}
	ZDBRecordID& operator=(const ZDBRecordID& inOther) { fValue = inOther.fValue; return *this;}
	ZDBRecordID& operator=(uint64 inValue) { fValue = inValue; return *this; }

	operator const void*() const;

	bool IsValid() const { return fValue; }

	bool operator==(const ZDBRecordID& inOther) const { return fValue == inOther.fValue; }
	bool operator!=(const ZDBRecordID& inOther) const { return fValue != inOther.fValue; }
	bool operator<(const ZDBRecordID& inOther) const { return fValue < inOther.fValue; }
	bool operator>(const ZDBRecordID& inOther) const { return fValue > inOther.fValue; }

	bool operator==(const int& inOther) const { return fValue == inOther; }
	bool operator!=(const int& inOther) const { return fValue != inOther; }

	ZDBRecordID& operator+=(int inDelta)
		{
		fValue += inDelta;
		return *this;
		}

	void ToStream(const ZStreamW& inStream) const { inStream.WriteUInt64(fValue); }
	void FromStream(const ZStreamR& inStream) { fValue = inStream.ReadUInt64(); }

	static size_t sSizeInStream() { return sizeof(uint64); }

	uint64 GetValue() const { return fValue; }

	std::string AsString() const;
	std::string AsString10() const;

	static ZDBRecordID sFromString(const std::string& iString);
	static ZDBRecordID sFromString(const char* iString);

	static ZDBRecordID sFromStringHex(const std::string& iString);
	static ZDBRecordID sFromStringHex(const char* iString);
	};

inline ZDBRecordID::operator const void*() const
	{ return (const void*)(fValue ? 1 : 0); }

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldName

class ZDBFieldName
	{
public:
	static const ZDBFieldName ID;
	ZDBFieldName()
		{ fName = 0; }
	ZDBFieldName(uint32 inName)
		{ fName = inName; }
	ZDBFieldName(const char* inName);
	~ZDBFieldName()
		{}
	ZDBFieldName& operator=(const ZDBFieldName& inOther)
		{ fName = inOther.fName; return *this; }

	uint32 GetName() const
		{ return fName; }
	void SetName(uint32 inName)
		{ fName = inName; }

	bool operator==(const ZDBFieldName& inOther) const
		{ return fName == inOther.fName; }
	bool operator!=(const ZDBFieldName& inOther) const
		{ return fName != inOther.fName; }
	bool operator<(const ZDBFieldName& inOther) const
		{ return fName < inOther.fName; }

	void FromStream(const ZStreamR& inStream);
	void ToStream(const ZStreamW& inStream) const;
	static size_t sSizeInStream()
		{ return sizeof(uint32); }

	std::string AsString() const;

private:
	uint32 fName;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldRep

class ZDBFieldRep
	{
protected:
	ZDBFieldRep() {}

public:
	virtual ~ZDBFieldRep() {}

	enum FieldType
			{ eInvalid, eNumber, eString, eBinary, eBLOB, eID, eIString, eFloat, eDouble };

	virtual std::string AsString() const;

	virtual void FromStream(const ZStreamR& inStream) = 0;
	virtual void ToStream(const ZStreamW& inStream) const = 0;

	virtual ZDBFieldRep::FieldType GetFieldType() const = 0;
	virtual long Compare(const ZDBFieldRep* inOther) const = 0;

	virtual ZDBFieldRep* Clone() const = 0;
	virtual ZDBFieldRep* TotalClone(ZBlockStore* inSourceBlockStore, ZBlockStore* inDestBlockStore) const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)= 0;

	virtual void DeleteExtraStorage(ZBlockStore* inBlockStore);
	virtual bool CreateExtraStorage(ZBlockStore* inBlockStore);
	virtual bool CheckExtraStorageOkay(ZBlockStore* inBlockStore);
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepBLOB

class ZDBFieldRepBLOB : public ZDBFieldRep
	{
public:
	ZDBFieldRepBLOB() : fBLOBHandle(0) {}
	ZDBFieldRepBLOB(ZBlockStore::BlockID inBlockID) : fBLOBHandle(inBlockID) {}

	virtual ~ZDBFieldRepBLOB();

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual ZDBFieldRep* TotalClone(ZBlockStore* inSourceBlockStore, ZBlockStore* inDestBlockStore) const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	virtual void DeleteExtraStorage(ZBlockStore* inBlockStore);
	virtual bool CreateExtraStorage(ZBlockStore* inBlockStore);
	virtual bool CheckExtraStorageOkay(ZBlockStore* inBlockStore);

	virtual std::string AsString() const;

	ZBlockStore::BlockID GetBLOBHandle() const
		{ return fBLOBHandle; }

private:
	ZBlockStore::BlockID fBLOBHandle;
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepNumber

class ZDBFieldRepNumber : public ZDBFieldRep
	{
public:
	ZDBFieldRepNumber() : fNumber(0) {}
	ZDBFieldRepNumber(long inNumber) : fNumber(inNumber) {}

	virtual ~ZDBFieldRepNumber();

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	long GetNumber() const
		{ return fNumber; }
	virtual std::string AsString() const;
private:
	long fNumber;
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepID

class ZDBFieldRepID : public ZDBFieldRep
	{
public:
	ZDBFieldRepID() : fValue(0) {};
	ZDBFieldRepID(const ZDBRecordID& inID) : fValue(inID) {}
	virtual ~ZDBFieldRepID();

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	virtual std::string AsString() const;

	const ZDBRecordID& GetValue() const
		{ return fValue; }

	void SetValue(const ZDBRecordID& newValue);

private:
	ZDBRecordID fValue;
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepStringBase

class ZDBFieldRepStringBase : public ZDBFieldRep
	{
public:
	ZDBFieldRepStringBase() {}
	ZDBFieldRepStringBase(const std::string& inString);
	virtual ~ZDBFieldRepStringBase();

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const = 0;
	virtual long Compare(const ZDBFieldRep* inOther) const = 0;

	virtual std::string AsString() const;

	const std::string& GetString() const
		{ return fString; }

protected:
	std::string fString;
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepString

class ZDBFieldRepString : public ZDBFieldRepStringBase
	{
public:
	ZDBFieldRepString() {}
	ZDBFieldRepString(const std::string& inString);
	virtual ~ZDBFieldRepString();

	virtual ZDBFieldRep* Clone() const;
	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepIString

class ZDBFieldRepIString : public ZDBFieldRepStringBase
	{
public:
	ZDBFieldRepIString() {}
	ZDBFieldRepIString(const std::string& inString);
	virtual ~ZDBFieldRepIString();

	virtual ZDBFieldRep* Clone() const;
	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepBinary

class ZDBFieldRepBinary : public ZDBFieldRep
	{
public:
	ZDBFieldRepBinary() : fData(nil), fLength(0) {}
	ZDBFieldRepBinary(const void* inData, size_t inLength);
	ZDBFieldRepBinary(const ZStreamR& iStream, size_t iLength);

	virtual ~ZDBFieldRepBinary();

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* inOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	virtual std::string AsString() const;

	size_t GetLength() const { return fLength; }
	const void* GetData() const { return fData; }

private:
	char* fData;
	size_t fLength;
	};

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepFloat

class ZDBFieldRepFloat : public ZDBFieldRep
	{
public:
	ZDBFieldRepFloat() : fFloat(0.0) {};
	ZDBFieldRepFloat(const float iFloat) : fFloat(iFloat) {};

	virtual ~ZDBFieldRepFloat();

	virtual void FromStream(const ZStreamR& iStream);
	virtual void ToStream(const ZStreamW& iStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* iOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	float GetFloat() const { return fFloat; }
private:
	float fFloat;

	};


// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepDouble

class ZDBFieldRepDouble : public ZDBFieldRep
	{
public:
	ZDBFieldRepDouble() : fDouble(0.0) {};
	ZDBFieldRepDouble(double iDouble) : fDouble(iDouble) {};

	virtual ~ZDBFieldRepDouble();

	virtual void FromStream(const ZStreamR& iStream);
	virtual void ToStream(const ZStreamW& iStream) const;

	virtual ZDBFieldRep::FieldType GetFieldType() const;
	virtual long Compare(const ZDBFieldRep* iOther) const;

	virtual ZDBFieldRep* Clone() const;
	virtual void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore);

	double GetDouble() const { return fDouble; }
private:
	double fDouble;

	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBField

class ZDBField
	{
private:
	ZDBField(ZDBRecord* inOwnerRecord, const ZDBFieldName& inFieldName, ZDBFieldRep* inRep);
	virtual ~ZDBField();

	void FromStream(const ZStreamR& inStream);
	void ToStream(const ZStreamW& inStream) const;

	void DeleteExtraStorage(ZBlockStore* inBlockStore);
	bool CreateExtraStorage(ZBlockStore* inBlockStore);
	bool CheckExtraStorageOkay(ZBlockStore* inBlockStore);

	void CopyFrom(ZBlockStore* inThisBlockStore, const ZDBField* inOther, ZBlockStore* inOtherBlockStore);

public:
	const ZDBFieldName& GetFieldName() const { return fFieldName; }
	ZDBFieldRep::FieldType GetFieldType() const;

	void SetRep(ZDBFieldRep* inRep);

// Shortcut rep setters
	void SetRep(const std::string& inString);
	void SetRep(const long inNumber);
	void SetRep(const ZDBRecordID& inID);
	void SetRepFloat(const float iFloat);
	void SetRepDouble(const double iDouble);

	const ZDBFieldRep* GetRep() const;
	ZDBFieldRep* UnsafeGetRep() { return fFieldRep; }

	ZDBFieldRep* GetClonedRep() const;

	friend class ZDBRecord;
	friend class ZDBIndex;

private:
	ZDBRecord* fRecord;
	ZDBFieldName fFieldName;
	ZDBFieldRep* fFieldRep;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldSpec

class ZDBFieldSpec
	{
public:
	ZDBFieldSpec()
		{ fFieldName = uint32(0); fFieldType = ZDBFieldRep::eInvalid; }
	ZDBFieldSpec(const ZDBFieldName& inFieldName, ZDBFieldRep::FieldType inFieldType)
	:	fFieldName(inFieldName), fFieldType(inFieldType) {}
	ZDBFieldSpec(const ZDBFieldSpec& inSpec)
	:	fFieldName(inSpec.fFieldName), fFieldType(inSpec.fFieldType) {}

	bool operator==(const ZDBFieldSpec& inOther) const
		{ return fFieldName == inOther.fFieldName && fFieldType == inOther.fFieldType; }
	bool operator!=(const ZDBFieldSpec& inOther) const
		{ return fFieldName != inOther.fFieldName || fFieldType != inOther.fFieldType; }

	void SetFieldName(ZDBFieldName& inFieldName)
		{ fFieldName = inFieldName; }
	const ZDBFieldName& GetFieldName() const
		{ return fFieldName; }
	ZDBFieldRep::FieldType GetFieldType() const
		{ return fFieldType; }

	void FromStream(const ZStreamR& inStream);
	void ToStream(const ZStreamW& inStream) const;

private:
	ZDBFieldName fFieldName;
	ZDBFieldRep::FieldType fFieldType;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBSearchSpec

class ZDBSearchSpec
	{
public:
	ZDBSearchSpec()
		: fFieldName(uint32(0)), fFieldRep(nil) {}
	ZDBSearchSpec(const ZDBFieldName& inFieldName, ZDBFieldRep* inFieldRep)
	:	fFieldName(inFieldName), fFieldRep(inFieldRep) {}
	ZDBSearchSpec(const ZDBSearchSpec& inOther);
	~ZDBSearchSpec();

	ZDBSearchSpec& operator=(const ZDBSearchSpec& other);

	const ZDBFieldName& GetFieldName() const
		{ return fFieldName; }
	const ZDBFieldRep* GetFieldRep() const
		{ return fFieldRep; }
private:
	ZDBFieldName fFieldName;
	ZDBFieldRep* fFieldRep;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBDatabase

class ZDBDatabase
	{
public:
	ZDBDatabase(ZBlockStore* inBlockStore);
	ZDBDatabase(ZBlockStore* inBlockStore, ZBlockStore::BlockID inBlockID);
	virtual ~ZDBDatabase();

	ZBlockStore::BlockID GetHeaderBlockID()
		{ return fHeaderBlockID; }
	ZBlockStore* GetBlockStore()
		{ return fBlockStore; }

	long GetRefCon();
	void SetRefCon(long inRefCon);

	ZDBTable* CreateTable(const std::string& inName, const std::vector<ZDBFieldSpec>& inFieldSpecList, bool fullySpanningIndices, bool createIDs);
	void DeleteTable(ZDBTable* inTable);

	ZDBTable* GetTable(const std::string& inName);
	ZDBTable* GetTableByName(const std::string& inName)
		{ return this->GetTable(inName); }

	void GetTableVector(std::vector<ZDBTable*>& outTableVector) const;

	virtual void Flush();

	bool Validate(ZProgressWatcher* iProgressWatcher);
	void CopyInto(ZProgressWatcher* iProgressWatcher, ZDBDatabase& iDestDB);

protected:
	virtual ZDBTable* MakeTable(const std::string& inName, const std::vector<ZDBFieldSpec>& inFieldSpecList, bool inFullySpanningIndices, bool inCreateIDs);

private:
	virtual void WriteBack();
	virtual void LoadUp();

	std::vector<ZDBTable*> fTables;
	ZBlockStore* fBlockStore;
	ZBlockStore::BlockID fHeaderBlockID;
	long fRefCon;
	bool fDirty;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBRecord

class ZDBRecord : public ZRefCountedWithFinalization
	{
private:
	ZDBRecord(); // Not implemented
	ZDBRecord(ZDBTable* inTable, const ZDBTreeItem* inTreeItem, bool inPulledFromIndices = false);
	ZDBRecord(ZDBTable* inTable, ZBlockStore::BlockID inBlockID, const std::vector<ZDBFieldSpec>& inSpecList);
	ZDBRecord(ZDBTable* inTable, ZBlockStore::BlockID inBlockID,
			const std::vector<ZDBFieldSpec>& inSpecList, const ZDBRecordID& inRecordID, bool inPulledFromIndices);

public:
	virtual ~ZDBRecord();

// From ZRefCountedWithFinalization
	virtual void Finalize();

// Our protocol
	ZRWLock* GetLock();

	bool IsDeleted()
		{ return fDeleted; }

	bool CanBeRead();
	bool CanBeWritten();

	void RecordPreRead();
	void RecordPostRead();

	bool HasField(const ZDBFieldName& inFieldName) const;
	ZDBField* GetField(const ZDBFieldName& inFieldName) const;
	const std::vector<ZDBField*>& GetFields() const;
	ZDBTable* GetTable() const { return fTable; }
	ZDBRecordID GetRecordID() const;

	void CopyFrom(ZDBRecord* inOther, bool inCopyRecordID);

	bool IsDirty() { return fDirty; }
	void MarkClean() { fDirty = false; }

	ZTuple GetAsTuple();

private:
	void FieldPreChange(ZDBField* inField);
	void FieldPostChange(ZDBField* inField);

	virtual void FromStream(const ZStreamR& inStream);
	virtual void ToStream(const ZStreamW& inStream);

	ZBlockStore::BlockID GetBlockID() const
		{ return fBlockID; }

	bool CheckAllFieldsOkay();

	bool IsIdenticalTo(const ZDBRecord* inRecord) const;
	struct CompareForCheck
		{ bool operator()(ZDBRecord* const& first, ZDBRecord* const& second) const; };

	void DeleteYourself();
	void CreateFields(const std::vector<ZDBFieldSpec>& inSpecList, const ZDBRecordID& inRecordID);

	friend class ZDBIndex;
	friend class ZDBTable;
	friend class ZDBField;
	friend class ZDBTreeItem;
	friend struct CompareForCheck;

private:
	ZDBTable* fTable;
	ZBlockStore::BlockID fBlockID;
	bool fDeleted;
	bool fPulledFromIndices;
	bool fFinalizationInProgress;
	bool fDirty;
	std::vector<ZDBField*> fFieldList;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex

class ZDBIndex
	{
public:
	class iterator;
	class NodeWalker;

// These have been moved to the public section to allow dbbrowser to look at it
	const std::vector<ZDBFieldName>& GetIndexedFieldNames() { return fIndexedFields; }
	static std::string sIndexNameToString(const std::vector<ZDBFieldName>& inNames);

	bool DoYouIndexOnTheseFieldsPrecisely(const std::vector<ZDBFieldName>& inFields);
	bool DoYouIndexOnThisFieldListPrefix(const std::vector<ZDBFieldName>& inFields);

	ZDBTable* GetTable() const { return fTable; }

	void WalkNodes(NodeWalker& inNodeWalker);
	long GetDegree();

private:
	ZDBIndex(ZDBTable* inTable, const std::vector<ZDBFieldName>& inIndexedFieldNames, long inDegreeOfNodeSpace);
	ZDBIndex(ZDBTable* inTable);
	~ZDBIndex();

	void DeleteYourself();

	bool AllNodesPresentAndCorrect(long& oFoundRecordCount, std::set<ZBlockStore::BlockID>* ioFoundRecordBlockIDs,
					std::set<ZDBRecord*, ZDBRecord::CompareForCheck>* ioFoundDBRecords, ZProgressWatcher* iProgressWatcher);
	bool WalkNodeAndCheck(ZBTree::Handle inHandle, std::set<ZBTree::Handle>& ioVisitedNodeHandles, long& ioFoundRecordCount, std::set<ZBlockStore::BlockID>* ioFoundRecordBlockIDs,
			std::set<ZDBRecord*, ZDBRecord::CompareForCheck>* ioFoundDBRecords, ZProgressWatcher* iProgressWatcher);

	void WalkNodeAndDelete(ZBTree::Handle inStartHandle);

	ZRWLock* GetLock() const;

	void RecordPreChange(ZDBRecord* inRecord, const ZDBFieldName& inFieldName);
	void RecordPostChange(ZDBRecord* inRecord, const ZDBFieldName& inFieldName);
	void RecordPreRead(ZDBRecord* inRecord);
	void RecordPostRead(ZDBRecord* inRecord);
	void AddedNewRecord(ZDBRecord* inRecord);
	void DeletedRecord(ZDBRecord* inRecord);

	void FromStream(const ZStreamR& inStream, long inVersion);
	void ToStream(const ZStreamW& inStream, long inVersion);

	void Flush();

	bool GetIndicesSpan();

	ZBlockStore* GetBlockStore();
	const std::vector<ZDBFieldSpec>& GetIndexedFieldSpecs() { return fIndexedFieldSpecs; }

	ZRef<ZDBRecord> LoadRecordFromBTreeItem(const ZDBTreeItem* inItem);
	void BuildFieldSpecVector();
	void ChangeFieldName(ZDBFieldName inOldName, ZDBFieldName inNewName);

	void InvalidateIterators();
	void Internal_LinkIterator(ZDBIndex::iterator* inIterator);
	void Internal_UnlinkIterator(ZDBIndex::iterator* inIterator);

	friend class ZDBTreeItem;
	friend class ZDBTable;
	friend class ZDBRecord;
	friend class ZDBSearch;
	friend class iterator;
	friend class ZDBTreeNodeSpace;
	class IntermediateNodeWalker;
	friend class IntermediateNodeWalker;

	ZDBTable* fTable;

	std::vector<ZDBFieldName> fIndexedFields;
	std::vector<ZDBFieldSpec> fIndexedFieldSpecs;

	iterator* fHeadIterator;
	ZMutexNR fIteratorMutex;

	ZBTree* fBTree;
	ZDBTreeNodeSpace* fNodeSpace;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex::NodeWalker

class ZDBIndex::NodeWalker
	{
public:
	virtual void NodeEntered(ZDBIndex* inIndex, const ZRef<ZDBTreeNode>& inNode) = 0;
	virtual void DumpItem(ZDBIndex* inIndex, ZDBTreeItem* inItem, const ZRef<ZDBRecord>& inRecord) = 0;
	virtual void NodeExited(ZDBIndex* inIndex, const ZRef<ZDBTreeNode>& inNode) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex::iterator

class ZDBIndex::iterator
	{
	iterator(ZDBIndex* inIndex, const std::vector<std::pair<ZBTree::Handle, long> >& inStack);
public:
	typedef ptrdiff_t difference_type;
	typedef size_t size_type;

	iterator();
	iterator(const ZDBIndex::iterator& inOther);
	~iterator();
	ZDBIndex::iterator& operator=(const ZDBIndex::iterator& inOther);

	bool operator==(const ZDBIndex::iterator& inOther) const;
	bool operator!=(const ZDBIndex::iterator& inOther) const;

	ZRef<ZDBRecord> operator*() const;
	ZDBIndex::iterator& operator++();
	ZDBIndex::iterator operator++(int);

	ZDBIndex::iterator& operator--();
	ZDBIndex::iterator operator--(int);

	ZDBIndex::iterator operator+(difference_type offset) const;
	ZDBIndex::iterator& operator+=(difference_type offset);

	ZDBIndex::iterator operator-(difference_type offset) const;
	ZDBIndex::iterator& operator-=(difference_type offset);

	ZDBIndex::iterator::difference_type operator-(const ZDBIndex::iterator& inOther) const;

	ZRef<ZDBRecord> operator[](difference_type index) const;

	ZDBIndex* GetIndex() const
		{ return fIndex; }
	void MoveBy(difference_type index)
		{ fCurrentRank += fIndex->fBTree->MoveBy(index, fStack); }

private:
	ZDBIndex::iterator::size_type GetRank() const;
	ZDBIndex* fIndex;
	std::vector<std::pair<ZBTree::Handle, long> > fStack;
	size_type fCurrentRank;
	bool fCurrentRankValid;
	ZDBIndex::iterator* fPrevIterator;
	ZDBIndex::iterator* fNextIterator;

	friend class ZDBIndex;
	friend class ZDBSearch;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex::ZDBSearch

class ZDBSearch
	{
public:
	enum SearchType { eLess, eLessEqual, eEqual, eGreaterEqual, eGreater, eAll };
	typedef size_t size_type;
	typedef ZDBIndex::iterator iterator;

private:
	ZDBSearch(ZDBIndex* inIndex, const std::vector<ZDBSearchSpec>& inSearchSpecs, SearchType inSearchType);
	ZDBSearch(ZDBIndex* inIndex, const ZDBSearchSpec& inSearchSpec, SearchType inSearchType);
	ZDBSearch(ZDBIndex* inIndex);

public:
	ZDBSearch();
	ZDBSearch(const ZDBSearch& inOther);
	ZDBSearch& operator=(const ZDBSearch& inOther);
	~ZDBSearch();

	ZRef<ZDBRecord> operator[](size_type inIndex) const;
	size_type size() const;

	bool empty() const;

	const ZDBIndex::iterator& begin() const;
	const ZDBIndex::iterator& end() const;

	ZRef<ZDBRecord> front() const;
	ZRef<ZDBRecord> back() const;

	void DeleteFoundSet();
	void GetFoundSetIDs(std::vector<ZDBRecordID>& outIDVector);

	ZDBIndex* GetIndex() const
		{ return fIndex; }

private:
	void Internal_SetupStack_FirstEqual(std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;
	void Internal_SetupStack_FirstGreater(std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;
	void Internal_SetupStack_FarLeft(std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;
	void Internal_SetupStack_FarRight(std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;
	void Internal_SetupStack_Rank(size_type inRank, std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;
	void Internal_CleanUpStack(std::vector<std::pair<ZBTree::Handle, long> >& outStack) const;

	ZDBIndex* fIndex;
	SearchType fSearchType;
	ZDBTreeItem* fSearchItem;
	ZDBIndex::iterator fCachedBeginIterator;
	ZDBIndex::iterator fCachedEndIterator;

	friend class ZDBIndex;
	friend class ZDBTable;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTable

class ZDBTable
	{
protected:
	ZDBTable(ZDBDatabase* inDatabase, ZBlockStore::BlockID inBlockID);
	ZDBTable(ZDBDatabase* inDatabase, const std::string& inTableName,
					const std::vector<ZDBFieldSpec>& inFieldSpecList, bool inFullySpanningIndices, bool inCreateIDs);

	virtual ~ZDBTable();

public:
	bool Validate(ZProgressWatcher* iProgressWatcher);
	void CopyInto(ZProgressWatcher* iProgressWatcher, ZDBTable* iTable);

	long GetVersion() const
		{ return fVersion; }

	void CreateIndex(ZDBFieldName inFieldName, ZProgressWatcher* iProgressWatcher);
	void CreateIndex(const std::vector<ZDBFieldName>& inFieldsToIndex, ZProgressWatcher* iProgressWatcher);
	void CreateIndices(const std::vector<std::vector<ZDBFieldName> >& inIndexSpecs, ZProgressWatcher* iProgressWatcher);

	ZDBIndex* GetIndex(ZDBFieldName inFieldName);
	ZDBIndex* GetIndex(const std::vector<ZDBFieldName>& inFieldNames);

	void DeleteIndex(ZDBIndex* inIndex);

	ZDBFieldRep::FieldType GetFieldType(const ZDBFieldName& inName);
	ZDBFieldRep* CreateFieldRepByType(ZDBFieldRep::FieldType inType);
	ZDBFieldRep* CreateFieldRepByName(const ZDBFieldName& inName);
	ZDBFieldRep* CreateFieldRep(const ZDBFieldName& inName, ZDBFieldRep::FieldType inType);

	void MakeSpace();

	ZBlockStore::BlockID GetHeaderBlockID();

	ZBlockStore* GetBlockStore();

	ZRef<ZDBRecord> CreateRecord(bool inCreatePulledFromIndices = false);
	void DeleteRecord(ZRef<ZDBRecord> inRecord);

	ZRef<ZDBRecord> GetRecordByID(const ZDBRecordID& inID);
	bool FixNextRecordID();

	ZDBSearch Search(const ZDBSearchSpec& inSearchSpec, ZDBSearch::SearchType inSearchType = ZDBSearch::eEqual);
	ZDBSearch Search(const std::vector<ZDBSearchSpec>& inSearchSpecs, ZDBSearch::SearchType inSearchType = ZDBSearch::eEqual);
	
// Four ways to specify search, while specifying constraint on index to be used.
	ZDBSearch Search(const ZDBSearchSpec& inSearchSpec, const ZDBFieldName& inFieldName);
	ZDBSearch Search(const ZDBSearchSpec& inSearchSpec, const std::vector<ZDBFieldName>& inFieldNames);
	ZDBSearch Search(const std::vector<ZDBSearchSpec>& inSearchSpecs, const ZDBFieldName& inFieldName);
	ZDBSearch Search(const std::vector<ZDBSearchSpec>& inSearchSpecs, const std::vector<ZDBFieldName>& inFieldNames);

// Four different ways to walk all fields
	ZDBSearch SearchAll();
	ZDBSearch SearchAll(const ZDBFieldName& inName);
	ZDBSearch SearchAll(const std::vector<ZDBFieldName>& inFieldNames);
	ZDBSearch SearchAll(ZDBIndex* inIndexToUse);

	long GetRecordCount() const;

	const std::string& GetTableName() const { return fTableName; }
	void SetTableName(const std::string& newName);

	virtual void Flush();

	ZRWLock* GetLock();

	const std::vector<ZDBFieldSpec>& GetFieldSpecVector() const
		{ return fFieldSpecList; }

	void ChangeFieldName(ZDBFieldName oldName, ZDBFieldName newName);

// ----------
// These methods are intended for a browser app to get access to internal info.
// They're really not needed for most operations.
	void GetIndexVector(std::vector<ZDBIndex*>& outIndexVector) const;
	void GetIndexSpecVector(std::vector<std::vector<ZDBFieldName> >& oIndexSpecs);

	bool GetCreatingRecordIDs() const;
	bool GetIndicesSpan() const;
	ZDBRecordID GetNextRecordID() const;

// ----------
protected:
	virtual ZDBRecord* MakeRecord(const ZDBTreeItem* inItem, bool inPulledFromIndices = false);
	virtual ZDBRecord* MakeRecord(ZBlockStore::BlockID inBlockID, const std::vector<ZDBFieldSpec>& inFieldSpecList,
													const ZDBRecordID& inRecordID, bool inCreatePulledFromIndices);
	virtual ZDBRecord* MakeRecord(ZBlockStore::BlockID inBlockID, const std::vector<ZDBFieldSpec>& inFieldSpecList);

	virtual ZDBIndex* MakeIndex(const std::vector<ZDBFieldName>& inFieldsToIndex, long inDegreeOfNodeSpace);
	virtual ZDBIndex* MakeIndex();

private:

	bool CheckRecordLoadable(ZBlockStore::BlockID inBlockID);

	ZRef<ZDBRecord> LoadRecord(ZBlockStore::BlockID inBlockID);

	void RecordBeingFinalized(ZDBRecord* inRecord);

	void RecordPreChange(ZDBRecord* inRecord, ZDBField* inField);
	void RecordPostChange(ZDBRecord* inRecord, ZDBField* inField);
	void RecordPreRead(ZDBRecord* inRecord);
	void RecordPostRead(ZDBRecord* inRecord);

	ZRef<ZDBRecord> LoadRecordFromBTreeItem(const ZDBTreeItem* inItem);

	virtual void LoadUp();
	virtual void WriteBack();

	void DeleteYourself();

	friend class ZDBRecord;
	friend class ZDBIndex;
	friend class ZDBDatabase;

private:
	ZMutex fLoadMutex;
	ZBlockStore::BlockID fHeaderBlockID;

	long fRecordCount;
	ZDBRecordID fNextRecordID;

	std::string fTableName;

	long fVersion;

	bool fDirty;

	std::vector<ZDBRecord*> fLoadedRecords;
	std::vector<ZDBFieldSpec> fFieldSpecList;
	std::vector<ZDBIndex*> fIndexList;

	bool fFullySpanningIndices;

	ZDBDatabase* fDatabase;

	ZRWLock fLock;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeNode

class ZDBTreeNode : public ZBTree::Node
	{
	ZDBTreeNode(); // Not implemented
	ZDBTreeNode(const ZDBTreeNode& inOther); // Not implemented
	ZDBTreeNode& operator=(const ZDBTreeNode& inOther); // Not implemented

// Declaring these private ensures only ZDBTreeNodeSpace can access us
	ZDBTreeNode(ZDBTreeNodeSpace* inNodeSpace);
	ZDBTreeNode(ZDBTreeNodeSpace* inNodeSpace, ZBlockStore::BlockID inBlockID);

protected:
	virtual void ContentChanged();
	void WriteBack();

	bool IsDirty() { return fDirty; }

private:
	bool fDirty;

	// Used by ZDBTreeNodeSpace to maintain the order in which to eject
	// nodes from the cache.
	ZDBTreeNode* fPrev;
	ZDBTreeNode* fNext;

	friend class ZDBTreeNodeSpace;
	friend class ZDBIndex;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeNodeSpace

class ZDBTreeNodeSpace : public ZBTree::NodeSpace
	{
public:
	ZDBTreeNodeSpace(long inDegree, ZDBIndex* inIndex, long inVersion);
	ZDBTreeNodeSpace(long inDegree, ZDBIndex* inIndex, ZBTree::Handle inBlockID, long inVersion);

// From ZBTree::NodeSpace
	virtual ~ZDBTreeNodeSpace();

	virtual ZRef<ZBTree::Node> LoadNode(ZBTree::Handle inBlockID);
	virtual ZRef<ZBTree::Node> CreateNode();
	virtual void DeleteNode(ZRef<ZBTree::Node> inNode);

	virtual void NodeBeingFinalized(ZBTree::Node* inNode);

	virtual long GetNodeSubTreeSize(ZBTree::Handle inBlockID);
	virtual void SetNewRoot(ZBTree::Handle inBlockID);

// Our protocol
	ZBlockStore* GetBlockStore() { return fIndex->GetBlockStore(); }
	void Flush();
	ZDBIndex* GetIndex() { return fIndex; }

	void GrabRootNode();

	long GetVersion() const { return fVersion; }

private:
	ZDBIndex* fIndex;
	std::map<ZBTree::Handle, ZDBTreeNode*> fLoadedNodes;
	std::map<ZBTree::Handle, ZDBTreeNode*> fCachedNodes;
	ZDBTreeNode* fCached_Head;
	ZDBTreeNode* fCached_Tail;
	ZRef<ZBTree::Node> fRootNode;
	long fVersion;
	ZMutex fMutex;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeItem

class ZDBTreeItem : public ZBTree::Item
	{
	ZDBTreeItem(); // Not implemented

public:
	ZDBTreeItem(const ZDBTreeItem& inOther);
	ZDBTreeItem(ZDBIndex* inIndex, const std::vector<ZDBSearchSpec>& inSearchSpec);
	ZDBTreeItem(ZDBIndex* inIndex, const ZDBSearchSpec& inSearchSpec);
	ZDBTreeItem(ZDBIndex* inIndex, ZDBRecord* inRecord);
	ZDBTreeItem(ZDBIndex* inIndex, ZStreamR& inStream);

	virtual ~ZDBTreeItem();

	virtual long Compare(const ZBTree::Item* inOther);
	virtual long CompareForSearch(const ZBTree::Item* inOther, bool strictComparison);
	long CompareForIteration(const ZDBTreeItem* inOther);

	ZBlockStore::BlockID GetRecordBlockID() const { return fRecordBlockID; }

	ZDBTreeItem* Clone() const;

	ZDBIndex* GetIndex() const { return fIndex; }
	const std::vector<ZDBFieldRep*>& GetFieldReps() const { return fFieldReps; }


	void ToStream(const ZStreamW& inStream);

private:
	std::vector<ZDBFieldRep*> fFieldReps;
	ZBlockStore::BlockID fRecordBlockID;
	ZDBIndex* fIndex;
	};

// =================================================================================================
// Some typedefs for code that uses the old names
#if 1
typedef ZDBDatabase DBDatabase;
typedef ZDBRecordID DBRecordID;
typedef ZDBSearch DBSearch;
typedef ZDBSearchSpec DBSearchSpec;
typedef ZDBFieldSpec DBFieldSpec;
typedef ZDBFieldName DBFieldName;
typedef ZDBField DBField;
typedef ZDBFieldRep DBFieldRep;
typedef ZDBFieldRepBinary DBFieldRepBinary;
typedef ZDBFieldRepID DBFieldRepID;
typedef ZDBFieldRepNumber DBFieldRepNumber;
typedef ZDBFieldRepString DBFieldRepString;
typedef ZDBFieldRepIString DBFieldRepIString;
typedef ZDBFieldRepBLOB DBFieldRepBLOB;
typedef ZDBRecord DBRecord;
typedef ZDBTable DBTable;
typedef ZDBIndex DBIndex;
#endif

#endif // __ZDBase__
