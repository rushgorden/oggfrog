static const char rcsid[] = "@(#) $Id: ZDBase.cpp,v 1.36 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZDBase.h"

#include "ZByteSwap.h"
#include "ZMemory.h"
#include "ZProgressWatcher.h"
#include "ZStream.h"
#include "ZStream_Buffered.h"
#include "ZString.h"
#include "ZUnicode.h" // for sToUpper(). -ec 02.05.28
#include "ZUtil_STL.h"

#include "ZCompat_algorithm.h"
#include <cctype> // for toupper()
#include <cstdio> // for sprintf
#include <list>

using std::bad_alloc;
using std::exception;
using std::make_pair;
using std::map;
using std::pair;
using std::range_error;
using std::runtime_error;
using std::set;
using std::string;
using std::vector;

#define kDebug_ZDBase 0

// =================================================================================================
#pragma mark -
#pragma mark * ZDBRecordID

string ZDBRecordID::AsString() const
	{
	// Format as a hex number for now
	char theString[19];
	char* theStringPtr = &theString[0];

	uint32 localHi = uint32(fValue >> 32);
	if (localHi != 0)
		{
		for (long x = 28; x >= 0; x -= 4)
			{
			char theChar = ((localHi >> x) & 0xF)+ '0';
			if (theChar > '9')
				theChar += 'A' - '9' - 1;
			*theStringPtr++ = theChar;
			}
		*theStringPtr++ = ':';
		}

	uint32 localLo = uint32(fValue);
	for (long x = (localHi > 0 || localLo > 0xFFFF) ? 28 : 12; x >= 0; x -= 4)
		{
		char theChar = ((localLo >> x) & 0xF)+ '0';
		if (theChar > '9')
			theChar += 'A' - '9' - 1;
		*theStringPtr++ = theChar;
		}

	return string(theString, theStringPtr - &theString[0]);
	}

string ZDBRecordID::AsString10() const
	{
	char theString[21];
	sprintf(theString,"%lld",fValue);
	return string(theString);
	}

ZDBRecordID ZDBRecordID::sFromString(const string& iString)
	{
	int64 theInt;
	if (::sscanf(iString.c_str(), "%lld", &theInt) <= 0)
		theInt = 0;
	return theInt;
	}

ZDBRecordID ZDBRecordID::sFromString(const char* iString)
	{
	int64 theInt;
	if (::sscanf(iString, "%lld", &theInt) <= 0)
		theInt = 0;
	return theInt;
	}

ZDBRecordID ZDBRecordID::sFromStringHex(const string& iString)
	{
	int64 theInt;
	if (::sscanf(iString.c_str(), "%llx", &theInt) <= 0)
		theInt = 0;
	return theInt;
	}

ZDBRecordID ZDBRecordID::sFromStringHex(const char* iString)
	{
	int64 theInt;
	if (::sscanf(iString, "%llx", &theInt) <= 0)
		theInt = 0;
	return theInt;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldName

ZDBFieldName::ZDBFieldName(const char* inName)
	{
	ZAssert(::strlen(inName) == 4);
	fName = *reinterpret_cast<const uint32*>(inName);
	::ZByteSwap_BigToHost32(&fName);
	}

void ZDBFieldName::FromStream(const ZStreamR& inStream)
	{ fName = inStream.ReadUInt32(); }

void ZDBFieldName::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteUInt32(fName); }

string ZDBFieldName::AsString() const
	{
	int32 tempName = fName;
	::ZByteSwap_HostToBig32(&tempName);
	string aString((char*)&tempName, 4);
	return aString;
	}

const ZDBFieldName ZDBFieldName::ID(0x5F5F4944); // '__ID'

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldRep

string ZDBFieldRep::AsString() const
	{ return "** No String Representation Available **"; }

ZDBFieldRep* ZDBFieldRep::TotalClone(ZBlockStore* inSourceBlockStore, ZBlockStore* inDestBlockStore) const
	{ return this->Clone(); }

void ZDBFieldRep::DeleteExtraStorage(ZBlockStore* inBlockStore)
	{}

bool ZDBFieldRep::CreateExtraStorage(ZBlockStore* inBlockStore)
	{ return false; }

bool ZDBFieldRep::CheckExtraStorageOkay(ZBlockStore* inBlockStore)
	{ return true; }

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepBLOB

ZDBFieldRepBLOB::~ZDBFieldRepBLOB()
	{}

void ZDBFieldRepBLOB::FromStream(const ZStreamR& inStream)
	{ fBLOBHandle = inStream.ReadUInt32(); }

void ZDBFieldRepBLOB::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteUInt32(fBLOBHandle); }

ZDBFieldRep::FieldType ZDBFieldRepBLOB::GetFieldType() const
	{ return eBLOB; }

long ZDBFieldRepBLOB::Compare(const ZDBFieldRep* inOther) const
	{
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	// Sorted BLOBs are ordered by the BLOB handle
	if (fBLOBHandle < ((ZDBFieldRepBLOB*)inOther)->fBLOBHandle)
		return -1;
	else if (fBLOBHandle > ((ZDBFieldRepBLOB*)inOther)->fBLOBHandle)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepBLOB::Clone() const
	{ return new ZDBFieldRepBLOB(fBLOBHandle); }

ZDBFieldRep* ZDBFieldRepBLOB::TotalClone(ZBlockStore* inSourceBlockStore, ZBlockStore* inDestBlockStore) const
	{
	// The total clone of a BLOB also clones the contents of our block
	ZBlockStore::BlockID newBlockID;
	try
		{
		ZRef<ZStreamerW> newStreamer = inDestBlockStore->Create(newBlockID);
		if (ZRef<ZStreamerR> sourceStreamer = inSourceBlockStore->OpenR(fBLOBHandle))
			newStreamer->GetStreamW().CopyAllFrom(sourceStreamer->GetStreamR());
		}
	catch (...)
		{
		ZDebugLogf(1, ("ZDBFieldRepBLOB::TotalClone, couldn't read original block"));
		}
	return new ZDBFieldRepBLOB(newBlockID);
	}

void ZDBFieldRepBLOB::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType())
		{
		case eBLOB:
			{
			ZDBFieldRep* newRep = inOther->TotalClone(inOtherBlockStore, inThisBlockStore);
			fBLOBHandle = ((ZDBFieldRepBLOB*)newRep)->fBLOBHandle;
			break;
			}
		default:
			ZUnimplemented();
		}
	}

void ZDBFieldRepBLOB::DeleteExtraStorage(ZBlockStore* inBlockStore)
	{
	if (fBLOBHandle)
		inBlockStore->Delete(fBLOBHandle);
	}

bool ZDBFieldRepBLOB::CreateExtraStorage(ZBlockStore* inBlockStore)
	{
	ZAssert(!fBLOBHandle);
	inBlockStore->Create(fBLOBHandle);
	return true;
	}

bool ZDBFieldRepBLOB::CheckExtraStorageOkay(ZBlockStore* inBlockStore)
	{
	return inBlockStore->OpenR(fBLOBHandle);
	}

string ZDBFieldRepBLOB::AsString() const
	{
	string theString;
	theString += "0x"; // nice prefix.
	theString += ZString::sHexFromInt(fBLOBHandle);
	return theString;
	}

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepNumber

ZDBFieldRepNumber::~ZDBFieldRepNumber()
	{}

void ZDBFieldRepNumber::FromStream(const ZStreamR& inStream)
	{ fNumber = inStream.ReadInt32(); }

void ZDBFieldRepNumber::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteInt32(fNumber); }

ZDBFieldRep::FieldType ZDBFieldRepNumber::GetFieldType() const
	{ return eNumber; }

long ZDBFieldRepNumber::Compare(const ZDBFieldRep* inOther) const
	{
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	if (fNumber < ((ZDBFieldRepNumber*)inOther)->fNumber)
		return -1;
	else if (fNumber > ((ZDBFieldRepNumber*)inOther)->fNumber)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepNumber::Clone() const
	{ return new ZDBFieldRepNumber(fNumber); }

void ZDBFieldRepNumber::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType()) 
		{
		case eNumber:
			fNumber = ((ZDBFieldRepNumber*)inOther)->fNumber;
			break;
		case eID:
			fNumber = ((ZDBFieldRepID*)inOther)->GetValue().GetValue();
			break;
		case eString:
		case eIString:
			fNumber = ZString::sAsInt(((ZDBFieldRepStringBase*)inOther)->GetString());
			break;
		default:
			ZUnimplemented();
		}
	}

string ZDBFieldRepNumber::AsString() const
	{ return ZString::sFromInt(fNumber); }

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepID

ZDBFieldRepID::~ZDBFieldRepID()
	{}

void ZDBFieldRepID::FromStream(const ZStreamR& inStream)
	{ fValue.FromStream(inStream); }

void ZDBFieldRepID::ToStream(const ZStreamW& inStream) const
	{ fValue.ToStream(inStream); }

ZDBFieldRep::FieldType ZDBFieldRepID::GetFieldType() const
	{ return eID; }

long ZDBFieldRepID::Compare(const ZDBFieldRep* inOther) const
	{
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	const ZDBFieldRepID* realOther = (ZDBFieldRepID*)inOther;
	if (fValue < realOther->fValue)
		return -1;
	else if (fValue > realOther->fValue)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepID::Clone() const
	{ return new ZDBFieldRepID(fValue); }

void ZDBFieldRepID::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch(inOther->GetFieldType())
		{
		case eID:
			fValue = ((ZDBFieldRepID*)inOther)->fValue;
			break;
		case eNumber:
			fValue = ((ZDBFieldRepNumber*)inOther)->GetNumber();
			break;
		case eString:
		case eIString:
			fValue = atoi(((ZDBFieldRepStringBase*)inOther)->GetString().c_str());
			break;
		default:
			ZUnimplemented();
		}
	}

string ZDBFieldRepID::AsString() const
	{ return fValue.AsString(); }

void ZDBFieldRepID::SetValue(const ZDBRecordID& inNewValue)
	{ fValue = inNewValue; }

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepStringBase

ZDBFieldRepStringBase::ZDBFieldRepStringBase(const string& inString)
:	fString(inString)
	{}

ZDBFieldRepStringBase::~ZDBFieldRepStringBase()
	{}

void ZDBFieldRepStringBase::FromStream(const ZStreamR& inStream)
	{ ZString::sFromStream(fString, inStream); }

void ZDBFieldRepStringBase::ToStream(const ZStreamW& inStream) const
	{ ZString::sToStream(fString, inStream); }

string ZDBFieldRepStringBase::AsString() const
	{ return fString; }

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepString

ZDBFieldRepString::ZDBFieldRepString(const string& inString)
:	ZDBFieldRepStringBase(inString)
	{}

ZDBFieldRepString::~ZDBFieldRepString()
	{}

ZDBFieldRep* ZDBFieldRepString::Clone() const
	{ return new ZDBFieldRepString(fString); }

ZDBSearchSpec& ZDBSearchSpec::operator=(const ZDBSearchSpec& other)
	{
	if (this != &other)
		{
		delete fFieldRep;
		if (other.fFieldRep)
			fFieldRep = other.fFieldRep->Clone();
		else
			fFieldRep = nil;
		fFieldName = other.fFieldName;
		}
	return *this;
	}

ZDBFieldRep::FieldType ZDBFieldRepString::GetFieldType() const
	{ return eString; }

void ZDBFieldRepString::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType())
		{
		case eString:
			fString = ((ZDBFieldRepString*)inOther)->fString;
			break;
		case eIString:
			fString = ((ZDBFieldRepIString*)inOther)->GetString();
			break;
		case eID:
			{
			fString = ((ZDBFieldRepID*)inOther)->GetValue().AsString10();
			break;
			}
		case eNumber:
			{
			fString = ZString::sFromInt(((ZDBFieldRepNumber*)inOther)->GetNumber());
			break;
			}
		default:
			ZUnimplemented();
		}
	}

long ZDBFieldRepString::Compare(const ZDBFieldRep* other) const
	{
	ZAssert(other->GetFieldType() == this->GetFieldType());
	return fString.compare(((ZDBFieldRepString*)other)->fString);
	}

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepIString

ZDBFieldRepIString::ZDBFieldRepIString(const string& theString)
:	ZDBFieldRepStringBase(theString)
	{}

ZDBFieldRep::FieldType ZDBFieldRepIString::GetFieldType() const
	{ return eIString; }

ZDBFieldRepIString::~ZDBFieldRepIString()
	{}

ZDBFieldRep* ZDBFieldRepIString::Clone() const
	{ return new ZDBFieldRepIString(fString); }

void ZDBFieldRepIString::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore) 
	{
	switch (inOther->GetFieldType())
		{
		case eIString:
			fString = ((ZDBFieldRepIString*)inOther)->fString;
			break;
		case eString:
			fString = ((ZDBFieldRepString*)inOther)->GetString();
			break;
		case eID:
			{
			fString = ((ZDBFieldRepID*)inOther)->GetValue().AsString10();
			break;
			}
		case eNumber:
			{
			fString = ZString::sFromInt(((ZDBFieldRepNumber*)inOther)->GetNumber());
			break;
			}
		default:
			ZUnimplemented();
		}
	}

long ZDBFieldRepIString::Compare(const ZDBFieldRep* other) const
	{
	ZAssert(other->GetFieldType() == eIString || other->GetFieldType() == eString);

	for (long count = 0;
				count < fString.length() && count < ((ZDBFieldRepStringBase*)other)->GetString().length();
				++count)
		{
		if (ZUnicode::sToUpper(fString[count]) < ZUnicode::sToUpper(((ZDBFieldRepStringBase*)other)->GetString()[count]))
			return -1;
		else if (ZUnicode::sToUpper(fString[count]) > ZUnicode::sToUpper(((ZDBFieldRepStringBase*)other)->GetString()[count]))
			return 1;
		}
	if (fString.length() < ((ZDBFieldRepStringBase*)other)->GetString().length())
		return -1;
	else if (fString.length() > ((ZDBFieldRepStringBase*)other)->GetString().length())
		return 1;
	return 0;
	}

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepBinary

ZDBFieldRepBinary::ZDBFieldRepBinary(const void* inData, size_t inLength)
	{
	fData = nil;
	fLength = inLength;
	if (fLength)
		{
		fData = new char[fLength];
		ZBlockMove(inData, fData, inLength);
		}
	}

ZDBFieldRepBinary::ZDBFieldRepBinary(const ZStreamR& iStream, size_t iLength)
	{
	fData = nil;
	fLength = iLength;
	if (fLength)
		{
		fData = new char[fLength];
		iStream.Read(fData, fLength);
		}
	}

ZDBFieldRepBinary::~ZDBFieldRepBinary()
	{
	// If we've got a length, we've got to have data, and vice versa
	ZAssert((fLength == 0 && fData == nil) || (fLength != 0 && fData != nil));
	if (fData)
		delete[] fData;
	}

void ZDBFieldRepBinary::FromStream(const ZStreamR& inStream)
	{
	// Reallocate only if we're changing sizes
	size_t newLength = inStream.ReadUInt32();
	if (newLength > 0x100000)
		throw runtime_error("ZDBFieldRepBinary::FromStream, invalid size");
	if (fLength != newLength)
		{
		if (fData)
			{
			delete[] fData;
			fData = nil;
			fLength = 0;
			}
		if (newLength)
			fData = new char[newLength];
		fLength = newLength;
		}
	if (fLength)
		inStream.Read(fData, fLength);
	}

void ZDBFieldRepBinary::ToStream(const ZStreamW& inStream) const
	{
	inStream.WriteUInt32(fLength);
	if (fLength)
		inStream.Write(fData, fLength);
	}

ZDBFieldRep::FieldType ZDBFieldRepBinary::GetFieldType() const
	{ return eBinary; }

long ZDBFieldRepBinary::Compare(const ZDBFieldRep* inOther) const
	{
	// We sort binary data based on the character by character values
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	char* ourData = fData;
	char* otherData = ((ZDBFieldRepBinary*)inOther)->fData;
	for (long count = 0;
				count < fLength && count < ((ZDBFieldRepBinary*)inOther)->fLength;
				++count, ++ourData, ++otherData)
		{
		if (*ourData < *otherData)
			return -1;
		else if (*ourData > *otherData)
			return 1;
		}
	if (fLength < ((ZDBFieldRepBinary*)inOther)->fLength)
		return -1;
	else if (fLength > ((ZDBFieldRepBinary*)inOther)->fLength)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepBinary::Clone() const
	{ return new ZDBFieldRepBinary(fData, fLength); }

void ZDBFieldRepBinary::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType())
		{
		case eBinary:
			fData = ((ZDBFieldRepBinary*)inOther)->fData;
			fLength = ((ZDBFieldRepBinary*)inOther)->fLength;
			break;
		default:
			ZUnimplemented();
		}
	}

string ZDBFieldRepBinary::AsString() const
	{ return string("Binary: ") + ZString::sFromInt(fLength)+" bytes"; }

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepFloat

ZDBFieldRepFloat::~ZDBFieldRepFloat()
	{}

void ZDBFieldRepFloat::FromStream(const ZStreamR& inStream)
	{ fFloat = inStream.ReadFloat(); }

void ZDBFieldRepFloat::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteFloat(fFloat); }

ZDBFieldRep::FieldType ZDBFieldRepFloat::GetFieldType() const
	{ return eFloat; }

long ZDBFieldRepFloat::Compare(const ZDBFieldRep* inOther) const
	{
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	if (fFloat < ((ZDBFieldRepFloat*)inOther)->fFloat)
		return -1;
	else if (fFloat > ((ZDBFieldRepFloat*)inOther)->fFloat)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepFloat::Clone() const
	{ return new ZDBFieldRepFloat(fFloat); }

void ZDBFieldRepFloat::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType()) 
		{
		case eFloat:
			fFloat = ((ZDBFieldRepFloat*)inOther)->fFloat;
		case eNumber:
			fFloat = (float)((ZDBFieldRepNumber*)inOther)->GetNumber();
			break;
		default:
			ZUnimplemented();
		}
	}

// ==================================================
#pragma mark -
#pragma mark * ZDBFieldRepDouble

ZDBFieldRepDouble::~ZDBFieldRepDouble()
	{}

void ZDBFieldRepDouble::FromStream(const ZStreamR& inStream)
	{ fDouble = inStream.ReadDouble(); }

void ZDBFieldRepDouble::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteDouble(fDouble); }

ZDBFieldRep::FieldType ZDBFieldRepDouble::GetFieldType() const
	{ return eDouble; }

long ZDBFieldRepDouble::Compare(const ZDBFieldRep* inOther) const
	{
	ZAssert(inOther->GetFieldType() == this->GetFieldType());
	if (fDouble < ((ZDBFieldRepDouble*)inOther)->fDouble)
		return -1;
	else if (fDouble > ((ZDBFieldRepDouble*)inOther)->fDouble)
		return 1;
	return 0;
	}

ZDBFieldRep* ZDBFieldRepDouble::Clone() const
	{ return new ZDBFieldRepDouble(fDouble); }

void ZDBFieldRepDouble::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBFieldRep* inOther, ZBlockStore* inOtherBlockStore)
	{
	switch (inOther->GetFieldType()) 
		{
		case eFloat:
			fDouble = (double)((ZDBFieldRepFloat*)inOther)->GetFloat();
		case eDouble:
			fDouble = ((ZDBFieldRepDouble*)inOther)->fDouble;
		case eNumber:
			fDouble = (double)((ZDBFieldRepNumber*)inOther)->GetNumber();
			break;
		default:
			ZUnimplemented();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBField

ZDBField::ZDBField(ZDBRecord* inOwnerRecord, const ZDBFieldName& inFieldName, ZDBFieldRep* inRep)
:	fRecord(inOwnerRecord), fFieldName(inFieldName), fFieldRep(inRep)
	{}

ZDBField::~ZDBField()
	{ delete fFieldRep; }

void ZDBField::FromStream(const ZStreamR& inStream)
	{ fFieldRep->FromStream(inStream); }

void ZDBField::ToStream(const ZStreamW& inStream) const
	{ fFieldRep->ToStream(inStream); }

void ZDBField::DeleteExtraStorage(ZBlockStore* inBlockStore)
	{
	fFieldRep->DeleteExtraStorage(inBlockStore);
	// Most fields do nothing
	}

bool ZDBField::CreateExtraStorage(ZBlockStore* inBlockStore)
	{ return fFieldRep->CreateExtraStorage(inBlockStore); }

bool ZDBField::CheckExtraStorageOkay(ZBlockStore* inBlockStore)
	{
	return fFieldRep->CheckExtraStorageOkay(inBlockStore);
	// Most fields do nothing
	}

void ZDBField::CopyFrom(ZBlockStore* inThisBlockStore, const ZDBField* inOther, ZBlockStore* inOtherBlockStore)
	{
	// We can copy data from another field if the write lock is current
	ZAssert(fRecord->CanBeWritten());

	fRecord->FieldPreChange(this);
	ZDBFieldRep* newRep;
	if (inOther->GetRep()->GetFieldType() == fFieldRep->GetFieldType())
		{
		newRep = inOther->GetRep()->TotalClone(inOtherBlockStore, inThisBlockStore);
		}
	else
		{
		newRep = fRecord->fTable->CreateFieldRepByType(fFieldRep->GetFieldType());
		newRep->CopyFrom(inThisBlockStore, inOther->GetRep(), inOtherBlockStore);
		}
	delete fFieldRep;
	fFieldRep = newRep;
	fRecord->FieldPostChange(this);
	}

ZDBFieldRep::FieldType ZDBField::GetFieldType() const
	{ return fFieldRep->GetFieldType(); }

void ZDBField::SetRep(ZDBFieldRep* inRep)
	{
	// We include a hard-coded check here to make sure no one tries to *change*
	// a record's auto-ID field
	ZAssert(fFieldName != ZDBFieldName::ID);

	// We can have our rep changed if the write lock is current, and there is no
	// extant read lock
	ZAssert(fRecord->CanBeWritten());

	// And of course the new rep has to have the same type as the existing one
	ZAssert(inRep->GetFieldType() == fFieldRep->GetFieldType());

	fRecord->FieldPreChange(this);
	delete fFieldRep;
	fFieldRep = inRep;
	fRecord->FieldPostChange(this);
	}

void ZDBField::SetRep(const string& inString)
	{ 
	if (fFieldRep->GetFieldType() == ZDBFieldRep::eString)
		this->SetRep(new ZDBFieldRepString(inString));
	else if (fFieldRep->GetFieldType() == ZDBFieldRep::eIString)
		this->SetRep(new ZDBFieldRepIString(inString));
	}

void ZDBField::SetRep(const long inNumber)
	{ this->SetRep(new ZDBFieldRepNumber(inNumber)); }

void ZDBField::SetRep(const ZDBRecordID& inID)
	{ this->SetRep(new ZDBFieldRepID(inID)); }

void ZDBField::SetRepFloat(const float iFloat)
	{ this->SetRep(new ZDBFieldRepFloat(iFloat)); }

void ZDBField::SetRepDouble(const double iDouble)
	{ this->SetRep(new ZDBFieldRepDouble(iDouble)); }

const ZDBFieldRep* ZDBField::GetRep() const
	{
	ZAssert(fRecord->CanBeRead());
	return fFieldRep;
	}

ZDBFieldRep* ZDBField::GetClonedRep() const
	{ return fFieldRep->Clone(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZDBFieldSpec

void ZDBFieldSpec::FromStream(const ZStreamR& inStream)
	{
	fFieldName.FromStream(inStream);
	fFieldType = (ZDBFieldRep::FieldType) inStream.ReadInt32();
	}

void ZDBFieldSpec::ToStream(const ZStreamW& inStream) const
	{
	fFieldName.ToStream(inStream);
	inStream.WriteInt32(fFieldType);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBSearchSpec

ZDBSearchSpec::ZDBSearchSpec(const ZDBSearchSpec& inOther)
:	fFieldName(inOther.fFieldName), fFieldRep(inOther.fFieldRep? inOther.fFieldRep->Clone() : nil)
	{}
ZDBSearchSpec::~ZDBSearchSpec()
	{ 
	if (fFieldRep)
		delete fFieldRep;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBDatabase

ZDBDatabase::ZDBDatabase(ZBlockStore* inBlockStore)
:	fBlockStore(inBlockStore)
	{
	fRefCon = 0;
	fDirty = true;
	// We're creating a new database (a collection of named tables) so we need
	// a block from the blockfile in which to store our header data
	fBlockStore->Create(fHeaderBlockID);

	this->WriteBack();
	}

ZDBDatabase::ZDBDatabase(ZBlockStore* inBlockStore, ZBlockStore::BlockID inBlockID)
:	fBlockStore(inBlockStore), fHeaderBlockID(inBlockID)
	{
	fDirty = false;
	this->LoadUp();
	}

ZDBDatabase::~ZDBDatabase()
	{
	this->Flush();
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		delete *i;
	}

long ZDBDatabase::GetRefCon()
	{ return fRefCon; }

void ZDBDatabase::SetRefCon(long inRefCon)
	{
	if (fRefCon != inRefCon)
		{
		fRefCon = inRefCon;
		fDirty = true;
		}
	}

ZDBTable* ZDBDatabase::CreateTable(const string& inName, const vector<ZDBFieldSpec>& inFieldSpecList, bool inFullySpanningIndices, bool inCreateIDs)
	{
	fDirty = true;
	ZDBTable* theTable = this->MakeTable(inName, inFieldSpecList, inFullySpanningIndices, inCreateIDs);
	fTables.push_back(theTable);
	return theTable;
	}

void ZDBDatabase::DeleteTable(ZDBTable* inTable)
	{
	vector<ZDBTable*>::iterator foundIter = fTables.end();
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		{
		if (*i == inTable)
			{
			foundIter = i;
			break;
			}
		}
	ZAssert(foundIter != fTables.end());
	inTable->DeleteYourself();
	fTables.erase(foundIter);
	}

ZDBTable* ZDBDatabase::GetTable(const string& inName)
	{
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		{
		if (inName == (*i)->GetTableName())
			return *i;
		}
	return nil;
	}

void ZDBDatabase::GetTableVector(vector<ZDBTable*>& outTableVector) const
	{
	// We don't return a reference as we will not always be using a vector internally
	outTableVector = fTables;
	}

void ZDBDatabase::Flush()
	{
	ZMutexComposite theComposite;
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		theComposite.Add((*i)->GetLock()->GetRead());
	ZLocker theLocker(theComposite);
	if (fDirty)
		this->WriteBack();
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		(*i)->Flush();
	fBlockStore->Flush();
	}

bool ZDBDatabase::Validate(ZProgressWatcher* iProgressWatcher)
	{
	ZProgressPusher thePusher(iProgressWatcher, "Checking tables", fTables.size());

	bool allGood = true;
	try
		{
		for (vector<ZDBTable*>::iterator i = fTables.begin(); allGood && i != fTables.end(); ++i)
			{
			if (!(*i)->Validate(iProgressWatcher))
				allGood = false;
			}
		}
	catch (...)
		{
		allGood = false;
		}

	return allGood;
	}

void ZDBDatabase::CopyInto(ZProgressWatcher* iProgressWatcher, ZDBDatabase& iDestDB)
	{
	ZProgressPusher thePusher(iProgressWatcher, "Copying tables", fTables.size());

	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		{
		ZLocker lockerSource((*i)->GetLock()->GetRead());

		ZDBTable* theTable = iDestDB.CreateTable((*i)->GetTableName(), (*i)->GetFieldSpecVector(), (*i)->GetIndicesSpan(), (*i)->GetCreatingRecordIDs());

		ZLocker lockerDest(theTable->GetLock()->GetWrite());

		vector<vector<ZDBFieldName> > theIndexSpecVector;
		(*i)->GetIndexSpecVector(theIndexSpecVector);
		theTable->CreateIndices(theIndexSpecVector, iProgressWatcher);

		(*i)->CopyInto(iProgressWatcher, theTable);
		}
	}

ZDBTable* ZDBDatabase::MakeTable(const string& inName, const vector<ZDBFieldSpec>& inFieldSpecList, bool inFullySpanningIndices, bool inCreateIDs)
	{ return new ZDBTable(this, inName, inFieldSpecList, inFullySpanningIndices, inCreateIDs); }

void ZDBDatabase::WriteBack()
	{
	ZRef<ZStreamerWPos> theStreamer = fBlockStore->OpenWPos(fHeaderBlockID);
	{
	ZStreamW_Buffered theStream(1024, theStreamer->GetStreamWPos());

	theStream.WriteInt32(2); // Structure version number
	theStream.WriteInt32(fRefCon);
	theStream.WriteInt32(fTables.size());
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		theStream.WriteUInt32((*i)->GetHeaderBlockID());

	}
	theStreamer->GetStreamWPos().Truncate();
	fDirty = false;
	}

void ZDBDatabase::LoadUp()
	{
	// Ensure that our list is empty -- this should really only be called once anyway, at construct time
	for (vector<ZDBTable*>::iterator i = fTables.begin(); i != fTables.end(); ++i)
		delete *i;
	fTables.erase(fTables.begin(), fTables.end());

	ZRef<ZStreamerR> theStreamer = fBlockStore->OpenR(fHeaderBlockID);
	const ZStreamR& theStream = theStreamer->GetStreamR();

	fRefCon = 0;
	long structureVersion = theStream.ReadInt32();
	if (structureVersion == 0)
		fRefCon = -1;
	else if (structureVersion == 1)
		fRefCon = 1;
	else if (structureVersion == 2)
		fRefCon = theStream.ReadInt32();

	long tableCount = theStream.ReadInt32();
	for (long x = 0; x < tableCount; ++x)
		{
		ZBlockStore::BlockID theHandle = theStream.ReadUInt32();
		try
			{
			ZDBTable* aTable = new ZDBTable(this, theHandle);
			fTables.push_back(aTable);
			}
		catch (...)
			{}
		}
	fDirty = false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBRecord

ZDBRecord::ZDBRecord(ZDBTable* inTable, const ZDBTreeItem* inTreeItem, bool inPulledFromIndices)
:	fTable(inTable), fFinalizationInProgress(false)
	{
	// This constructor is called by a ZDBIndex when it is creating a "fake" record from
	// its own copy of the record's fields. This only happens when the index fully spans
	// the table's set of fields, and thus the index has a copy of every field in the record in
	// question. And of course all other indices must also have a complete set of fields,
	// otherwise one of them will be tracking a block on disk with the full set.
	fDeleted = false;
	fPulledFromIndices = inPulledFromIndices;
	fBlockID = 0;

	ZDBIndex* theIndex(inTreeItem->GetIndex());
	const vector<ZDBFieldRep*>& fieldReps(inTreeItem->GetFieldReps());
	const vector<ZDBFieldName>& fieldNames(theIndex->GetIndexedFieldNames());

	// 96-09-16. Although we don't have to create our records in any particular
	// order, remember that there must be a canonical ordering when sending them
	// over a net connection.

	// Walk the list of fieldNames from the index, and the fieldReps from the treeItem
	// creating DBFields that contain info from both. This is safe to do because the tree item
	// keeps its ZDBFieldReps in the same order (sort order) as the index keeps its field IDs
	fFieldList.reserve(fieldReps.size());
	vector<ZDBFieldRep*>::const_iterator fieldRepIterator = fieldReps.begin();
	vector<ZDBFieldName>::const_iterator fieldNameIterator = fieldNames.begin();
	for (;fieldRepIterator != fieldReps.end(); ++fieldRepIterator, ++fieldNameIterator)
		{
		ZDBField* aField = new ZDBField(this, (*fieldNameIterator), (*fieldRepIterator)->Clone());
		fFieldList.push_back(aField);
		}
	}

ZDBRecord::ZDBRecord(ZDBTable* inTable, ZBlockStore::BlockID inBlockID, const vector<ZDBFieldSpec>& inSpecList)
:	fTable(inTable), fBlockID(inBlockID), fFinalizationInProgress(false)
	{
	fDeleted = false;
	fPulledFromIndices = false;

	this->CreateFields(inSpecList, ZDBRecordID(0));

// Either we've been created fresh, or we haven't been read in yet. In either case we're dirty.
	fDirty = true;
	}

ZDBRecord::ZDBRecord(ZDBTable* inTable, ZBlockStore::BlockID inBlockID,
			const vector<ZDBFieldSpec>& inSpecList, const ZDBRecordID& inRecordID, bool inPulledFromIndices)
:	fTable(inTable), fBlockID(inBlockID), fFinalizationInProgress(false)
	{
	// This constructor is used when creating a new record in the database
	fDeleted = false;
	fPulledFromIndices = inPulledFromIndices;

	this->CreateFields(inSpecList, inRecordID);

	// Walk our field list telling each to allocate its extra storage. This only affects
	// DBFieldBLOB, as it keeps its data in a separate block
	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		(*x)->CreateExtraStorage(fTable->GetBlockStore());


	// Either we've been created fresh, or we haven't been read in yet. In either case we're dirty.
	fDirty = true;
	}

ZDBRecord::~ZDBRecord()
	{
	// Delete our owned fields
	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		delete (*x);
	}

void ZDBRecord::Finalize()
	{
	// We shouldn't be finalized if we've been pulled from our table's indices
	ZAssert(!fPulledFromIndices);

	// If we've been deleted, then our table is no longer keeping track of us and
	// we should just delete ourselves (remembering to call FinalizationComplete to
	// release the ZRefCountedWithFinalization mutex).
	if (fDeleted == true)
		{
		this->FinalizationComplete();
		delete this;
		return;
		}

	fTable->RecordBeingFinalized(this);
	}

ZRWLock* ZDBRecord::GetLock()
	{ return fTable->GetLock(); }

bool ZDBRecord::CanBeRead()
	{
	ZAssert(fDeleted == false);
	ZAssert(this->GetLock());
	return this->GetLock()->CanRead();
	}

bool ZDBRecord::CanBeWritten()
	{
	ZAssert(fDeleted == false);
	ZAssert(this->GetLock());
	return this->GetLock()->CanWrite();
	}

void ZDBRecord::RecordPreRead()
	{
	ZAssert(!fPulledFromIndices);
	// Tell the table all of our contents are about to be modified
	fTable->RecordPreRead(this);
	fPulledFromIndices = true;
	}

void ZDBRecord::RecordPostRead()
	{
	ZAssert(fPulledFromIndices);

	// Tell the table our contents have finished being modified
	fPulledFromIndices = false;
	fDirty = true;
	fTable->RecordPostRead(this);
	}

bool ZDBRecord::HasField(const ZDBFieldName& inFieldName) const
	{
	ZAssert(fDeleted == false);

	ZDBField* theField = nil;
	for (vector<ZDBField*>::const_iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		{
		if ((*x)->GetFieldName() == inFieldName)
			{
			theField = *x;
			return true;
			}
		}
	return false;
	}

ZDBField* ZDBRecord::GetField(const ZDBFieldName& inFieldName) const
	{
	ZAssert(fDeleted == false);

	ZDBField* theField = nil;
	for (vector<ZDBField*>::const_iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		{
		if ((*x)->GetFieldName() == inFieldName)
			{
			theField = *x;
			break;
			}
		}
	if (!theField)
		{
		string tableName;
		if (ZDBTable* table = this->GetTable())
			tableName = table->GetTableName();
		ZDebugStopf(0, ("ZDBRecord::GetField, theField == nil (for '%s' in '%s')", inFieldName.AsString().c_str(), tableName.c_str()));
		}
	return theField;
	}

const vector<ZDBField*>& ZDBRecord::GetFields() const
	{ return fFieldList; }

ZDBRecordID ZDBRecord::GetRecordID() const
	{
	ZDBRecordID theRecordID;
	ZDBField* theField = this->GetField(ZDBFieldName::ID);
	ZAssert(theField);
	if (theField)
		theRecordID = ((ZDBFieldRepID*)(theField->UnsafeGetRep()))->GetValue();
	return theRecordID;
	}

void ZDBRecord::CopyFrom(ZDBRecord* inOther, bool inCopyRecordID)
	{
	if (!fPulledFromIndices)
		fTable->RecordPreRead(this);

	// Used (currently) when we're making a new table based on the contents of an old one
	const vector<ZDBField*>& otherFields(inOther->GetFields());

	for (vector<ZDBField*>::iterator myIter = fFieldList.begin(); myIter != fFieldList.end(); ++myIter)
		{
		ZDBField* myField = *myIter;
		if (!inCopyRecordID && (myField->GetFieldName() == ZDBFieldName::ID))
			continue;
		for (vector<ZDBField*>::const_iterator otherIter = otherFields.begin(); otherIter != otherFields.end(); ++otherIter)
			{
			ZDBField* otherField = *otherIter;
			if (myField->GetFieldName() == otherField->GetFieldName())
				myField->CopyFrom(fTable->GetBlockStore(), otherField, inOther->fTable->GetBlockStore());
			}
		}

	fDirty = true;
	if (!fPulledFromIndices)
		fTable->RecordPostRead(this);
	}

void ZDBRecord::FieldPreChange(ZDBField* inField)
	{
	if (!fPulledFromIndices)
		fTable->RecordPreChange(this, inField);
	}

void ZDBRecord::FieldPostChange(ZDBField* inField)
	{
	fDirty = true;
	if (!fPulledFromIndices)
		fTable->RecordPostChange(this, inField);
	}

void ZDBRecord::FromStream(const ZStreamR& inStream)
	{
	long dummy = inStream.ReadInt32();

	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		(*x)->FromStream(inStream);
	fDirty = false;
	}

void ZDBRecord::ToStream(const ZStreamW& inStream)
	{
	// Write out a dummy value that we can turn into flags or versioning information
	inStream.WriteInt32(0);

	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		(*x)->ToStream(inStream);
	fDirty = false;
	}

bool ZDBRecord::CheckAllFieldsOkay()
	{
	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		{
		if (!(*x)->CheckExtraStorageOkay(fTable->GetBlockStore()))
			return false;
		}
	return true;
	}

bool ZDBRecord::IsIdenticalTo(const ZDBRecord* inRecord) const
	{
	// See ZDBTable::LoadRecordFromBTreeItem, which is the only method that calls us

	// Shortcut, if it's the same pointer, we're identical
	if (inRecord == this)
		return true;

	// It *must* come from the same table as us
	ZAssert(fTable == inRecord->fTable);

	// We cannot assume that fields in the other record are in the same order as ours, it
	// might have been created by a different index, which would have stuffed fields in
	// in their sorting order

	// Walk our list of fields, compare each against the other's matching field
	for (vector<ZDBField*>::const_iterator i = fFieldList.begin(); i != fFieldList.end(); ++i)
		{
		bool gotIt = false;
		for (vector<ZDBField*>::const_iterator j = inRecord->fFieldList.begin(); j != inRecord->fFieldList.end(); ++j)
			{
			if ((*i)->GetFieldName() == (*j)->GetFieldName())
				{
				gotIt = true;
				if ((*i)->UnsafeGetRep()->Compare((*j)->UnsafeGetRep()) != 0)
					return false;
				}
			}
		// We *have* to have found the current field at some point
		ZAssert(gotIt);
		}
	return true;
	}

bool ZDBRecord::CompareForCheck::operator()(ZDBRecord* const& first, ZDBRecord* const& second) const
	{
	// See ZDBIndex::WalkNodeAndCheck, which is the only method that calls us
	ZAssert(first->fTable == second->fTable);

	// Shortcut, if it's the same pointer, we're identical
	if (first == second)
		return false;

	// We cannot assume that fields in the other record are in the same order as ours, it
	// might have been created by a different index, which would have stuffed fields in
	// in their sorting order

	// Walk our list of fields, compare each against the other's matching field
	for (vector<ZDBField*>::const_iterator i = first->fFieldList.begin(); i != first->fFieldList.end(); ++i)
		{
		#if ZCONFIG_Debug
		bool gotIt = false;
		#endif

		for (vector<ZDBField*>::const_iterator j = second->fFieldList.begin(); j != second->fFieldList.end(); ++j)
			{
			if ((*i)->GetFieldName() == (*j)->GetFieldName())
				{
				#if ZCONFIG_Debug
				gotIt = true;
				#endif
				long result = (*i)->UnsafeGetRep()->Compare((*j)->UnsafeGetRep());
				if (result == -1)
					return true;
				else if (result == 1)
					return false;
				break;
				}
			}
		// We *have* to have found the current field at some point
		#if ZCONFIG_Debug
		ZAssert(gotIt);
		#endif
		}
	return false;
	}

void ZDBRecord::DeleteYourself()
	{
	// Flag that we've been deleted -- checked by our fields if someone tries to
	// access us after this has happened. Also, checked when we're being finalized
	// as the lack of a record handle is not a complete indicator of our deleted status
	// (we may come from a fully-spanned table).
	fDeleted = true;

	// Walk our field list telling each to delete its storage. This only actually affects
	// DBFieldBLOB, as it keeps its data in a separate block
	for (vector<ZDBField*>::iterator x = fFieldList.begin(); x != fFieldList.end(); ++x)
		{
		try
			{
			(*x)->DeleteExtraStorage(fTable->GetBlockStore());
			}
		catch (...)
			{
			}
		}

	if (fBlockID)
		{
		fTable->GetBlockStore()->Delete(fBlockID);
		fBlockID = 0;
		}
	fDirty = false;
	}

void ZDBRecord::CreateFields(const vector<ZDBFieldSpec>& inSpecList, const ZDBRecordID& inRecordID)
	{
	// Walk the specification vector, creating fields to match
	fFieldList.reserve(inSpecList.size());
	for (vector<ZDBFieldSpec>::const_iterator x = inSpecList.begin(); x != inSpecList.end(); ++x)
		{
		ZDBField* theField = nil;
		switch (x->GetFieldType())
			{
			case ZDBFieldRep::eNumber:
			case ZDBFieldRep::eString:
			case ZDBFieldRep::eIString:
			case ZDBFieldRep::eBinary:
			case ZDBFieldRep::eBLOB:
			case ZDBFieldRep::eFloat:
			case ZDBFieldRep::eDouble:
				theField = new ZDBField(this, x->GetFieldName(), fTable->CreateFieldRepByType(x->GetFieldType()));
				break;
			case ZDBFieldRep::eID:
				{
				// Handle the special ZDBFieldName::ID field, whose value is taken from theRecordID, and which is never
				// changed (it's generated by the table). Any other field which is of type eID can hold any
				// value, as it will be a relational reference to another record
				if (x->GetFieldName() == ZDBFieldName::ID)
					theField = new ZDBField(this, x->GetFieldName(), new ZDBFieldRepID(inRecordID));
				else
					theField = new ZDBField(this, x->GetFieldName(), new ZDBFieldRepID());
				}
				break;
			default:
				ZUnimplemented();
				break;
			}
		ZAssert(theField);
		fFieldList.push_back(theField);
		}
	}

// Return ZDBRecord as ZTuple. Do not confuse this with KADBOwner::GetRecordAsTuple() - which
// is specialized for KF. this will include *all* fields - even '__ID' . -ec 00.12.02
ZTuple ZDBRecord::GetAsTuple()
	{
	ZTuple theTuple;
	ZLocker theLock(this->GetLock()->GetRead());

	const vector<ZDBField*>& theFieldList = this->GetFields();
	for (vector<ZDBField*>::const_iterator x = theFieldList.begin(); x !=theFieldList.end(); ++x)
		{
		ZDBFieldName fieldName = (*x)->GetFieldName();
		ZDBFieldRep::FieldType fieldType = (*x)->GetFieldType();
		const ZDBFieldRep* inFieldRep = (*x)->GetRep();
		ZAssertStop(2, inFieldRep);
		switch (inFieldRep->GetFieldType())
			{
			case ZDBFieldRep::eNumber: 
				{
				if (const ZDBFieldRepNumber* numberRep = dynamic_cast<const ZDBFieldRepNumber*>(inFieldRep))
					theTuple.SetInt32(fieldName.AsString(), numberRep->GetNumber());
				break;
				}
			case ZDBFieldRep::eString: 
				{
				if (const ZDBFieldRepString* stringRep =  dynamic_cast<const ZDBFieldRepString*>(inFieldRep))
					theTuple.SetString(fieldName.AsString(), stringRep->GetString());
				break;
				}
			case ZDBFieldRep::eIString:
				{
				if (const ZDBFieldRepIString* stringRep =  dynamic_cast<const ZDBFieldRepIString*>(inFieldRep))
					theTuple.SetString(fieldName.AsString(), stringRep->GetString());
				break;
				}
			case ZDBFieldRep::eID: 
				{
				if (const ZDBFieldRepID* idRep =  dynamic_cast<const ZDBFieldRepID*>(inFieldRep))
					theTuple.SetInt64(fieldName.AsString(), idRep->GetValue().GetValue());
				break;
				}
			case ZDBFieldRep::eBinary: 
				{
				if (const ZDBFieldRepBinary* binaryRep =  dynamic_cast<const ZDBFieldRepBinary*>(inFieldRep))
					{
					if (size_t theLength = binaryRep->GetLength())
						{
						const char* theData = reinterpret_cast<const char*>(binaryRep->GetData());
						string binaryAsString;
						for (size_t x = 0; x < theLength; ++x)
							binaryAsString += ZString::sHexFromInt(theData[x]);
						theTuple.SetString(fieldName.AsString(), binaryAsString);
						} 
					}
				break;
				}
			case ZDBFieldRep::eBLOB: 
				{
				if (const ZDBFieldRepBLOB* blobRep = dynamic_cast<const ZDBFieldRepBLOB*>(inFieldRep))
					theTuple.SetInt32(fieldName.AsString(), blobRep->GetBLOBHandle());
				break;
				}
			case ZDBFieldRep::eFloat:
				{
				if (const ZDBFieldRepFloat* floatRep = dynamic_cast<const ZDBFieldRepFloat*>(inFieldRep))
					theTuple.SetFloat(fieldName.AsString(), floatRep->GetFloat());
				break;
				}
			case ZDBFieldRep::eDouble:
				{
				if (const ZDBFieldRepDouble* doubleRep = dynamic_cast<const ZDBFieldRepDouble*>(inFieldRep))
					theTuple.SetDouble(fieldName.AsString(), doubleRep->GetDouble());
				break;
				}
			}
		}
	return theTuple;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex

string ZDBIndex::sIndexNameToString(const vector<ZDBFieldName>& inNames)
	{
	string indexName;
	for (vector<ZDBFieldName>::const_iterator j = inNames.begin(); j != inNames.end();)
		{
		indexName += (*j).AsString();
		++j;
		if (j == inNames.end())
			break;
		indexName += " ";
		}
	return indexName;
	}

bool ZDBIndex::DoYouIndexOnTheseFieldsPrecisely(const vector<ZDBFieldName>& inFields)
	{
	// Walk the list of fields, and our list, together. If we have an entry that
	// does not match, we've failed.
	if (inFields.size() != fIndexedFields.size())
		return false;
	vector<ZDBFieldName>::iterator us = fIndexedFields.begin();
	vector<ZDBFieldName>::const_iterator them = inFields.begin();
	for (; us != fIndexedFields.end() && them != inFields.end(); ++us, ++them)
		{
		if (*us != *them)
			return false;
		}
	return true;
	}

bool ZDBIndex::DoYouIndexOnThisFieldListPrefix(const vector<ZDBFieldName>& inFields)
	{
	// Walk our list of fields until theFields runs out
	long theCount = inFields.size();
	long ourCount = fIndexedFields.size();
	if (theCount > ourCount)
		return false;
	vector<ZDBFieldName>::iterator us = fIndexedFields.begin();
	vector<ZDBFieldName>::const_iterator them = inFields.begin();
	for (; us != fIndexedFields.end() && them != inFields.end(); ++us, ++them)
		{
		if (*us != *them)
			return false;
		}
	return true;
	}

class ZDBIndex::IntermediateNodeWalker : public ZBTree::NodeWalker
	{
public:
	IntermediateNodeWalker(ZDBIndex* inIndex, ZDBIndex::NodeWalker& inRealWalker);
	~IntermediateNodeWalker();

// From ZBTree::NodeWalker
	virtual void NodeEntered(const ZRef<ZBTree::Node>& inNode);
	virtual void DumpItem(ZBTree::Item* inItem);
	virtual void NodeExited(const ZRef<ZBTree::Node>& inNode);

protected:
	ZDBIndex* fIndex;
	ZDBIndex::NodeWalker& fRealWalker;
	};

ZDBIndex::IntermediateNodeWalker::IntermediateNodeWalker(ZDBIndex* inIndex, ZDBIndex::NodeWalker& inRealWalker)
:	fIndex(inIndex), fRealWalker(inRealWalker)
	{}

ZDBIndex::IntermediateNodeWalker::~IntermediateNodeWalker()
	{}

void ZDBIndex::IntermediateNodeWalker::NodeEntered(const ZRef<ZBTree::Node>& inNode)
	{
	ZRef<ZDBTreeNode> theDBTreeNode = dynamic_cast<ZDBTreeNode*>(inNode.GetObject());
	ZAssert(theDBTreeNode);
	fRealWalker.NodeEntered(fIndex, theDBTreeNode);
	}

void ZDBIndex::IntermediateNodeWalker::DumpItem(ZBTree::Item* inItem)
	{
	ZDBTreeItem* theDBTreeItem = dynamic_cast<ZDBTreeItem*>(inItem);
	ZAssert(theDBTreeItem);
	ZRef<ZDBRecord> theRecord = fIndex->LoadRecordFromBTreeItem(theDBTreeItem);
	fRealWalker.DumpItem(fIndex, theDBTreeItem, theRecord);
	}

void ZDBIndex::IntermediateNodeWalker::NodeExited(const ZRef<ZBTree::Node>& inNode)
	{
	ZRef<ZDBTreeNode> theDBTreeNode = dynamic_cast<ZDBTreeNode*>(inNode.GetObject());
	ZAssert(theDBTreeNode);
	fRealWalker.NodeExited(fIndex, theDBTreeNode);
	}


void ZDBIndex::WalkNodes(NodeWalker& inNodeWalker)
	{
	IntermediateNodeWalker theWalker(this, inNodeWalker);
	fBTree->WalkNodes(theWalker);
	}

long ZDBIndex::GetDegree()
	{ return fBTree->GetDegree(); }

ZDBIndex::ZDBIndex(ZDBTable* inTable, const vector<ZDBFieldName>& inIndexedFieldNames, long inDegreeOfNodeSpace)
:	fTable(inTable),
	fIndexedFields(inIndexedFieldNames),
	fHeadIterator(nil),
	fIteratorMutex("ZDBIndex::fIteratorMutex"),
	fBTree(nil),
	fNodeSpace(nil)
	{
	this->BuildFieldSpecVector();

	// Last param is the version of the format on disk
	fNodeSpace = new ZDBTreeNodeSpace(inDegreeOfNodeSpace, this, 1);

	// Have it create its root node
	fNodeSpace->Create();

	fBTree = new ZBTree(fNodeSpace);

	// And have it grab a ref to the root node
	fNodeSpace->GrabRootNode();
	}

ZDBIndex::ZDBIndex(ZDBTable* inTable)
:	fTable(inTable),
	fHeadIterator(nil),
	fIteratorMutex("ZDBIndex::fIteratorMutex"),
	fBTree(nil),
	fNodeSpace(nil)
	{
	// We haven't been loaded yet, so we don't create anything
	}

ZDBIndex::~ZDBIndex()
	{
	// Deleting the b tree should cause all loaded nodes to be flushed out, so there's no need to
	// explicitly do so here
	delete fBTree;
	delete fNodeSpace;
	}

void ZDBIndex::DeleteYourself()
	{
	// Walk all our nodes and delete them
	ZBTree::Handle rootHandle = fNodeSpace->GetRootHandle();
	this->WalkNodeAndDelete(rootHandle);
	delete this;
	}

bool ZDBIndex::AllNodesPresentAndCorrect(long& oFoundRecordCount, set<ZBlockStore::BlockID>* ioFoundRecordBlockIDs,
					set<ZDBRecord*, ZDBRecord::CompareForCheck>* ioFoundDBRecords, ZProgressWatcher* iProgressWatcher)
	{
	oFoundRecordCount = 0;
	try
		{
		set<ZBTree::Handle> visitedNodeHandles;
		return this->WalkNodeAndCheck(fNodeSpace->GetRootHandle(), visitedNodeHandles, oFoundRecordCount, ioFoundRecordBlockIDs, ioFoundDBRecords, iProgressWatcher);
		}
	catch (bad_alloc& theEx)
		{ throw; }
	catch (...)
		{}
	return false;
	}

bool ZDBIndex::WalkNodeAndCheck(ZBTree::Handle inHandle, set<ZBTree::Handle>& ioVisitedNodeHandles, long& ioFoundRecordCount, set<ZBlockStore::BlockID>* ioFoundRecordBlockIDs,
			set<ZDBRecord*, ZDBRecord::CompareForCheck>* ioFoundDBRecords, ZProgressWatcher* iProgressWatcher)
	{
	if (inHandle == ZBTree::kInvalidHandle)
		return false;
	if (ioVisitedNodeHandles.find(inHandle) != ioVisitedNodeHandles.end())
		return false;
	ioVisitedNodeHandles.insert(inHandle);
	bool allOkay = true;
	try
		{
		ZRef<ZBTree::Node> currentNode(fNodeSpace->LoadNode(inHandle));
		ZAssert(currentNode);
		// Walk the node's subnodes
		long nodeItemCount = currentNode->GetItemCount();
		ioFoundRecordCount += nodeItemCount;
		if (iProgressWatcher)
			iProgressWatcher->Process(nodeItemCount);
		if (!currentNode->GetIsLeaf())
			{
			for (long x = 0; x <= nodeItemCount; ++x)
				{
				ZBTree::Handle aNodeHandle = currentNode->GetHandleAt(x);
				if (!this->WalkNodeAndCheck(aNodeHandle, ioVisitedNodeHandles, ioFoundRecordCount, ioFoundRecordBlockIDs, ioFoundDBRecords, iProgressWatcher))
					allOkay = false;
				}
			}
		if (fTable->GetIndicesSpan())
			{
			if (ioFoundDBRecords)
				{
				for (long x = 0; x < nodeItemCount; ++x)
					{
					ZDBTreeItem* theItem = (ZDBTreeItem*)currentNode->GetItemAt(x);
					ZAssert(theItem);
					ZDBRecord* theRecord = fTable->MakeRecord(theItem, true);
					if (ioFoundDBRecords->find(theRecord) != ioFoundDBRecords->end())
						{
						delete theRecord;
						allOkay = false;
						}
					else
						{
						ioFoundDBRecords->insert(theRecord);
						}
					}
				}
			}
		else
			{
			if (ioFoundRecordBlockIDs)
				{
				for (long x = 0; x < nodeItemCount; ++x)
					{
					ZDBTreeItem* theItem = (ZDBTreeItem*)currentNode->GetItemAt(x);
					ZAssert(theItem);
					ZBlockStore::BlockID theRecordBlockID = theItem->GetRecordBlockID();
					if (ioFoundRecordBlockIDs->find(theRecordBlockID) != ioFoundRecordBlockIDs->end())
						{
						// If we have a duplicate then we've got a problem
						allOkay = false;
						}
					else
						{
						// Check that the record can be loaded
						if (!fTable->CheckRecordLoadable(theRecordBlockID))
							allOkay = false;
						else
							ioFoundRecordBlockIDs->insert(theRecordBlockID);
						}
					}
				}
			}
		}
	catch (bad_alloc& theEx)
		{ throw; }
	catch (...)
		{
		allOkay = false;
		}
	return allOkay;
	}

void ZDBIndex::WalkNodeAndDelete(ZBTree::Handle inStartHandle)
	{
	if (inStartHandle == ZBTree::kInvalidHandle)
		return;
	try
		{
		ZRef<ZBTree::Node> currentNode(fNodeSpace->LoadNode(inStartHandle));
		ZAssert(currentNode);
		// Walk the node's subnodes
		if (!currentNode->GetIsLeaf())
			{
			long nodeItemCount = currentNode->GetItemCount();
			for (long x = 0; x <= nodeItemCount; ++x)
				{
				ZBTree::Handle aNodeHandle = currentNode->GetHandleAt(x);
				this->WalkNodeAndDelete(aNodeHandle);
				}
			}
		fNodeSpace->DeleteNode(currentNode);
		}
	catch (bad_alloc& theEx)
		{ throw; }
	catch (...)
		{
		// Just absorb and ignore this error
		}
	}

ZRWLock* ZDBIndex::GetLock() const
	{ return fTable->GetLock(); }

void ZDBIndex::RecordPreChange(ZDBRecord* inRecord, const ZDBFieldName& inFieldName)
	{
	// Only bother if we index on this field
	for (vector<ZDBFieldName>::iterator i = fIndexedFields.begin(); i != fIndexedFields.end(); ++i)
		{
		if ((*i) == inFieldName)
			{
			this->InvalidateIterators();
			ZDBTreeItem itemToDelete(this, inRecord);
			fBTree->Remove(&itemToDelete);
			return;
			}
		}
	}

void ZDBIndex::RecordPostChange(ZDBRecord* inRecord, const ZDBFieldName& inFieldName)
	{
	// Only bother if we index on this field
	for (vector<ZDBFieldName>::iterator i = fIndexedFields.begin(); i != fIndexedFields.end(); ++i)
		{
		if ((*i) == inFieldName)
			{
			this->InvalidateIterators();
			ZDBTreeItem* newItem = new ZDBTreeItem(this, inRecord);
			fBTree->Insert(newItem);
			return;
			}
		}
	}

void ZDBIndex::RecordPreRead(ZDBRecord* inRecord)
	{
	this->InvalidateIterators();
	ZDBTreeItem itemToDelete(this, inRecord);
	fBTree->Remove(&itemToDelete);
	}

void ZDBIndex::RecordPostRead(ZDBRecord* inRecord)
	{
	this->InvalidateIterators();
	fBTree->Insert(new ZDBTreeItem(this, inRecord));
	}

void ZDBIndex::AddedNewRecord(ZDBRecord* inRecord)
	{
	// Insert the record into our index, we only have to record its block ID if
	// fTable->GetIndicesSpan() returns false;
	this->InvalidateIterators();
	ZDBTreeItem* newItem = new ZDBTreeItem(this, inRecord);
	fBTree->Insert(newItem);
	}

void ZDBIndex::DeletedRecord(ZDBRecord* inRecord)
	{
	// Remove the record from our index
	this->InvalidateIterators();
	ZDBTreeItem itemToDelete(this, inRecord);
	fBTree->Remove(&itemToDelete);
	}

void ZDBIndex::FromStream(const ZStreamR& inStream, long inVersion)
	{
	ZAssert(inVersion == 1 || inVersion == 2);

	// Make sure our list of fields are empty
	ZAssert(fIndexedFields.size() == 0);

	// Get the handle of our root node
	ZBTree::Handle theHandle = inStream.ReadUInt32();

	long degreeOfNodeSpace = inStream.ReadInt32();

	// Now, read in our indexed fields
	long theCountOfIndexedFields = inStream.ReadInt32();
	for (long x = 1; x <= theCountOfIndexedFields; ++x)
		{
		ZDBFieldName theName;
		theName.FromStream(inStream);
		fIndexedFields.push_back(theName);
		}

	this->BuildFieldSpecVector();

	// Create our nodespace and b tree
	ZAssert (fNodeSpace == nil && fBTree == nil);
	fNodeSpace = new ZDBTreeNodeSpace(degreeOfNodeSpace, this, theHandle, inVersion);
	fBTree = new ZBTree(fNodeSpace);

	// AG 98-03-21. There is a bug in Solaris g++ 2.7.2.x wherein exceptions don't
	// get caught every place they should. Although we catch exceptions inside GrabRootNode,
	// the catch block is skipped over, so we do the catch here as well, which does work.
	try
		{
		// And have it grab a ref to the root node
		fNodeSpace->GrabRootNode();
		}
	catch (bad_alloc& theEx)
		{ throw; }
	catch (...)
		{
		// Absorb the error
		}
	}

void ZDBIndex::ToStream(const ZStreamW& inStream, long inVersion)
	{
	ZAssert(inVersion == 0);

	// Write the handle of our ZDBTreeNodeSpace's roothandle
	inStream.WriteUInt32(fNodeSpace->GetRootHandle());

	// Write our btree's degree
	inStream.WriteInt32(fNodeSpace->GetDegree());

	// Write out our list of indexed fields
	inStream.WriteInt32(fIndexedFields.size());
	for (vector<ZDBFieldName>::iterator x = fIndexedFields.begin(); x != fIndexedFields.end(); ++x)
		x->ToStream(inStream);
	}

void ZDBIndex::Flush()
	{ fNodeSpace->Flush(); }

bool ZDBIndex::GetIndicesSpan()
	{ return fTable->GetIndicesSpan(); }

ZBlockStore* ZDBIndex::GetBlockStore()
	{ return fTable->GetBlockStore(); }

ZRef<ZDBRecord> ZDBIndex::LoadRecordFromBTreeItem(const ZDBTreeItem* inItem)
	{ return fTable->LoadRecordFromBTreeItem(inItem); }

void ZDBIndex::BuildFieldSpecVector()
	{
	/* This routine builds a vector of ZDBFieldSpecs, which is used by DBTreeItems when they are constructing
	themselves. We used to generate this vector each time it was needed -- but this used up a
	*lot* of CPU time. */
	fIndexedFieldSpecs.erase(fIndexedFieldSpecs.begin(), fIndexedFieldSpecs.end());

	// Get the table's ZDBFieldSpec vector
	const vector<ZDBFieldSpec>& allFieldSpecs = fTable->GetFieldSpecVector();

	// Now walk *our* list of field IDs in order. As we find a matching field spec, decrement the
	// count of fields left to find, and add the field spec to our vector of field specs -- which is
	// thus kept in the same order as our sorting order, so DBTreeItems which use this vector
	// will have the correct primary sort order.

	long countLeftToFind = allFieldSpecs.size();
	for (vector<ZDBFieldName>::iterator i = fIndexedFields.begin(); i != fIndexedFields.end(); ++i)
		{
		for (vector<ZDBFieldSpec>::const_iterator j = allFieldSpecs.begin(); j !=allFieldSpecs.end(); ++j)
			{
			if ((*i) == j->GetFieldName())
				{
				countLeftToFind--;
				fIndexedFieldSpecs.push_back(*j);
				break;
				}
			}
		}
	ZAssert(fTable->GetIndicesSpan() == false || countLeftToFind == 0);
	}

void ZDBIndex::ChangeFieldName(ZDBFieldName inOldName, ZDBFieldName inNewName)
	{
	ZAssert(inOldName != ZDBFieldName::ID);
	ZAssert(inNewName != ZDBFieldName::ID);
	for (vector<ZDBFieldName>::iterator i = fIndexedFields.begin(); i != fIndexedFields.end(); ++i)
		{
		if ((*i) == inOldName)
			{
			(*i).SetName(inNewName.GetName());
			break;
			}
		}
	this->BuildFieldSpecVector();
	}

void ZDBIndex::InvalidateIterators()
	{
	// This looks dodgy. We're unconditionally calling Release against our
	// read lock for each current iterator. And it is kinda dodgy, except
	// that InvalidateIterators is only called because a mutating operation
	// has occurred, which requires that the write lock be held. So the
	// only extant iterators must all actually be holding the write lock,
	// and must all be instantiated on the current thread. So we're okay.
	
	fIteratorMutex.Acquire();
	iterator* currentIterator = fHeadIterator;
	fHeadIterator = nil;
	ZMutexBase& theReadLock = this->GetLock()->GetRead();
	while (currentIterator)
		{
		iterator* nextIterator = currentIterator->fNextIterator;
		currentIterator->fNextIterator = nil;
		currentIterator->fPrevIterator = nil;
		currentIterator->fIndex = nil;
		currentIterator = nextIterator;
		theReadLock.Release();
		}
	fIteratorMutex.Release();
	}

void ZDBIndex::Internal_LinkIterator(ZDBIndex::iterator* inIterator)
	{
	fIteratorMutex.Acquire();
	if (fHeadIterator)
		fHeadIterator->fPrevIterator = inIterator;
	inIterator->fNextIterator = fHeadIterator;
	fHeadIterator = inIterator;
	fIteratorMutex.Release();

	this->GetLock()->GetRead().Acquire();
	}

void ZDBIndex::Internal_UnlinkIterator(ZDBIndex::iterator* inIterator)
	{
	fIteratorMutex.Acquire();
	if (inIterator->fPrevIterator)
		inIterator->fPrevIterator->fNextIterator = inIterator->fNextIterator;
	if (inIterator->fNextIterator)
		inIterator->fNextIterator->fPrevIterator = inIterator->fPrevIterator;
	if (fHeadIterator == inIterator)
		fHeadIterator = inIterator->fNextIterator;
	inIterator->fPrevIterator = nil;
	inIterator->fNextIterator = nil;
	inIterator->fIndex = nil;
	fIteratorMutex.Release();

	this->GetLock()->GetRead().Release();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex::iterator

ZDBIndex::iterator::iterator(ZDBIndex* inIndex, const vector<pair<ZBTree::Handle, long> >& inStack)
:	fIndex(inIndex), fStack(inStack), fCurrentRank(0), fCurrentRankValid(false), fPrevIterator(nil), fNextIterator(nil)
	{
	if (fIndex)
		fIndex->Internal_LinkIterator(this);
	}

ZDBIndex::iterator::iterator()
:	fIndex(nil), fCurrentRank(0), fCurrentRankValid(false), fPrevIterator(nil), fNextIterator(nil)
	{}

ZDBIndex::iterator::iterator(const ZDBIndex::iterator& inOther)
:	fIndex(inOther.fIndex), fStack(inOther.fStack),
	fCurrentRank(inOther.fCurrentRank), fCurrentRankValid(inOther.fCurrentRankValid),
	fPrevIterator(nil), fNextIterator(nil)
	{
	if (fIndex)
		fIndex->Internal_LinkIterator(this);
	}

ZDBIndex::iterator::~iterator()
	{
	if (fIndex)
		fIndex->Internal_UnlinkIterator(this);
	}

ZDBIndex::iterator& ZDBIndex::iterator::operator=(const ZDBIndex::iterator& inOther)
	{
	if (&inOther != this)
		{
		if (fIndex)
			fIndex->Internal_UnlinkIterator(this);
		fIndex = inOther.fIndex;
		fStack = inOther.fStack;
		fCurrentRank = inOther.fCurrentRank;
		fCurrentRankValid = inOther.fCurrentRankValid;
		if (fIndex)
			fIndex->Internal_LinkIterator(this);
		}
	return *this;
	}

bool ZDBIndex::iterator::operator==(const ZDBIndex::iterator& inOther) const
	{ return fIndex == inOther.fIndex && fStack == inOther.fStack; }

bool ZDBIndex::iterator::operator!=(const ZDBIndex::iterator& inOther) const
	{ return fIndex != inOther.fIndex || fStack != inOther.fStack; }

ZRef<ZDBRecord> ZDBIndex::iterator::operator*() const
	{
	if (fStack.size() == 0)
		throw range_error("ZDBIndex::iterator, dereference with empty stack");
	ZAssert(fIndex);
	ZRef<ZBTree::Node> currentNode = fIndex->fNodeSpace->LoadNode(fStack.back().first);
	ZDBTreeItem* theItem = static_cast<ZDBTreeItem*>(currentNode->GetItemAt(fStack.back().second));
	ZAssert(theItem != nil);
	return fIndex->LoadRecordFromBTreeItem(theItem);
	}

ZDBIndex::iterator& ZDBIndex::iterator::operator++()
	{
	this->MoveBy(1);
	return *this;
	}

ZDBIndex::iterator ZDBIndex::iterator::operator++(int)
	{
	ZDBIndex::iterator temp(*this);
	this->MoveBy(1);
	return temp;
	}

ZDBIndex::iterator& ZDBIndex::iterator::operator--()
	{
	this->MoveBy(-1);
	return *this;
	}

ZDBIndex::iterator ZDBIndex::iterator::operator--(int)
	{
	ZDBIndex::iterator temp(*this);
	this->MoveBy(-1);
	return temp;
	}

ZDBIndex::iterator ZDBIndex::iterator::operator+(difference_type offset) const
	{
	ZDBIndex::iterator temp(*this);
	temp.MoveBy(offset);
	return temp;
	}

ZDBIndex::iterator& ZDBIndex::iterator::operator+=(difference_type offset)
	{
	this->MoveBy(offset);
	return *this;
	}

ZDBIndex::iterator ZDBIndex::iterator::operator-(difference_type offset) const
	{
	ZDBIndex::iterator temp(*this);
	temp.MoveBy(-offset);
	return temp;
	}

ZDBIndex::iterator& ZDBIndex::iterator::operator-=(difference_type offset)
	{
	this->MoveBy(-offset);
	return (*this);
	}
ZDBIndex::iterator::difference_type ZDBIndex::iterator::operator-(const ZDBIndex::iterator& inOther) const
	{
	ZAssert(fIndex && fIndex == inOther.fIndex);
	return this->GetRank() - inOther.GetRank();
	}

ZRef<ZDBRecord> ZDBIndex::iterator::operator[](difference_type index) const
	{
	if (index == 0)
		return **this;
	ZDBIndex::iterator temp = *this;
	temp += index;
	return *temp;
	}

ZDBIndex::iterator::size_type ZDBIndex::iterator::GetRank() const
	{
	// We've been keeping track of how much we've been moved in fCurrentRank,
	// but we haven't necessarily worked out what our real position is, cause
	// it's kinda expensive to calculate -- we have to pull in every node to
	// the left of all nodes on our stack, which can equal tree depth * tree order
	// If nodes kept the sub tree size in another array along with the sub tree handles,
	// rather than keeping the sub tree size in each node, then we wouldn't have to pull in
	// adjacent nodes, just nodes on the stack. But this will change the format of nodes
	// on disk and in memory. Later maybe.
	if (!fCurrentRankValid)
		{
		const_cast<ZDBIndex::iterator*>(this)->fCurrentRank = fIndex->fBTree->CalculateRank(fStack);
		const_cast<ZDBIndex::iterator*>(this)->fCurrentRankValid = true;
		}
	return fCurrentRank;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBIndex::ZDBSearch

ZDBSearch::ZDBSearch(ZDBIndex* inIndex, const vector<ZDBSearchSpec>& inSearchSpecs, SearchType inSearchType)
:	fIndex(inIndex),
	fSearchType(inSearchType),
	fSearchItem(new ZDBTreeItem(inIndex, inSearchSpecs))
	{
	if (fIndex->GetLock())
		fIndex->GetLock()->GetRead().Acquire();
	}

ZDBSearch::ZDBSearch(ZDBIndex* inIndex, const ZDBSearchSpec& inSearchSpec, SearchType inSearchType)
:	fIndex(inIndex),
	fSearchType(inSearchType),
	fSearchItem(new ZDBTreeItem(inIndex, inSearchSpec))
	{
	if (fIndex->GetLock())
		fIndex->GetLock()->GetRead().Acquire();
	}

ZDBSearch::ZDBSearch(ZDBIndex* inIndex)
:	fIndex(inIndex),
	fSearchType(ZDBSearch::eAll),
	fSearchItem(nil)
	{
	if (fIndex->GetLock())
		fIndex->GetLock()->GetRead().Acquire();
	}

ZDBSearch::ZDBSearch()
:	fIndex(nil),
	fSearchType(ZDBSearch::eAll),
	fSearchItem(nil)
	{}

ZDBSearch::ZDBSearch(const ZDBSearch& inOther)
:	fIndex(inOther.fIndex),
	fSearchType(inOther.fSearchType),
	fSearchItem(nil)
	{
	if (fIndex->GetLock())
		fIndex->GetLock()->GetRead().Acquire();
	if (inOther.fSearchItem)
		fSearchItem = inOther.fSearchItem->Clone();
	}

ZDBSearch& ZDBSearch::operator=(const ZDBSearch& inOther)
	{
	// Release the lock we have on our current index.
	if (fIndex && fIndex->GetLock())
		fIndex->GetLock()->GetRead().Release();

	fIndex = inOther.fIndex;

	// And acquire a lock on the new index (if any).
	if (fIndex && fIndex->GetLock())
		fIndex->GetLock()->GetRead().Acquire();

	fSearchType = inOther.fSearchType;
	delete fSearchItem;
	fSearchItem = nil;
	if (inOther.fSearchItem)
		fSearchItem = inOther.fSearchItem->Clone();
	
	fCachedBeginIterator = inOther.fCachedBeginIterator;
	fCachedEndIterator = inOther.fCachedEndIterator;

	return *this;
	}

ZDBSearch::~ZDBSearch()
	{
	if (fIndex && fIndex->GetLock())
		fIndex->GetLock()->GetRead().Release();
	if (fSearchItem)
		delete fSearchItem;
	}

ZRef<ZDBRecord> ZDBSearch::operator[](size_type inIndex) const
	{ return this->begin()[inIndex]; }

ZDBSearch::size_type ZDBSearch::size() const
	{
	if (fIndex)
		return this->end() - this->begin();
	return 0;
	}

bool ZDBSearch::empty() const
	{
	// We're empty if we have no index or if begin() == end()
	return fIndex == nil || this->begin() == this->end();
	}

const ZDBIndex::iterator& ZDBSearch::begin() const
	{
	if (fIndex && !fCachedBeginIterator.GetIndex())
		{
		// We have an index, and our cached begin iterator has not yet been set up.
		vector<pair<ZBTree::Handle, long> > theStack;
		switch (fSearchType)
			{
			case ZDBSearch::eAll:
			case ZDBSearch::eLessEqual:
			case ZDBSearch::eLess:
				this->Internal_SetupStack_FarLeft(theStack);
				break;

			case ZDBSearch::eEqual:
			case ZDBSearch::eGreaterEqual:
				this->Internal_SetupStack_FirstEqual(theStack);
				break;

			case ZDBSearch::eGreater:
				this->Internal_SetupStack_FirstGreater(theStack);
				break;

			default:
				ZDebugStopf(0, ("ZDBSearch::begin, invalid search type"));
				throw runtime_error("ZDBSearch::begin, invalid search type");
			}
		const_cast<ZDBSearch*>(this)->fCachedBeginIterator = ZDBIndex::iterator(fIndex, theStack);
		}
	return fCachedBeginIterator;
	}

const ZDBIndex::iterator& ZDBSearch::end() const
	{
	if (fIndex && !fCachedEndIterator.GetIndex())
		{
		// We have an index, and our cached end iterator has not yet been set up.
		vector<pair<ZBTree::Handle, long> > theStack;
		switch (fSearchType)
			{
			case ZDBSearch::eAll:
			case ZDBSearch::eGreater:
			case ZDBSearch::eGreaterEqual:
				this->Internal_SetupStack_FarRight(theStack);
				break;

			case ZDBSearch::eEqual:
			case ZDBSearch::eLessEqual:
				this->Internal_SetupStack_FirstGreater(theStack);
				break;

			case ZDBSearch::eLess:
				this->Internal_SetupStack_FirstEqual(theStack);
				break;

			default:
				ZDebugStopf(0, ("ZDBSearch::begin, invalid search type"));
				throw runtime_error("ZDBSearch::begin, invalid search type");
			}
		const_cast<ZDBSearch*>(this)->fCachedEndIterator = ZDBIndex::iterator(fIndex, theStack);
		}
	return fCachedEndIterator;
	}

ZRef<ZDBRecord> ZDBSearch::front() const
	{
	return *this->begin();
	}

ZRef<ZDBRecord> ZDBSearch::back() const
	{
	return *(this->end() - 1);
	}

void ZDBSearch::DeleteFoundSet()
	{
	if (fIndex)
		{
		// To delete the caller has to have acquired our write lock.
		ZAssert(fIndex->GetLock()->CanWrite());

		// Hmmm. Gonna be inefficient
		while (!this->empty())
			{
			ZRef<ZDBRecord> theRecord = this->front();
			fIndex->GetTable()->DeleteRecord(theRecord);
			}
		}
	}

void ZDBSearch::GetFoundSetIDs(vector<ZDBRecordID>& outIDVector)
	{
	outIDVector.erase(outIDVector.begin(), outIDVector.end());
	for (ZDBIndex::iterator theIter = this->begin(); theIter != this->end(); ++theIter)
		outIDVector.push_back((*theIter)->GetRecordID());
	}

void ZDBSearch::Internal_SetupStack_FirstEqual(vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	ZRef<ZBTree::Node> rootNode = fIndex->fNodeSpace->LoadNode(fIndex->fNodeSpace->GetRootHandle());
	if (!rootNode)
		return;

	if (rootNode->GetItemCount() == 0)
		return;

	ZRef<ZBTree::Node> currentNode(rootNode);
	while (true)
		{
		long result;
		long thePosition = currentNode->FindPosition_LowestEqualOrGreater(fSearchItem, result);

		outStack.push_back(make_pair(currentNode->GetHandle(), thePosition));
		if (/*result == 0 ||*/ currentNode->GetIsLeaf()) // -ec 00.04.12
			break;
		currentNode = fIndex->fNodeSpace->LoadNode(currentNode->GetHandleAt(thePosition));
		}

	this->Internal_CleanUpStack(outStack);
	}

void ZDBSearch::Internal_SetupStack_FirstGreater(vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	// We take advantage of the fact that if there is a transition between item values, it *will* happen at a leaf
	// node. The distance between node item values is least in leaf nodes, and progressively greater as we walk
	// up the tree.
	ZRef<ZBTree::Node> rootNode = fIndex->fNodeSpace->LoadNode(fIndex->fNodeSpace->GetRootHandle());
	if (!rootNode)
		return;

	if (rootNode->GetItemCount() == 0)
		return;

	ZRef<ZBTree::Node> currentNode(rootNode);
	while (true)
		{
		long result;
		long thePosition = currentNode->FindPosition_LowestGreater(fSearchItem, result);

		if (currentNode->GetIsLeaf())
			{
			outStack.push_back(make_pair(currentNode->GetHandle(), thePosition));
			break;
			}
		outStack.push_back(make_pair(currentNode->GetHandle(), thePosition));
		currentNode = fIndex->fNodeSpace->LoadNode(currentNode->GetHandleAt(thePosition));
		}

	this->Internal_CleanUpStack(outStack);
	}

void ZDBSearch::Internal_SetupStack_FarLeft(vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	// Walk down the tree to the extreme leftmost node and item
	ZRef<ZBTree::Node> rootNode = fIndex->fNodeSpace->LoadNode(fIndex->fNodeSpace->GetRootHandle());
	if (!rootNode)
		return;

	if (rootNode->GetItemCount() == 0)
		return;

	ZRef<ZBTree::Node> currentNode(rootNode);
	while (true)
		{
		outStack.push_back(make_pair(currentNode->GetHandle(), 0L));
		if (currentNode->GetIsLeaf())
			break;
		// Go down the leftmost branch
		currentNode = fIndex->fNodeSpace->LoadNode(currentNode->GetHandleAt(0));
		}
	// Don't need to clean up
	}

void ZDBSearch::Internal_SetupStack_FarRight(vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	// The "far right" is an empty stack
	}

void ZDBSearch::Internal_SetupStack_Rank(size_type targetRank, vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	ZRef<ZBTree::Node> rootNode = fIndex->fNodeSpace->LoadNode(fIndex->fNodeSpace->GetRootHandle());
	if (!rootNode)
		return;

	if (rootNode->GetItemCount() == 0)
		return;

	outStack.push_back(make_pair(rootNode->GetHandle(), 0L));
	size_type currentRank = 0;
	while (currentRank != targetRank)
		{
		ZRef<ZBTree::Node> currentNode(fIndex->fNodeSpace->LoadNode(outStack.back().first));
		if (currentNode->GetIsLeaf())
			{
			// If we've hit a leaf node, then we've descended into it, and our
			// remaining distance to move must be able to be accomodated
			// by the items in the node. However, it *is* legal to refer to the item
			// that would be at this[this->size()], although we can't dereference
			// it. This will generate an empty stack when we do the clean up below
			outStack.back().second += targetRank-currentRank;
			currentRank = targetRank;
			}
		else
			{
			ZRef<ZBTree::Node> subNode = fIndex->fNodeSpace->LoadNode(currentNode->GetHandleAt(outStack.back().second));
			currentRank += subNode->GetSubTreeSize();
			if (currentRank > targetRank)
				{
				outStack.push_back(make_pair(subNode->GetHandle(), 0L));
				currentRank -= subNode->GetSubTreeSize();
				}
			else if (currentRank < targetRank)
				{
				++outStack.back().second;
				++currentRank;
				}
			}
		}
	this->Internal_CleanUpStack(outStack);
	}

void ZDBSearch::Internal_CleanUpStack(vector<pair<ZBTree::Handle, long> >& outStack) const
	{
	while (outStack.size() > 0)
		{
		// If we've run off the end of the current node, then pop it and loop again
		ZRef<ZBTree::Node> currentNode = fIndex->fNodeSpace->LoadNode(outStack.back().first);
		if (outStack.back().second < currentNode->GetItemCount())
			break;
		outStack.pop_back();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTable

ZDBTable::ZDBTable(ZDBDatabase* inDatabase, ZBlockStore::BlockID inBlockID)
:	fLoadMutex("ZDBTable::fLoadMutex"), fHeaderBlockID(inBlockID), fNextRecordID(0), fDatabase(inDatabase)
	{
	fFullySpanningIndices = false;
	fRecordCount = 0;
	fVersion = 0;
	fDirty = false;
	this->LoadUp();
	}

ZDBTable::ZDBTable(ZDBDatabase* inDatabase, const string& inTableName,
					const vector<ZDBFieldSpec>& inFieldSpecList, bool inFullySpanningIndices, bool inCreateIDs)
:	fLoadMutex("ZDBTable::fLoadMutex"), fTableName(inTableName), fFieldSpecList(inFieldSpecList), fDatabase(inDatabase)
	{
	fFullySpanningIndices = inFullySpanningIndices;
	fRecordCount = 0;
	fVersion = 1;
	fDirty = false;
	fDatabase->GetBlockStore()->Create(fHeaderBlockID);
	// The indicator that we are assigning IDs is that fNextRecordID is non-zero
	if (inCreateIDs)
		{
		fNextRecordID = 1;

		// If the user did not already include an __ID field, then add one
		bool gotIt = false;
		for (vector<ZDBFieldSpec>::const_iterator theIter = inFieldSpecList.begin(); theIter != inFieldSpecList.end(); ++theIter)
			{
			if ((*theIter).GetFieldName() == ZDBFieldName::ID)
				{
				gotIt = true;
				break;
				}
			}
		// We have to put the record ID at the *front* of the list
		if (!gotIt)
			fFieldSpecList.insert(fFieldSpecList.begin(), ZDBFieldSpec(ZDBFieldName::ID, ZDBFieldRep::eID));
		// And create an index with the ID as the prefix. If we're fully spanning, then all the other fields
		// will also end up in the index, otherwise it'll be just the ID
		this->CreateIndex(ZDBFieldName::ID, nil);
		}
	else
		fNextRecordID = 0;

	this->WriteBack();
	}

ZDBTable::~ZDBTable()
	{
	// This flush is superfluous, and trips an assertion that we cannot get a read lock -- ct 2001.08.21 via phone from ag
	//this->Flush();

	// We're the sole owners of indices, others should only take a temporary
	// reference to them
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		delete *i;

	// Records, however, can be used by many others. The ZRefCounted destructor
	// does a ZAssert on its ref count, so when we delete one of these records, that's
	// still ZRefed by someone, that's the assertion that will trip.
	for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end(); ++i)
		delete *i;

	ZAssertStop(2, !fDirty);
	}

bool ZDBTable::Validate(ZProgressWatcher* iProgressWatcher)
	{
	ZProgressPusher thePusher(iProgressWatcher, "table \""+fTableName+"\"", 1);

	bool allOkay = true;

	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); allOkay && i != fIndexList.end(); ++i)
		{
		string indexName = ZDBIndex::sIndexNameToString((*i)->GetIndexedFieldNames());
		
		ZProgressPusher innerPusher(iProgressWatcher, "index " + indexName, fRecordCount);

		long foundRecordCount = 0;
		if ((*i)->AllNodesPresentAndCorrect(foundRecordCount, nil, nil, iProgressWatcher))
			{
			// The index may have been able to read all its nodes, but it might have the wrong
			// total number of records, in which case there's clearly been damage
			if (foundRecordCount != fRecordCount)
				allOkay = false;
			}
		else
			{
			allOkay = false;
			}
		}

	return allOkay;
	}

void ZDBTable::CopyInto(ZProgressWatcher* iProgressWatcher, ZDBTable* iTable)
	{
	ZProgressPusher thePusher(iProgressWatcher, "table \"" + fTableName + "\"", 2);

	// Check each index to see if we can load all of its nodes. At the same time build a list
	// of all record block IDs referenced by valid nodes in the indices
	vector<ZDBIndex*> okayIndices;

	set<ZBlockStore::BlockID> allRecordBlockIDs;
	set<ZDBRecord*, ZDBRecord::CompareForCheck> allDBRecords;
	{
	ZProgressPusher innerPusher(iProgressWatcher, "walk indices", fIndexList.size());

	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		set<ZBlockStore::BlockID>* foundRecordBlockIDs = nil;
		set<ZDBRecord*, ZDBRecord::CompareForCheck>* foundDBRecords = nil;
		// If we don't have any good indices yet then we need to gather records
		if (okayIndices.size() == 0)
			{
			foundRecordBlockIDs = new set<ZBlockStore::BlockID>;
			foundDBRecords = new set<ZDBRecord*, ZDBRecord::CompareForCheck>;
			}
		string indexName = ZDBIndex::sIndexNameToString((*i)->GetIndexedFieldNames());
		
		thePusher.Push("index "+indexName, fRecordCount);

		long foundRecordCount = 0;
		if ((*i)->AllNodesPresentAndCorrect(foundRecordCount, foundRecordBlockIDs, foundDBRecords, iProgressWatcher))
			{
			// The index may have been able to read all its nodes, but it might have the wrong
			// total number of records, in which case we should regenerate it
			if (foundRecordCount == fRecordCount)
				okayIndices.push_back(*i);
			}

		thePusher.Pop();

		if (okayIndices.empty())
			{
			// Stick the records found for this index in our master list
			if (foundRecordBlockIDs)
				{
				for (set<ZBlockStore::BlockID>::iterator j = foundRecordBlockIDs->begin(); j != foundRecordBlockIDs->end(); ++j)
					allRecordBlockIDs.insert(*j);
				delete foundRecordBlockIDs;
				}

			if (foundDBRecords)
				{
				for (set<ZDBRecord*, ZDBRecord::CompareForCheck>::iterator j = foundDBRecords->begin(); j != foundDBRecords->end(); ++j)
					{
					ZDBRecord* theRecord = *j;
					if (allDBRecords.find(theRecord) == allDBRecords.end())
						allDBRecords.insert(theRecord);
					else
						delete theRecord;
					}
				delete foundDBRecords;
				}
			}
		else
			{
			if (foundRecordBlockIDs)
				delete foundRecordBlockIDs;

			if (foundDBRecords)
				{
				ZUtil_STL::sDeleteAll(foundDBRecords->begin(), foundDBRecords->end());
				delete foundDBRecords;
				}

			allRecordBlockIDs.erase(allRecordBlockIDs.begin(), allRecordBlockIDs.end());

			ZUtil_STL::sDeleteAll(allDBRecords.begin(), allDBRecords.end());
			allDBRecords.erase(allDBRecords.begin(), allDBRecords.end());
			}
		}

	} // End of innerPusher scope

	{
	ZProgressPusher innerPusher(iProgressWatcher, "transcribe contents", 1);
	if (okayIndices.empty())
		{
		if (fFullySpanningIndices)
			{
			ZProgressPusher copyPusher(iProgressWatcher, "Copy records (allDBRecords)", allDBRecords.size());

			for (set<ZDBRecord*, ZDBRecord::CompareForCheck>::iterator i = allDBRecords.begin(); i != allDBRecords.end(); ++i)
				{
				ZRef<ZDBRecord> newRecord = iTable->CreateRecord(true);
				newRecord->CopyFrom(*i, true);
				delete *i;
				newRecord->RecordPostRead();
				copyPusher.Process(1);
				}
			}
		else
			{
			ZProgressPusher copyPusher(iProgressWatcher, "Copy records (allRecordBlockIDs)", allRecordBlockIDs.size());

			for (set<ZBlockStore::BlockID>::iterator i = allRecordBlockIDs.begin(); i != allRecordBlockIDs.end(); ++i)
				{
				ZRef<ZDBRecord> existingRecord = this->LoadRecord(*i);
				ZRef<ZDBRecord> newRecord = iTable->CreateRecord(true);
				newRecord->CopyFrom(existingRecord.GetObject(), true);
				newRecord->RecordPostRead();
				copyPusher.Process(1);
				}
			}
		}
	else
		{
		ZDBSearch theSearch(okayIndices.front());
		ZProgressPusher copyPusher(iProgressWatcher, "Copy records", theSearch.size());
		for (ZDBSearch::iterator existingIter = theSearch.begin(); existingIter != theSearch.end(); ++existingIter)
			{
			ZRef<ZDBRecord> newRecord = iTable->CreateRecord(true);
			newRecord->CopyFrom((*existingIter).GetObject(), true);
			newRecord->RecordPostRead();
			copyPusher.Process(1);
			}
		}
	} // End of innerPusher scope

	iTable->FixNextRecordID();
	}

void ZDBTable::CreateIndex(ZDBFieldName inFieldName, ZProgressWatcher* iProgressWatcher)
	{
	ZLocker locker(this->GetLock()->GetWrite());

	vector<ZDBFieldName> theFieldsToIndex;
	theFieldsToIndex.push_back(inFieldName);
	this->CreateIndex(theFieldsToIndex, iProgressWatcher);
	}

void ZDBTable::CreateIndex(const vector<ZDBFieldName>& inFieldsToIndex, ZProgressWatcher* iProgressWatcher)
	{
	vector<vector<ZDBFieldName> > theIndicesToCreate;
	theIndicesToCreate.push_back(inFieldsToIndex);
	this->CreateIndices(theIndicesToCreate, iProgressWatcher);
	}

void ZDBTable::CreateIndices(const vector<vector<ZDBFieldName> >& inIndexSpecs, ZProgressWatcher* iProgressWatcher)
	{
	ZLocker locker(this->GetLock()->GetWrite());

	ZProgressPusher thePusher(iProgressWatcher, "create indices", fRecordCount*inIndexSpecs.size());

	// Note that the order of the fields in theFieldsToIndex is *very* important. It indicates primary, secondary...
	// ordering.
	vector<ZDBIndex*> createdIndices;
	for (vector<vector<ZDBFieldName> >::const_iterator indexIterator = inIndexSpecs.begin(); indexIterator != inIndexSpecs.end(); ++indexIterator)
		{
		vector<ZDBFieldName> localFieldsToIndex = *indexIterator;
		// If we're fully spanning we have to insert any fields missing from the passed in vector
		if (fFullySpanningIndices)
			{
			for (vector<ZDBFieldSpec>::iterator i = fFieldSpecList.begin(); i != fFieldSpecList.end(); ++i)
				{
				bool fieldIncluded = false;
				for (vector<ZDBFieldName>::const_iterator j = localFieldsToIndex.begin(); j !=localFieldsToIndex.end(); ++j)
					{
					if ((*i).GetFieldName() == *j)
						{
						fieldIncluded = true;
						break;
						}
					}
				if (!fieldIncluded)
					localFieldsToIndex.push_back((*i).GetFieldName());
				}
			}

		bool alreadyExists = false;
		for (size_t x = 0; x < fIndexList.size(); ++x)
			{
			if (localFieldsToIndex == fIndexList[x]->GetIndexedFieldNames())
				{
				alreadyExists = true;
				break;
				}
			}

		if (!alreadyExists)
			{
			// Check to see that we have the fields that the index should operate on
			long totalLikelySize = 0;
			bool allFieldsExist = true;
			for (vector<ZDBFieldName>::const_iterator i = localFieldsToIndex.begin(); i !=localFieldsToIndex.end(); ++i)
				{
				bool fieldExists = false;
				for (vector<ZDBFieldSpec>::iterator j = fFieldSpecList.begin(); j != fFieldSpecList.end(); ++j)
					{
					if ((*j).GetFieldName() == *i)
						{
						fieldExists = true;
						switch((*j).GetFieldType())
							{
							case ZDBFieldRep::eNumber: totalLikelySize += sizeof(long); break;
							case ZDBFieldRep::eString:
							case ZDBFieldRep::eIString: totalLikelySize += 32; break;
							case ZDBFieldRep::eBinary: totalLikelySize += 32; break;
							case ZDBFieldRep::eBLOB: totalLikelySize += sizeof(ZBlockStore::BlockID); break;
							case ZDBFieldRep::eID: totalLikelySize += ZDBRecordID::sSizeInStream(); break;
							case ZDBFieldRep::eFloat: totalLikelySize += sizeof(float); break;
							case ZDBFieldRep::eDouble: totalLikelySize += sizeof(double); break;
							default: ZDebugStopf(0, ("Invalid field type")); break;
							}
						break;
						}
					}
				if (!fieldExists)
					{
					allFieldsExist = false;
					break;
					}
				}
			ZAssert(allFieldsExist == true);
			ZAssert(totalLikelySize != 0);
			// If we're not fully spanning, then each item is also going to record the block ID of its record
			if (!fFullySpanningIndices)
				totalLikelySize += sizeof(ZBlockStore::BlockID);
	
			// Work out what would be a good degree for this set of fields. We try to fill a primary slot,
			// with a minimum of 4 and a max of 127
			long orderOfNodeSpace = this->GetBlockStore()->EfficientSize() / totalLikelySize;
			long degreeOfNodeSpace = (orderOfNodeSpace-1)/2;
			if (degreeOfNodeSpace < 4)
				degreeOfNodeSpace = 4;
			else if (degreeOfNodeSpace > 127)
				degreeOfNodeSpace = 127;
	
			ZDBIndex* theIndex = this->MakeIndex(localFieldsToIndex, degreeOfNodeSpace);
			createdIndices.push_back(theIndex);
			fIndexList.push_back(theIndex);
			}
		}

	// If we have any records then spool through them adding them to the new index
	if (fRecordCount > 0)
		{
		ZDBSearch theSearch(fIndexList[0]);
		for (ZDBIndex::iterator theIter = theSearch.begin(); theIter != theSearch.end(); ++theIter)
			{
			ZRef<ZDBRecord> theRecord(*theIter);
			for (vector<ZDBIndex*>::iterator j = createdIndices.begin(); j != createdIndices.end(); ++j)
				{
				thePusher.Process(1);
				(*j)->RecordPostRead(theRecord.GetObject());
				}
			}
		}
	fDirty = true;
	}

ZDBIndex* ZDBTable::GetIndex(ZDBFieldName inFieldName)
	{
	vector<ZDBFieldName> theFields;
	theFields.push_back(inFieldName);
	return this->GetIndex(theFields);
	}

ZDBIndex* ZDBTable::GetIndex(const vector<ZDBFieldName>& inFieldNames)
	{
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnTheseFieldsPrecisely(inFieldNames))
			return *i;
		}
	return nil;
	}

void ZDBTable::DeleteIndex(ZDBIndex* inIndex)
	{
	ZLocker locker(this->GetLock()->GetWrite());

	vector<ZDBIndex*>::iterator foundIter = fIndexList.end();
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if (*i == inIndex)
			{
			foundIter = i;
			break;
			}
		}

	ZAssert(foundIter != fIndexList.end());
	fIndexList.erase(foundIter);
	inIndex->DeleteYourself();
	fDirty = true;
	}

ZDBFieldRep::FieldType ZDBTable::GetFieldType(const ZDBFieldName& inName)
	{
	for (vector<ZDBFieldSpec>::const_iterator x = fFieldSpecList.begin(); x != fFieldSpecList.end(); ++x)
		{
		if (x->GetFieldName() == inName)
			return x->GetFieldType();
		}
	return ZDBFieldRep::eInvalid;
	}

ZDBFieldRep* ZDBTable::CreateFieldRepByType(ZDBFieldRep::FieldType inType)
	{
	ZDBFieldRep* theRep = nil;
	switch (inType)
		{
		case ZDBFieldRep::eBinary:
			theRep = new ZDBFieldRepBinary; break;
		case ZDBFieldRep::eID:
			theRep = new ZDBFieldRepID; break;
		case ZDBFieldRep::eNumber:
			theRep = new ZDBFieldRepNumber; break;
		case ZDBFieldRep::eString:
			theRep = new ZDBFieldRepString; break;
		case ZDBFieldRep::eIString:
			theRep = new ZDBFieldRepIString; break;
		case ZDBFieldRep::eBLOB:
			theRep = new ZDBFieldRepBLOB; break;
		case ZDBFieldRep::eFloat:
			theRep = new ZDBFieldRepFloat; break;
		case ZDBFieldRep::eDouble:
			theRep = new ZDBFieldRepDouble; break;
		default:
			ZUnimplemented();
		}
	return theRep;
	}

ZDBFieldRep* ZDBTable::CreateFieldRepByName(const ZDBFieldName& inName)
	{
	ZDBFieldRep* theRep = nil;
	ZDBFieldRep::FieldType fieldType = this->GetFieldType(inName);
	switch (fieldType)
		{
		case ZDBFieldRep::eBinary:
			theRep = new ZDBFieldRepBinary; break;
		case ZDBFieldRep::eID:
			theRep = new ZDBFieldRepID; break;
		case ZDBFieldRep::eNumber:
			theRep = new ZDBFieldRepNumber; break;
		case ZDBFieldRep::eString:
			theRep = new ZDBFieldRepString; break;
		case ZDBFieldRep::eIString:
			theRep = new ZDBFieldRepIString; break;
		case ZDBFieldRep::eBLOB:
			theRep = new ZDBFieldRepBLOB; break;
		case ZDBFieldRep::eFloat:
			theRep = new ZDBFieldRepFloat; break;
		case ZDBFieldRep::eDouble:
			theRep = new ZDBFieldRepDouble; break;
		default:
			ZUnimplemented();
		}
	return theRep;
	}

// first see if this table has a field named inName and if it is compatible with inType.
// if so, return appropriate ZDBFieldRep*.
// if not, return ZDBFieldRep* of type inType.
ZDBFieldRep* ZDBTable::CreateFieldRep(const ZDBFieldName& inName, ZDBFieldRep::FieldType inType)
{
	ZDBFieldRep* theRep = nil;
	ZDBFieldRep::FieldType fieldType = this->GetFieldType(inName);

	if (fieldType == ZDBFieldRep::eInvalid)
		{
		fieldType = inType;
		}
	else if (fieldType != inType)
		{
		if (inType == ZDBFieldRep::eString && fieldType == ZDBFieldRep::eIString)
			{
			theRep = new ZDBFieldRepIString;
			return theRep;
			}
		else if (inType == ZDBFieldRep::eIString && fieldType == ZDBFieldRep::eString)
			{
			theRep = new ZDBFieldRepString;
			return theRep;
			}
		}
	else // play it safe, and go with inType.
		fieldType = inType;

	switch (fieldType)
		{
		case ZDBFieldRep::eBinary:
			theRep = new ZDBFieldRepBinary; break;
		case ZDBFieldRep::eID:
			theRep = new ZDBFieldRepID; break;
		case ZDBFieldRep::eNumber:
			theRep = new ZDBFieldRepNumber; break;
		case ZDBFieldRep::eString:
			theRep = new ZDBFieldRepString; break;
		case ZDBFieldRep::eIString:
			theRep = new ZDBFieldRepIString; break;
		case ZDBFieldRep::eBLOB:
			theRep = new ZDBFieldRepBLOB; break;
		case ZDBFieldRep::eFloat:
			theRep = new ZDBFieldRepFloat; break;
		case ZDBFieldRep::eDouble:
			theRep = new ZDBFieldRepDouble; break;
		default:
			ZUnimplemented();
		}
	return theRep;
	}

void ZDBTable::MakeSpace()
	{
	// We don't increment the i pointer in the for loop, because we may not want to advance it if
	// we end up deleting the thing it references
	for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end();)
		{
		// Is this record unreferenced?
		if ((*i)->GetRefCount() == 0)
			{
			// So, see if we need to flush it out
			ZBlockStore::BlockID theBlockID = (*i)->GetBlockID();
			if (theBlockID && (*i)->IsDirty())
				{
				if (ZRef<ZStreamerWPos> theStreamer = fDatabase->GetBlockStore()->OpenWPos(theBlockID))
					{
					const ZStreamWPos& theStream = theStreamer->GetStreamWPos();
					(*i)->ToStream(theStream);
					theStream.Truncate();
					}
				else
					{
					ZDebugLogf(0, ("Couldn't write to block %X", theBlockID));
					}
				// Mark the record as clean
				(*i)->MarkClean();
				}
			// delete the object
			delete *i;
			// Remove the entry from our list
			fLoadedRecords.erase(i);
			}
		else
			{
			// We only move on to the next entry if we haven't deleted the current entry
			++i;
			}
		}
	}

ZBlockStore::BlockID ZDBTable::GetHeaderBlockID()
	{ return fHeaderBlockID; }

ZBlockStore* ZDBTable::GetBlockStore()
	{ return fDatabase->GetBlockStore(); }

ZRef<ZDBRecord> ZDBTable::CreateRecord(bool inCreatePulledFromIndices)
	{
	ZLocker locker1(this->GetLock()->GetWrite());
	ZLocker locker2(fLoadMutex);

	// We only need to create a block on disk if we're not creating fully spanning indices
	ZBlockStore::BlockID newRecordBlockID = 0;
	if (!fFullySpanningIndices)
		fDatabase->GetBlockStore()->Create(newRecordBlockID);

	// We pass in our record ID, which will be zero if we're not actually auto creating record IDs
	ZDBRecord* newRecord = this->MakeRecord(newRecordBlockID, fFieldSpecList, fNextRecordID, inCreatePulledFromIndices);

	// Don't increment the next record ID unless it's already non-zero
	if (fNextRecordID != 0)
		fNextRecordID += 1;
	fRecordCount += 1;
	fLoadedRecords.push_back(newRecord);

	fDirty = true;
	if (!inCreatePulledFromIndices)
		{
		for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
			(*i)->AddedNewRecord(newRecord);
		}

	return newRecord;
	}

void ZDBTable::DeleteRecord(ZRef<ZDBRecord> inRecord)
	{
	ZLocker locker1(this->GetLock()->GetWrite());
	ZLocker locker2(fLoadMutex);

	// It's gotta be a real record
	ZAssert(inRecord);

	// Tell our indices to remove any reference to this record
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->DeletedRecord(inRecord.GetObject());

	// And tell the record to remove itself from our block file (as well as any BLOB fields
	// it may have). If the record is virtual, ie it was fabricated from fields kept in
	// an index, then it won't have allocated any storage on disk (although a BLOB might)
	// and it won't have a block to release.
	inRecord->DeleteYourself();

	// Remove the record from our loaded record cache
	bool doneIt = false;
	for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end(); ++i)
		{
		if ((*i) == inRecord.GetObject())
			{
			fLoadedRecords.erase(i);
			doneIt = true;
			break;
			}
		}
	ZAssert(doneIt);
	ZAssert(fRecordCount > 0);
	--fRecordCount;
	fDirty = true;
	}

ZRef<ZDBRecord> ZDBTable::GetRecordByID(const ZDBRecordID& inID)
	{
	// We can only retrieve records by ID if we're creating records with IDs
	ZAssert(fNextRecordID != 0);
	ZDBSearchSpec theSearchSpec(ZDBFieldName::ID, new ZDBFieldRepID(inID));
	ZDBSearch theSearch = this->Search(theSearchSpec, ZDBSearch::eEqual);
	if (!theSearch.empty())
		return theSearch.front();
	return ZRef<ZDBRecord>();
	}

bool ZDBTable::FixNextRecordID()
	{
	try
		{
		// If we generate record IDs then ensure that fNextRecordID is greater than the largest
		// in any extant records.
		if (fNextRecordID != 0)
			{
			ZDBSearch search_lastID = this->Search(ZDBSearchSpec(ZDBFieldName::ID, new ZDBFieldRepID), ZDBSearch::eAll);
			if (!search_lastID.empty())
				{
				fNextRecordID = search_lastID.back()->GetRecordID();
				fNextRecordID += 1; // it would be nice to have operator++ defined on ZDBRecordID.
				}
			}
		}
	catch (exception& theEx)
		{
		return false; // something went wrong.
		}
	return true;
	}

ZDBSearch ZDBTable::Search(const ZDBSearchSpec& inSearchSpec, ZDBSearch::SearchType inSearchType )
	{
	vector<ZDBSearchSpec> theSearchSpecVector;
	theSearchSpecVector.push_back(inSearchSpec);
	return this->Search(theSearchSpecVector, inSearchType);
	}

ZDBSearch ZDBTable::Search(const vector<ZDBSearchSpec>& inSearchSpecs, ZDBSearch::SearchType inSearchType )
	{
	// Find the index that matches the search specs
	vector<ZDBFieldName> theFieldNames;
	for (vector<ZDBSearchSpec>::const_iterator theSpecIterator = inSearchSpecs.begin();
						theSpecIterator != inSearchSpecs.end(); ++theSpecIterator)
		theFieldNames.push_back(theSpecIterator->GetFieldName());

	// First, find an index who indexes on our search expression precisely
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnTheseFieldsPrecisely(theFieldNames))
			return ZDBSearch((*i), inSearchSpecs, inSearchType);
		}
	// If that fails, use an index where our search expression is a proper prefix
	// of the index's field list
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnThisFieldListPrefix(theFieldNames))
			return ZDBSearch((*i), inSearchSpecs, inSearchType);
		}
	return ZDBSearch();
	}

ZDBSearch ZDBTable::Search(const ZDBSearchSpec& inSearchSpec, const ZDBFieldName& inFieldName)
	{
	vector<ZDBSearchSpec> theSearchSpecVector;
	theSearchSpecVector.push_back(inSearchSpec);
	vector<ZDBFieldName> theFieldNames;
	theFieldNames.push_back(inFieldName);
	return this->Search(theSearchSpecVector, theFieldNames);
	}

ZDBSearch ZDBTable::Search(const ZDBSearchSpec& inSearchSpec, const vector<ZDBFieldName>& inFieldNames)
	{
	vector<ZDBSearchSpec> theSearchSpecVector;
	theSearchSpecVector.push_back(inSearchSpec);
	return this->Search(theSearchSpecVector, inFieldNames);
	}

ZDBSearch ZDBTable::Search(const vector<ZDBSearchSpec>& inSearchSpecs, const ZDBFieldName& inFieldName)
	{
	vector<ZDBFieldName> theFieldNames;
	theFieldNames.push_back(inFieldName);
	return this->Search(inSearchSpecs, theFieldNames);
	}

ZDBSearch ZDBTable::Search(const vector<ZDBSearchSpec>& inSearchSpecs, const vector<ZDBFieldName>& inFieldNames)
	{
	// Find the index that matches the search specs
	vector<ZDBFieldName> theFieldNames;
	for (vector<ZDBSearchSpec>::const_iterator theSpecIterator = inSearchSpecs.begin();
						theSpecIterator != inSearchSpecs.end(); ++theSpecIterator)
		theFieldNames.push_back(theSpecIterator->GetFieldName());

	// Push "floating" field name onto list to help guarantee correct index is selected.
	for (vector<ZDBFieldName>::const_iterator theFieldNameIterator = inFieldNames.begin();
						theFieldNameIterator != inFieldNames.end(); ++theFieldNameIterator)
		theFieldNames.push_back(*theFieldNameIterator);

	// First, find an index who indexes on our search expression precisely
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnTheseFieldsPrecisely(theFieldNames))
			return ZDBSearch((*i), inSearchSpecs, ZDBSearch::eEqual);
		}
	// If that fails, use an index where our search expression is a proper prefix
	// of the index's field list
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnThisFieldListPrefix(theFieldNames))
			return ZDBSearch((*i), inSearchSpecs, ZDBSearch::eEqual);
		}
	return ZDBSearch();
	}

ZDBSearch ZDBTable::SearchAll()
	{
	// Get the first index we come across, which will be the index by __ID, if we have one
	// Note that this means that you cannot iterate over all records *unless* you have an index
	// Of course, if you don't have any indices, then the table's pretty much useless anyway
	if (fIndexList.size() > 0)
		return ZDBSearch(*fIndexList.begin());
	ZDebugStopf(0, ("ZDBTable::SearchAll, yes we have no indices today!!!"));
	return ZDBSearch();
	}

ZDBSearch ZDBTable::SearchAll(const ZDBFieldName& inName)
	{
	vector<ZDBFieldName> theFieldNames;
	theFieldNames.push_back(inName);
	return this->SearchAll(theFieldNames);
	}


ZDBSearch ZDBTable::SearchAll(const vector<ZDBFieldName>& inFieldNames)
	{
	// First, find an index who indexes on our field names exactly
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnTheseFieldsPrecisely(inFieldNames))
			return ZDBSearch(*i);
		}
	// If that fails, use an index where our field names form a proper prefix
	// of the index's field list
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		{
		if ((*i)->DoYouIndexOnThisFieldListPrefix(inFieldNames))
			return ZDBSearch(*i);
		}
	return ZDBSearch();
	}

ZDBSearch ZDBTable::SearchAll(ZDBIndex* inIndexToUse)
	{
	// Return an iterator that will walk all records as ordered by the index that's
	// been passed in. Do a sanity check to make sure it is one of ours.
	ZAssert(inIndexToUse != nil);
	ZAssert(inIndexToUse->GetTable() == this);
	return ZDBSearch(inIndexToUse);
	}

long ZDBTable::GetRecordCount() const
	{ return fRecordCount; }

void ZDBTable::SetTableName(const string& inNewName)
	{
	if (fTableName == inNewName)
		return;
	ZLocker locker(this->GetLock()->GetWrite());
	fTableName = inNewName;
	fLock.SetName(fTableName.c_str()); // for debugging. -ec 01.02.23
	fDirty = true;
	}

void ZDBTable::Flush()
	{
	ZAssertStop(kDebug_ZDBase, fLock.CanRead());
	ZLocker loadLocker(fLoadMutex);

	// Now, walk our loaded records, writing them out if necessary
	if (!fFullySpanningIndices)
		{
		for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end(); ++i)
			{
			ZBlockStore::BlockID theID = (*i)->GetBlockID();
			if (theID && (*i)->IsDirty())
				{
				if (ZRef<ZStreamerWPos> theStreamer = fDatabase->GetBlockStore()->OpenWPos(theID))
					{
					(*i)->ToStream(ZStreamW_Buffered(1024, theStreamer->GetStreamW()));

					// Truncate the stream at the end of whatever's just been written
					theStreamer->GetStreamWPos().Truncate();
					}
				else
					{
					ZDebugLogf(0, ("Couldn't write to block %X", theID));
					}	

				// And note that our persistent copy is up to date
				(*i)->MarkClean();
				}
			}
		}
	loadLocker.Release();

	// Tell our indices to flush as well
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->Flush();

	// We don't do the same for the index header info, because that's handled below, as part of our own writeback
	if (fDirty)
		this->WriteBack();
	}

ZRWLock* ZDBTable::GetLock()
	{ return &fLock; }

void ZDBTable::ChangeFieldName(ZDBFieldName inOldName, ZDBFieldName inNewName)
	{
	ZAssert(inOldName != ZDBFieldName::ID);
	ZAssert(inNewName != ZDBFieldName::ID);
	ZLocker locker(this->GetLock()->GetWrite());
	for (vector<ZDBFieldSpec>::iterator i = fFieldSpecList.begin(); i != fFieldSpecList.end(); ++i)
		{
		if ((*i).GetFieldName() == inOldName)
			{
			(*i).SetFieldName(inNewName);
			break;
			}
		}

	for (vector<ZDBIndex*>::iterator j = fIndexList.begin(); j != fIndexList.end(); ++j)
		(*j)->ChangeFieldName(inOldName, inNewName);

	fDirty = true;
	}

void ZDBTable::GetIndexVector(vector<ZDBIndex*>& outIndexVector) const
	{
	// We don't return a reference as we may not always be using a vector internally
	outIndexVector = fIndexList;
	}

void ZDBTable::GetIndexSpecVector(vector<vector<ZDBFieldName> >& oIndexSpecs)
	{
	for (size_t x = 0; x < fIndexList.size(); ++x)
		oIndexSpecs.push_back(fIndexList[x]->GetIndexedFieldNames());
	}

bool ZDBTable::GetCreatingRecordIDs() const
	{ return fNextRecordID != 0; }

bool ZDBTable::GetIndicesSpan() const
	{ return fFullySpanningIndices; }

ZDBRecordID ZDBTable::GetNextRecordID() const
	{ return fNextRecordID; }

ZDBRecord* ZDBTable::MakeRecord(const ZDBTreeItem* inItem, bool inPulledFromIndices)
	{ return new ZDBRecord(this, inItem, inPulledFromIndices); }

ZDBRecord* ZDBTable::MakeRecord(ZBlockStore::BlockID inBlockID, const vector<ZDBFieldSpec>& inFieldSpecList)
	{ return new ZDBRecord(this, inBlockID, inFieldSpecList); }

ZDBRecord* ZDBTable::MakeRecord(ZBlockStore::BlockID inBlockID, const vector<ZDBFieldSpec>& inFieldSpecList,
													const ZDBRecordID& inRecordID, bool inCreatePulledFromIndices)
	{ return new ZDBRecord(this, inBlockID, inFieldSpecList, inRecordID, inCreatePulledFromIndices); }

ZDBIndex* ZDBTable::MakeIndex(const vector<ZDBFieldName>& inFieldsToIndex, long inDegreeOfNodeSpace)
	{ return new ZDBIndex(this, inFieldsToIndex, inDegreeOfNodeSpace); }

ZDBIndex* ZDBTable::MakeIndex()
	{ return new ZDBIndex(this); }

bool ZDBTable::CheckRecordLoadable(ZBlockStore::BlockID inHandle)
	{
	// If we're fully spanned, then this should never be called
	ZAssert(!fFullySpanningIndices);

	bool okay = true;
	try
		{
		ZRef<ZStreamerR> theStreamer = fDatabase->GetBlockStore()->OpenR(inHandle);
		if (!theStreamer)
			throw runtime_error(ZString::sFormat("ZDBTable::CheckRecordLoadable, block 0x%X isn't there.", inHandle));
			
		ZStreamR_Buffered theStream(1024, theStreamer->GetStreamR());
		long dummy = theStream.ReadInt32();
		// AG 98-02-11. Check that the record's format is valid by interpreting the data explicitly
		for (vector<ZDBFieldSpec>::const_iterator x = fFieldSpecList.begin(); okay && x != fFieldSpecList.end(); ++x)
			{
			switch ((*x).GetFieldType())
				{
				case ZDBFieldRep::eBinary:
					{
					size_t theLength = theStream.ReadUInt32();
					if (theLength > 1024 * 1024) // 1 megabyte
						throw runtime_error(ZString::sFormat("ZDBTable::CheckRecordLoadable, bad binary field length (block 0x%X).", inHandle));
					theStream.Skip(theLength);
					break;
					}
				case ZDBFieldRep::eID:
					{
					theStream.Skip(ZDBRecordID::sSizeInStream());
					break;
					}
				case ZDBFieldRep::eNumber:
					{
					long theLong = theStream.ReadInt32();
					break;
					}
				case ZDBFieldRep::eString:
				case ZDBFieldRep::eIString: //## shouldn't this be here for completeness? -ec 00.04.03
					{
					size_t theLength = theStream.ReadUInt32();
					if (theLength > 200000)
						throw runtime_error(ZString::sFormat("ZDBTable::CheckRecordLoadable, invalid string length (block 0x%X).", inHandle));
					theStream.Skip(theLength);
					break;
					}
				case ZDBFieldRep::eBLOB:
					{
					ZBlockStore::BlockID theBlockID = theStream.ReadUInt32();
					if (theBlockID)
						{
						if (!this->GetBlockStore()->OpenR(theBlockID))
							throw runtime_error(ZString::sFormat("ZDBTable::CheckRecordLoadable, bad BLOB field in block 0x%X, block 0x%X not allocated.", inHandle, theBlockID));
						}
					break;
					}
				case ZDBFieldRep::eFloat:
					{
					float theFloat = theStream.ReadFloat();
					break;
					}
				case ZDBFieldRep::eDouble:
					{
					double theDouble = theStream.ReadDouble();
					break;
					}
				}
			}
		}
	catch (...)
		{
		okay = false;
		}
	return okay;
	}

ZRef<ZDBRecord> ZDBTable::LoadRecord(ZBlockStore::BlockID inHandle)
	{
	// We have to be locked, cause we're about to modify our loaded records list
	// and we may screw ourselves if we try flushing in the meantime
	ZMutexLocker lock(fLoadMutex);

	// This is called by an index's iterator when it needs a record created and
	// loaded from disk. If all our indices span, then this will never be called.
	// Walk our list of loaded records
	for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end(); ++i)
		{
		if ((*i)->GetBlockID() == inHandle)
			return *i;
		}
	ZDBRecord* theRecord = this->MakeRecord(inHandle, fFieldSpecList);
	try
		{
		ZRef<ZStreamerR> theStreamer = fDatabase->GetBlockStore()->OpenR(inHandle);
		if (!theStreamer)
			throw runtime_error(ZString::sFormat("ZDBTable::LoadRecord, block 0x%X isn't there", inHandle));
		theRecord->FromStream(ZStreamR_Buffered(1024, theStreamer->GetStreamR()));
		fLoadedRecords.push_back(theRecord);
		}
	catch (...)
		{
		theRecord->MarkClean();
		delete theRecord;
		throw;
		}

	return theRecord;
	}

void ZDBTable::RecordBeingFinalized(ZDBRecord* inRecord)
	{
	ZMutexLocker lock(fLoadMutex);

	if (inRecord->GetRefCount() > 1)
		{
		inRecord->FinalizationComplete();
		return;
		}

	vector<ZDBRecord*>::iterator where = find(fLoadedRecords.begin(), fLoadedRecords.end(), inRecord);
	ZAssert(where != fLoadedRecords.end());

	fLoadedRecords.erase(where);

	// We've done all we need to do to finalize the record (pulled it from fLoadedRecords), so
	// call FinalizationComplete
	inRecord->FinalizationComplete();

	// Note that we're still protected by fLoadMutex here, even if another thread wants to
	// load the record we've just finalized it will not be able to start the process until we
	// release fLoadMutex.

	// Flush the record's contents if needed
	ZBlockStore::BlockID theBlockID = inRecord->GetBlockID();

	// If theBlockID is invalid, then it's been deleted, or it was never a real record ... virtual records for
	// tables with indices that all span all fields
	if (theBlockID && inRecord->IsDirty())
		{
		if (ZRef<ZStreamerWPos> theStreamer = fDatabase->GetBlockStore()->OpenWPos(theBlockID))
			{
			inRecord->ToStream(ZStreamW_Buffered(1024, theStreamer->GetStreamW()));

			// Truncate the streamer at the end of whatever's just been written
			theStreamer->GetStreamWPos().Truncate();
			}
		else
			{
			ZDebugLogf(0, ("Couldn't write to block %X", theBlockID));
			}	
		}

	// Ensure that the record's cache flag is okay, otherwise we trip an assertion in ~MCached
	inRecord->MarkClean();

	// This is the point at which we can decide whether to keep a cached copy of the record, or
	// just ditch it. We also have to do appropriate management in the routines which load
	// and create records.
	delete inRecord;
	}

void ZDBTable::RecordPreChange(ZDBRecord* inRecord, ZDBField* inField)
	{
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->RecordPreChange(inRecord, inField->GetFieldName());
	fDirty = true;
	}

void ZDBTable::RecordPostChange(ZDBRecord* inRecord, ZDBField* inField)
	{
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->RecordPostChange(inRecord, inField->GetFieldName());
	fDirty = true;
	}

void ZDBTable::RecordPreRead(ZDBRecord* inRecord)
	{
	// The record in question is about to have all of its fields modified, so
	// remove it from all our indices
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->RecordPreRead(inRecord);
	fDirty = true;
	}

void ZDBTable::RecordPostRead(ZDBRecord* inRecord)
	{
	// The record in question has had all its fields modified, so it can be
	// reinserted into our indices
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->RecordPostRead(inRecord);
	fDirty = true;
	}

ZRef<ZDBRecord> ZDBTable::LoadRecordFromBTreeItem(const ZDBTreeItem* inItem)
	{
	ZMutexLocker lock(fLoadMutex);

	if (fFullySpanningIndices)
		{
		// So, either all our records span, in which case we try to fulfill this from our cache
		// based on field values, otherwise instantiating a new record
		ZDBRecord* theRecord = this->MakeRecord(inItem);
		for (vector<ZDBRecord*>::iterator i = fLoadedRecords.begin(); i != fLoadedRecords.end(); ++i)
			{
			if ((*i)->IsIdenticalTo(theRecord))
				{
				// If this record is identical to (ie has every field exactly matching the one created from
				// the tree item) then we can return this cached record. This is what makes it possible for
				// a set of indices on a single table to all generate their content from tree items, and still
				// have only a single record instantiated to correspond to each entry in the indices
				delete theRecord;
				return *i;
				}
			}
		// We didn't have a loaded record that matches the one created from the btreeitem, but we do
		// now!
		fLoadedRecords.push_back(theRecord);
		return theRecord;
		}
	// Or not all indices span, in which case we just load up a record based on the
	// record block handle recorded by inItem
	ZAssert(inItem->GetRecordBlockID());
	return this->LoadRecord(inItem->GetRecordBlockID());
	}

void ZDBTable::LoadUp()
	{
	ZRef<ZStreamerR> theStreamer = fDatabase->GetBlockStore()->OpenR(fHeaderBlockID);
	if (!theStreamer)
		throw runtime_error(ZString::sFormat("ZDBTable::LoadUp, block 0x%X isn't there", fHeaderBlockID));

	ZStreamR_Buffered theStream(1024, theStreamer->GetStreamR());

	fVersion = theStream.ReadInt32();
	if (fVersion != 1)
		throw runtime_error(ZString::sFormat("ZDBTable::LoadUp, bad version number (block 0x%X)", fHeaderBlockID));

	ZString::sFromStream(fTableName, theStream);
	fLock.SetName(fTableName.c_str()); // for debugging. -ec 01.02.23
	fRecordCount = theStream.ReadInt32();

	fNextRecordID.FromStream(theStream);
	fFullySpanningIndices = theStream.ReadBool();

	// Read in the field spec list
	long countOfFieldSpecs = theStream.ReadInt32();
	for (long x = 1; x <= countOfFieldSpecs; ++x)
		{
		ZDBFieldSpec aSpec;
		aSpec.FromStream(theStream);
		fFieldSpecList.push_back(aSpec);
		}

	// and our indices
	long countOfIndices = theStream.ReadInt32();
	for (long x = 1; x <= countOfIndices; ++x)
		{
		ZDBIndex* theIndex = this->MakeIndex();
		theIndex->FromStream(theStream, fVersion);
		fIndexList.push_back(theIndex);
		}

#if kDebug
	ZDebugLogf(1, ("ZDBTable::LoadUp() : Table \"%s\" | Version  %d | Record Count %d | Next Record ID %lld [x%llX] |", fTableName.c_str(), fVersion, fRecordCount, fNextRecordID.GetValue(), fNextRecordID.GetValue()));
#endif

#if 0
	// This is wrapped in a try/catch because it is possible that the index that we are relying on is,
	// itself, damaged. The repair code will (most likely) fix this - but not if we don't absorb 
	// exception. -ec 00.10.18
	try
		{
		// If we generate record IDs then ensure that fNextRecordID is greater than the largest
		// in any extant records.
		bool needsWriteBack = false;
		if (fNextRecordID != 0)
			{
			ZDBSearch search_lastID = this->Search(ZDBSearchSpec(ZDBFieldName::ID, new ZDBFieldRepID), ZDBSearch::eAll);
			if (!search_lastID.empty())
				{
				ZDBRecordID nextRecordID = search_lastID.back()->GetRecordID();
				nextRecordID += 1;
				if (fNextRecordID < nextRecordID)
					{
					fNextRecordID = nextRecordID;
					needsWriteBack = true;
					}
				}
			}

		if (needsWriteBack)
			this->WriteBack();
		else
			fDirty = false;
		}
	catch (exception& theEx)
		{}
#endif
	}

void ZDBTable::WriteBack()
	{
	ZMutexLocker lock(fLoadMutex);

	ZRef<ZStreamerWPos> theStreamer = fDatabase->GetBlockStore()->OpenWPos(fHeaderBlockID);
	{
	ZStreamW_Buffered theStream(1024, theStreamer->GetStreamW());

	theStream.WriteInt32(fVersion); // Version number
	ZString::sToStream(fTableName, theStream);
	theStream.WriteInt32(fRecordCount);
	fNextRecordID.ToStream(theStream);
	theStream.WriteBool(fFullySpanningIndices);

	// Write out the field spec list
	theStream.WriteInt32(fFieldSpecList.size());
	for (vector<ZDBFieldSpec>::iterator i = fFieldSpecList.begin(); i != fFieldSpecList.end(); ++i)
		i->ToStream(theStream);

	// And the info for each extant index
	theStream.WriteInt32(fIndexList.size());
	for (vector<ZDBIndex*>::iterator i = fIndexList.begin(); i != fIndexList.end(); ++i)
		(*i)->ToStream(theStream, 0);
	}

	theStreamer->GetStreamWPos().Truncate();

	fDirty = false;
	}

void ZDBTable::DeleteYourself()
	{
	ZLocker locker(this->GetLock()->GetWrite());
	// Delete all indices except one, which we need for deleting records
	while (fIndexList.size() > 1)
		this->DeleteIndex(fIndexList[0]);

	// Now take our last index, assuming we have one
	if (fIndexList.size() != 1)
		ZDebugStopf(0, ("ZDBTable::DeleteYourself, we've got a table with no indices"));
	else
		{
		ZDBIndex* theIndex = fIndexList[0];
		// If we are fully spanning then we can just delete the final index, otherwise
		// we need to delete each record individually. This kinda makes the case for
		// keeping our records on a linked list, so we can access them without having
		// and index at all
		if (!fFullySpanningIndices)
			{
			long badRecordCount = 0;
			while (fRecordCount > badRecordCount)
				{
				try
					{
					ZDBSearch theSearch(theIndex);
					ZRef<ZDBRecord> aRecord;
					if (badRecordCount == 0)
						aRecord = *theSearch.begin();
					else
						aRecord = theSearch[badRecordCount];

					this->DeleteRecord(aRecord);
					}
				catch (...)
					{
					//ZDebugLogf(0, ("ZDBTable::DeleteYourself, trouble deleting a record"));
					ZDebugLogf(0, ("ZDBTable::DeleteYourself, trouble deleting a record in table '%s'", this->GetTableName().c_str())); // better info. -ec 01.01.05
					++badRecordCount;
					}
				}
			}
		this->DeleteIndex(theIndex);
		}

	fDatabase->GetBlockStore()->Delete(fHeaderBlockID);
	fDirty = false;

	locker.Release();
	delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeNode

static inline ZBlockStore::BlockID sCreateEmptyBlock(ZBlockStore* iBlockStore)
	{
	ZBlockStore::BlockID theID;
	if (!iBlockStore->Create(theID))
		ZDebugStopf(0, ("Couldn't create a block for a ZDBTreeNode"));
	return theID;
	}

ZDBTreeNode::ZDBTreeNode(ZDBTreeNodeSpace* inNodeSpace)
:	ZBTree::Node(inNodeSpace, sCreateEmptyBlock(inNodeSpace->GetBlockStore()))
	{
	// We've created our block, but have not been written out yet, so we start off dirty
	fDirty = true;
	fPrev = nil;
	fNext = nil;
	}

ZDBTreeNode::ZDBTreeNode(ZDBTreeNodeSpace* inNodeSpace, ZBlockStore::BlockID inBlockID)
:	ZBTree::Node(inNodeSpace, inBlockID)
	{
	fPrev = nil;
	fNext = nil;

	ZAssert(fItemCount == 0);
	// We *know* that our nodespace is a ZDBTreeNodeSpace
	ZDBTreeNodeSpace* ourNodeSpace = (ZDBTreeNodeSpace*)fNodeSpace;

	if (inNodeSpace->GetVersion() == 1)
		{
		ZRef<ZStreamerR> theStreamer = ourNodeSpace->GetBlockStore()->OpenR(fHandle);
		if (!theStreamer)
			throw runtime_error(ZString::sFormat("ZDBTreeNode, block 0x%X isn't there.", fHandle));

		ZStreamR_Buffered theStream(1024, theStreamer->GetStreamR());

		// Now load up our items
		// We support up to 127 items, 128 subtrees
		uint8 theHeaderByte = theStream.ReadUInt8();
		fIsLeaf = ((theHeaderByte & 0x80) == 0x80);

		// AG 97-11-02. Check that theHeaderByte has a sensible count of items.
		if ((theHeaderByte & 0x7F) > inNodeSpace->GetOrder())
			throw runtime_error(ZString::sFormat("ZDBTreeNode, illegal item count (block 0x%X)", fHandle));

		fItemCount = theHeaderByte & 0x7F;

		// Read in sub-tree size.
		fSubTreeSize = theStream.ReadInt32();

		// Read each of our items.
		for (long x = 0; x < fItemCount; ++x)
			fItems[x] = new ZDBTreeItem(ourNodeSpace->GetIndex(), theStream);

		// And our list of sub tree handles
		for (long x = 0; x <= fItemCount; ++x)
			fSubTreeHandles[x] = theStream.ReadUInt32();

		// In the past, nodes did not truncate their block when written back,
		// so if our size doesn't match then *don't* validate our cache, this
		// will force the node to be written back. When we update the
		// node format we'll tag a node with a signature word, and enforce
		// the rule that nodes must suck up all their data otherwise they're
		// invalid.
		if (theStream.CountReadable())
			{
			// If our item count is zero, we've probably been initialized from
			// a record block (most of which have all zeroes in their first five
			// bytes), so throw an exception.
			if (fItemCount == 0)
				throw runtime_error(ZString::sFormat("ZDBTreeNode, zero item count (block 0x%X)", fHandle));
			fDirty = true;
			}
		else
			{
			fDirty = false;
			}
		}
	else // if (inNodeSpace->GetVersion() == 2)
		{
		ZUnimplemented();
		}
	}

void ZDBTreeNode::ContentChanged()
	{
	fDirty = true;
	}

void ZDBTreeNode::WriteBack()
	{
	// We *know* that our nodespace is a ZDBTreeNodeSpace
	ZDBTreeNodeSpace* ourNodeSpace = (ZDBTreeNodeSpace*)fNodeSpace;

	if (ourNodeSpace->GetVersion() == 1)
		{
		ZRef<ZStreamerWPos> theStreamer = ourNodeSpace->GetBlockStore()->OpenWPos(fHandle);
		if (!theStreamer)
			{
			// We can't open our block for writing.
			if (ourNodeSpace->GetBlockStore()->OpenRPos(fHandle))
				{
				// But we can for reading. So our blockstore is read only,
				// and we might as well just quietly bail. The likelihood is
				// that the only reason we're marked dirty is that our block
				// was oversized, in which case bailing is no big deal.
				return;
				}
			ZDebugStopf(0, ("Couldn't open the block for our node"));
			}
		else
			{
			{ // Scope for the write stream buffer
			ZStreamW_Buffered theStream(1024, theStreamer->GetStreamW());
	
			// We support up to 127 items, 128 subtrees
			uint8 theHeaderByte = fItemCount;
			if (fIsLeaf)
				theHeaderByte |= 0x80;
			theStream.WriteUInt8(theHeaderByte);
	
			// Write out our sub-tree size
			theStream.WriteInt32(fSubTreeSize);
	
			// We walk our list of loaded items, writing each out to the stream
			for (long x = 0; x < fItemCount; ++x)
				{
				ZDBTreeItem* theItem = (ZDBTreeItem*)fItems[x];
				theItem->ToStream(theStream);
				}
	
			// And our list of sub tree handles
			for (long x = 0; x <= fItemCount; ++x)
				theStream.WriteUInt32(fSubTreeHandles[x]);
			} // End of scope for theStream.
	
			// Truncate the block, otherwise it'll show up as damaged when we try to read it back in.
			theStreamer->GetStreamWPos().Truncate();
	
			// And our in-memory copy is now the same as what's on disk
			fDirty = false;
			}
		}
	else // if (ourNodeSpace->GetVersion() == 2)
		{
		ZUnimplemented();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeNodeSpace

ZDBTreeNodeSpace::ZDBTreeNodeSpace(long inDegree, ZDBIndex* inIndex, ZBTree::Handle inHandle, long inVersion)
:	ZBTree::NodeSpace(inDegree, inHandle),
	fIndex(inIndex),
	fCached_Head(nil),
	fCached_Tail(nil),
	fVersion(inVersion),
	fMutex("ZDBTreeNodeSpace::fMutex")
	{
	// DBTreeNodes use a single byte to record the count of items, so our order must be <= 127
	ZAssertStop(kDebug_ZDBase, this->GetOrder() <= 127);
	}

ZDBTreeNodeSpace::ZDBTreeNodeSpace(long inDegree, ZDBIndex* inIndex, long inVersion)
:	ZBTree::NodeSpace(inDegree),
	fIndex(inIndex),
	fCached_Head(nil),
	fCached_Tail(nil),
	fVersion(inVersion),
	fMutex("ZDBTreeNodeSpace::fMutex")
	{
	// DBTreeNodes use a single byte to record the count of items, so our order must be <= 127
	ZAssertStop(kDebug_ZDBase, this->GetOrder() <= 127);
	}

ZDBTreeNodeSpace::~ZDBTreeNodeSpace()
	{
	// Lose the reference to our root node
	fRootNode = nil;

	// Note that we should not have any loaded nodes at this point, as every reference to a node
	// should have gone out of scope by now, so there's no need to flush, and no need to delete anything
	ZAssert(fLoadedNodes.empty());

	// Walk our cache, flushing any nodes that need it, and deleting them
	for (map<ZBTree::Handle, ZDBTreeNode*>::iterator i = fCachedNodes.begin(); i != fCachedNodes.end(); ++i)
		{
		if ((*i).second->IsDirty())
			(*i).second->WriteBack();
		delete (*i).second;
		}
	}

ZRef<ZBTree::Node> ZDBTreeNodeSpace::LoadNode(ZBTree::Handle inHandle)
	{
	ZAssertStop(kDebug_ZDBase, inHandle != ZBTree::kInvalidHandle);

	ZMutexLocker locker(fMutex);

	map<ZBTree::Handle, ZDBTreeNode*>::iterator theIter = fLoadedNodes.find(inHandle);
	if (theIter != fLoadedNodes.end())
		return (*theIter).second;

	theIter = fCachedNodes.find(inHandle);
	if (theIter != fCachedNodes.end())
		{
		ZDBTreeNode* theNode = (*theIter).second;
		fCachedNodes.erase(theIter);

		// Unlink it
		if (theNode->fPrev)
			theNode->fPrev->fNext = theNode->fNext;
		if (theNode->fNext)
			theNode->fNext->fPrev = theNode->fPrev;

		if (fCached_Head == theNode)
			fCached_Head = theNode->fNext;
		if (fCached_Tail == theNode)
			fCached_Tail = theNode->fPrev;

		theNode->fPrev = nil;
		theNode->fNext = nil;

		fLoadedNodes[inHandle] = theNode;
		return theNode;
		}
	else
		{	
		ZDBTreeNode* theNode = new ZDBTreeNode(this, inHandle);
		fLoadedNodes[inHandle] = theNode;
		return theNode;
		}
	}

ZRef<ZBTree::Node> ZDBTreeNodeSpace::CreateNode()
	{
	ZMutexLocker locker(fMutex);

	// By calling this constructor, the node knows to create a block for it to reside in
	ZDBTreeNode* theNode = new ZDBTreeNode(this);
	fLoadedNodes[theNode->GetHandle()] = theNode;
	return theNode;
	}

void ZDBTreeNodeSpace::DeleteNode(ZRef<ZBTree::Node> inNode)
	{
	ZAssertStop(kDebug_ZDBase, inNode);

	ZMutexLocker locker(fMutex);

	ZBTree::Handle theHandle = inNode->GetHandle();

	map<ZBTree::Handle, ZDBTreeNode*>::iterator loadedIter = fLoadedNodes.find(theHandle);
	ZAssertStop(kDebug_ZDBase, loadedIter != fLoadedNodes.end());
	fLoadedNodes.erase(loadedIter);

	fIndex->GetBlockStore()->Delete(theHandle);

	inNode->SetHandle(ZBTree::kInvalidHandle);
	// We don't have to delete inNode -- when it is finalized it will pass in its now nil handle
	// and we'll just okay its deletion. We don't want to delete it now as there is at least
	// one ZRef to the node extant -- deleting the object out from under the ZRef is UGLY.
	}

void ZDBTreeNodeSpace::NodeBeingFinalized(ZBTree::Node* inNode)
	{
	ZMutexLocker locker(fMutex);

	if (inNode->GetRefCount() > 1)
		{
		inNode->FinalizationComplete();
		return;
		}

	ZDBTreeNode* ourNode = static_cast<ZDBTreeNode*>(inNode);
	ZBTree::Handle theHandle = ourNode->GetHandle();

	// If this node has already been deleted (it has an invalid handle) then it's already been pulled from our
	// cache and loaded nodes list.
	if (theHandle == ZBTree::kInvalidHandle)
		{
		ourNode->FinalizationComplete();
		delete ourNode;
		return;
		}

	// So, it's not a deleted node. It must be in fLoadedNodes somewhere
	map<ZBTree::Handle, ZDBTreeNode*>::iterator loadedIter = fLoadedNodes.find(theHandle);
	ZAssertStop(kDebug_ZDBase, loadedIter != fLoadedNodes.end());
	fLoadedNodes.erase(loadedIter);

	// Now put the node at the end of the cached nodes list
	
	ZAssertStop(kDebug_ZDBase, ourNode->fPrev == nil);
	ZAssertStop(kDebug_ZDBase, ourNode->fNext == nil);

	ourNode->fPrev = fCached_Tail;
	if (fCached_Tail)
		fCached_Tail->fNext = ourNode;
	else
		fCached_Head = ourNode;

	fCached_Tail = ourNode;
	
	fCachedNodes[theHandle] = ourNode;

	// We've done all we need to do to finalize the node -- it's off fLoadedNodes and moved on to fCachedNodes.
	ourNode->FinalizationComplete();

	if (2 * fCachedNodes.size() > 16 * this->GetOrder())
		{
		size_t cacheMax = (12 * this->GetOrder()) / 2;
		while (fCachedNodes.size() > cacheMax)
			{
			// Lose the head
			ZDBTreeNode* theNode = fCached_Head;
			fCached_Head = theNode->fNext;
			if (fCached_Head)
				fCached_Head->fPrev = nil;
			else
				fCached_Tail = nil;
	
			map<ZBTree::Handle, ZDBTreeNode*>::iterator cacheIter = fCachedNodes.find(theNode->GetHandle());
			ZAssertStop(kDebug_ZDBase, cacheIter != fCachedNodes.end());
			fCachedNodes.erase(cacheIter);
			if (theNode)
				theNode->WriteBack();
			delete theNode;
			}
		}
	}

long ZDBTreeNodeSpace::GetNodeSubTreeSize(ZBTree::Handle inHandle)
	{
	ZMutexLocker locker(fMutex);

	ZAssert(inHandle != ZBTree::kInvalidHandle);

	// Is it on our loaded nodes list?
	map<ZBTree::Handle, ZDBTreeNode*>::iterator theIter = fLoadedNodes.find(inHandle);
	if (theIter != fLoadedNodes.end())
		return (*theIter).second->GetSubTreeSize();

	// How about in our cache?
	theIter = fCachedNodes.find(inHandle);
	if (theIter != fCachedNodes.end())
		return (*theIter).second->GetSubTreeSize();

	// Pull in the info off disk without fully loading the node
	ZRef<ZStreamerR> theStreamer = this->GetBlockStore()->OpenR(inHandle);
	const ZStreamR& theStream = theStreamer->GetStreamR();

	// Skip the header byte
	uint8 theHeaderByte = theStream.ReadUInt8();

	// read and return the sub-tree size
	return theStream.ReadInt32();
	}

void ZDBTreeNodeSpace::Flush()
	{
	ZMutexLocker locker(fMutex);

	// Write back our loaded nodes
	for (map<ZBTree::Handle, ZDBTreeNode*>::iterator i = fLoadedNodes.begin(); i != fLoadedNodes.end(); ++i)
		{
		if ((*i).second->IsDirty())
			(*i).second->WriteBack();
		}

	// And our cached nodes
	for (map<ZBTree::Handle, ZDBTreeNode*>::iterator i = fCachedNodes.begin(); i != fCachedNodes.end(); ++i)
		{
		if ((*i).second->IsDirty())
			(*i).second->WriteBack();
		}
	}

void ZDBTreeNodeSpace::GrabRootNode()
	{
	if (this->GetRootHandle() != ZBTree::kInvalidHandle)
		{
		try
			{
			fRootNode = this->LoadNode(this->GetRootHandle());
			}
		catch (bad_alloc& theEx)
			{ throw; }
		catch (...)
			{
			// Absorb the error
			}
		}
	}

void ZDBTreeNodeSpace::SetNewRoot(ZBTree::Handle inHandle)
	{
	ZBTree::NodeSpace::SetNewRoot(inHandle);
	if (fRootNode)
		{
		if (inHandle != fRootNode->GetHandle())
			fRootNode = nil;
		}
	if (inHandle != ZBTree::kInvalidHandle)
		fRootNode = this->LoadNode(inHandle);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDBTreeItem

ZDBTreeItem::ZDBTreeItem(const ZDBTreeItem& inOther)
:	fRecordBlockID(inOther.fRecordBlockID), fIndex(inOther.fIndex)
	{
	// We'll have to walk the field reps, cloning each one
	fFieldReps.reserve(inOther.fFieldReps.size());
	for (vector<ZDBFieldRep*>::const_iterator i = inOther.fFieldReps.begin(); i != inOther.fFieldReps.end(); ++i)
		fFieldReps.push_back((*i)->Clone());
	}

ZDBTreeItem::ZDBTreeItem(ZDBIndex* inIndex, const vector<ZDBSearchSpec>& inSearchSpecs)
	{
	// We're not going to be inserted into the index, and we're not related to a record, so just
	// initialize with an invalid handle here
	fRecordBlockID = 0;
	fIndex = nil;

	// Just make sure that the searchSpecs are a proper prefix of our indexed fields
	long countOfItemsPulled = inSearchSpecs.size();
	const vector<ZDBFieldName>& theFieldNames(inIndex->GetIndexedFieldNames());
	for (vector<ZDBFieldName>::const_iterator i = theFieldNames.begin();
										i != theFieldNames.end() && countOfItemsPulled > 0; ++i)
		{
		bool gotIt = false;
		for (vector<ZDBSearchSpec>::const_iterator j = inSearchSpecs.begin(); j != inSearchSpecs.end(); ++j)
			{
			if ((*i) == j->GetFieldName())
				{
				fFieldReps.push_back(j->GetFieldRep()->Clone());
				countOfItemsPulled--;
				gotIt = true;
				break;
				}
			}
		ZAssert(gotIt);		// If this fails, we didn't get one of our indexed fields *in order*
		}
	}

ZDBTreeItem::ZDBTreeItem(ZDBIndex* inIndex, const ZDBSearchSpec& inSearchSpec)
	{
	// We're not going to be inserted into the index, and we're not related to a record, so just
	// initialize with a invalid handle here
	fRecordBlockID = 0;
	fIndex = nil;

	// Grab copies of theSearchSpec's field reps, ordered by our index's sort order
	// Also, make sure we end up with exactly the same number of items as our index has

	// Again, we don't bother checking for matching sizes anymore -- just that it's a proper prefix
#if ZCONFIG_Debug
	const vector<ZDBFieldName>& theFieldNames(inIndex->GetIndexedFieldNames());
	ZAssert(inSearchSpec.GetFieldName() == theFieldNames[0]);
#endif

	fFieldReps.push_back(inSearchSpec.GetFieldRep()->Clone());
	}

ZDBTreeItem::ZDBTreeItem(ZDBIndex* inIndex, ZDBRecord* inRecord)
:	fRecordBlockID(0),
	fIndex(inIndex)
	{
	// Get the list of indexed fields from theIndex and pull the corresponding fields from theRecord
	// We can't just grab the fields direct from theRecord because we have to respect the ordering
	// of fields in theIndex, that order determines which field is the primary key etc.
	const vector<ZDBFieldName>& theFieldNames(inIndex->GetIndexedFieldNames());

	// A little optimization here, as we know how big the vector will be
	fFieldReps.reserve(theFieldNames.size());
	for (vector<ZDBFieldName>::const_iterator i = theFieldNames.begin(); i != theFieldNames.end(); ++i)
		{
		const ZDBField* theField = inRecord->GetField(*i);
		fFieldReps.push_back(theField->GetClonedRep());
		}

	// Only grab the record's BlockID if all indices span the whole table. Note that
	// even if we were to grab the block handle from a record belonging to a fully spanned
	// table, it would be invalid anyway -- we do this just to be explicit.
	if (!fIndex->GetIndicesSpan())
		fRecordBlockID = inRecord->GetBlockID();
	}

ZDBTreeItem::ZDBTreeItem(ZDBIndex* inIndex, ZStreamR& inStream)
:	fRecordBlockID(0), fIndex(inIndex)
	{
	ZAssert(fFieldReps.size() == 0);
	const vector<ZDBFieldSpec>& theFieldSpecs = fIndex->GetIndexedFieldSpecs();
	fFieldReps.reserve(theFieldSpecs.size());
	for (vector<ZDBFieldSpec>::const_iterator i = theFieldSpecs.begin(); i != theFieldSpecs.end(); ++i)
		{
		ZDBFieldRep* theRep = fIndex->GetTable()->CreateFieldRepByType(i->GetFieldType());
		try
			{
			theRep->FromStream(inStream);
			fFieldReps.push_back(theRep);
			}
		catch (...) // contain potential memory leak. -ec 00.12.14
			{
			delete theRep;
			throw;
			}
		}
	// We only have to read in a block handle if the table is not fully spanned by all indices
	if (!fIndex->GetIndicesSpan())
		fRecordBlockID = inStream.ReadUInt32();
	}

ZDBTreeItem::~ZDBTreeItem()
	{
	// Trash our copies of the fieldReps
	for (vector<ZDBFieldRep*>::iterator i = fFieldReps.begin(); i != fFieldReps.end(); ++i)
		delete *i;
	}

long ZDBTreeItem::Compare(const ZBTree::Item* inOther)
	{
	// This routine is primarily used for inserts and deletions
	ZDBTreeItem* realItem = (ZDBTreeItem*)inOther;

	// As we're doing a full compare, make sure the other item has the same number of field reps as us
	ZAssert(fFieldReps.size() == realItem->fFieldReps.size());

	// Make sure that we don't come from different indices
	ZAssert(realItem->fIndex == nil || fIndex == nil || realItem->fIndex == fIndex);

	// Check that both us and the otherItem have a valid record handle, or that neither of us has a valid
	// handle, as we might be part of a fully spanning index
	ZAssert((!fRecordBlockID && !realItem->fRecordBlockID) || (fRecordBlockID && realItem->fRecordBlockID));

	long result = 0;
	// Now, walk through our field reps, comparing them in order
	// The theory is this -- we can bail anytime one of our leading fields is not equal to their corresponding
	// field. The overall result of the comparison is the result of that particular comparison
	vector<ZDBFieldRep*>::iterator us = fFieldReps.begin();
	vector<ZDBFieldRep*>::iterator them = realItem->fFieldReps.begin();
	for (;us != fFieldReps.end() && result == 0; ++us, ++them)
		result = (*us)->Compare(*them);

	// Finally, and this is a bit subtle, as we were created from a record we will have
	// a value in fRecordBlockID, so our overall comparison must be
	// based on its value, assuming all our items match.
	// This is because removes and changes on records must affect the actual record that was
	// *originally* indexed, not another record that also has the same values in its key fields
	// Of course, if the content comparison did not evaluate equal, we don't have to worry
	if (result == 0)
		{
		if (fRecordBlockID < realItem->fRecordBlockID)
			result = -1;
		else if (fRecordBlockID > realItem->fRecordBlockID)
			result = 1;
		// don't need the other clause here, result is already 0
		}
	return result;
	}

long ZDBTreeItem::CompareForSearch(const ZBTree::Item* inOther, bool inStrictComparison)
	{
	// inStrictComparison will be true if we're an item in a leaf level node. If so, then our
	// comparison must return equal if we have an equal prefix. Otherwise, we return
	// less than or greater than if there is a length mismatch -- this is because any change
	// in *full* values must occur in a leaf.
	ZDBTreeItem* realItem = (ZDBTreeItem*)inOther;

	// This comparison method is used when we're doing partial finds -- finds on a prefix
	// of our index's field list

	// Make sure that the other item does not come from an index (this method is only used)
	// when a search is being done, not for insertions, deletions etc.
	ZAssert(realItem->fIndex == nil);

	// Also, check that our size >= the other's size -- we're searching on a prefix
	ZAssert(fFieldReps.size() >= realItem->fFieldReps.size());

	long result = 0;
	// Now, walk through our field reps, comparing them in order
	// The theory is this -- we can bail anytime one of our leading fields is not equal to their corresponding
	// field. The overall result of the comparison is the result of that particular comparison, (so long
	// as we're the same length)
	vector<ZDBFieldRep*>::iterator us = fFieldReps.begin();
	vector<ZDBFieldRep*>::iterator them = realItem->fFieldReps.begin();
	for (; us != fFieldReps.end() && them !=realItem->fFieldReps.end() && result == 0; ++us, ++them)
		result = (*us)->Compare(*them);

	// We matched, and bailed from the loop because one (or both) iterators reached then end of their vectors
	if (result == 0)
		{
		// If we're not in a leaf, then check that we are the same length as our comparior. If we have
		// different lengths, then return that we are less than or greater than the item in question
		if (inStrictComparison)
			{
			// If one of the iterators is not at the end of its vector, then we're not *precisely* equal
			if (us != fFieldReps.end())
				result = 1;
			else if (them != realItem->fFieldReps.end())
				result = -1;
			}
		}
	return result;
	}


long ZDBTreeItem::CompareForIteration(const ZDBTreeItem* inOther)
	{
	ZDBTreeItem* realItem = (ZDBTreeItem*)inOther;

	// This comparison is used when we're checking whether we still match a
	// search specification. So, we return the result of even a partial find without
	// fucking with it.

	// Make sure that the other item does not come from an index (this method is only used)
	// when a search is being done, not for insertions, deletions etc.
	ZAssert(realItem->fIndex == nil);

	// Also, check that our size >= the other's size -- we're searching on a prefix
	ZAssert(fFieldReps.size() >= realItem->fFieldReps.size());

	long result = 0;
	// Now, walk through our field reps, comparing them in order
	// The theory is this -- we can bail anytime one of our leading fields is not equal to their corresponding
	// field. The overall result of the comparison is the result of that particular comparison
	vector<ZDBFieldRep*>::iterator us = fFieldReps.begin();
	vector<ZDBFieldRep*>::iterator them = realItem->fFieldReps.begin();
	for (;us != fFieldReps.end() && them !=realItem->fFieldReps.end() && result == 0; ++us, ++them)
		result = (*us)->Compare((*them));

	return result;
	}

ZDBTreeItem* ZDBTreeItem::Clone() const
	{ return new ZDBTreeItem(*this); }

void ZDBTreeItem::ToStream(const ZStreamW& inStream)
	{
	// We don't have to write out either a count of the number of fields, nor their types -- both
	// items of info can be derived from our index
	for (vector<ZDBFieldRep*>::iterator i = fFieldReps.begin(); i != fFieldReps.end(); ++i)
		(*i)->ToStream(inStream);
	if (!fIndex->GetIndicesSpan())
		inStream.WriteUInt32(fRecordBlockID);
	}

