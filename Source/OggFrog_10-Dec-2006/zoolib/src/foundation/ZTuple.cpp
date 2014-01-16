static const char rcsid[] = "@(#) $Id: ZTuple.cpp,v 1.64 2006/11/17 21:03:34 agreen Exp $";

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

#include "ZTuple.h"
#include "ZMemory.h"
#include "ZMemoryBlock.h"

#include <stdexcept> // For runtime_error

using std::pair;
using std::runtime_error;
using std::string;
using std::vector;

#define kDebug_Tuple 1

static ZRectPOD sNilRect;
static ZPointPOD sNilPoint;

static vector<ZTupleValue> sNilVector;
static ZTupleValue sNilValue;
static ZTuple sNilTuple;
static string sNilString;
static ZTuple::PropList sEmptyProperties;

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

static inline void sWriteCount(const ZStreamW& iStream, uint32 iCount)
	{
	if (iCount < 0xFF)
		{
		iStream.WriteUInt8(iCount);
		}
	else
		{
		iStream.WriteUInt8(0xFF);
		iStream.WriteUInt32(iCount);
		}
	}

static inline uint32 sReadCount(const ZStreamR& iStreamR)
	{
	uint8 firstByte = iStreamR.ReadUInt8();
	if (firstByte < 0xFF)
		return firstByte;
	return iStreamR.ReadUInt32();
	}

static inline int sCompare(const void* iLeft, size_t iLeftLength,
	const void* iRight, size_t iRightLength)
	{
	if (int compare = memcmp(iLeft, iRight, min(iLeftLength, iRightLength)))
		return compare;
	// Strictly speaking the rules of two's complement mean that we
	// not need the casts, but let's be clear about what we're doing.
	return int(iLeftLength) - int(iRightLength);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuplePropName::String

// ZTuplePropName::String is only used for non-zero length names that do not
// appear in the pre-registered names table. We assert that fSize is non-zero
// because there are a few places in ZTuplePropName where we rely on the fact.

inline ZTuplePropName::String::String(const char* iString, size_t iSize)
:	fSize(iSize)
	{
	ZAssertStop(kDebug_Tuple, fSize);
	ZBlockCopy(iString, fBuffer, fSize);
	}

inline int ZTuplePropName::String::Compare(const String& iOther) const
	{
	ZAssertStop(kDebug_Tuple, fSize);
	return sCompare(fBuffer, fSize, iOther.fBuffer, iOther.fSize);
	}

inline int ZTuplePropName::String::Compare(const char* iString, size_t iSize) const
	{
	ZAssertStop(kDebug_Tuple, fSize);
	return sCompare(fBuffer, fSize, iString, iSize);
	}

inline int ZTuplePropName::String::Compare(const std::string& iString) const
	{
	ZAssertStop(kDebug_Tuple, fSize);
	if (size_t otherSize = iString.size())
		return sCompare(fBuffer, fSize, iString.data(), otherSize);
	// We can't be empty, so we must be greater than iString.
	return 1;
	}

inline void ZTuplePropName::String::ToStream(const ZStreamW& iStreamW) const
	{
	ZAssertStop(kDebug_Tuple, fSize);
	sWriteCount(iStreamW, fSize);
	iStreamW.Write(fBuffer, fSize);
	}

inline string ZTuplePropName::String::AsString() const
	{ return string(fBuffer, fSize); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTuplePropName, helper functions

namespace ZANONYMOUS {

struct PNRep
	{
	uint8 fLength;
	char fBuffer[1];
	};

typedef pair<const char*, size_t> Key;

struct Compare_PNRep_Key_t
	{
	typedef const PNRep* first_argument_type;
	typedef Key second_argument_type;

	bool operator()(const first_argument_type& iLeft, const second_argument_type& iRight) const
		{ return 0 > sCompare(iLeft->fBuffer, iLeft->fLength, iRight.first, iRight.second); }
		
	typedef bool result_type;
	};

} // anonymous namespace

static vector<const PNRep*>* sNames;

static inline bool sDifferent(const PNRep* iPNRep, const char* iName, size_t iLength)
	{ return sCompare(iPNRep->fBuffer, iPNRep->fLength, iName, iLength); }

static const PNRep* sLookupAndTag(const char* iName, size_t iLength)
	{
	if (!iLength)
		return reinterpret_cast<const PNRep*>(1);

	if (!sNames)
		return nil;

	vector<const PNRep*>::iterator theIter =
		lower_bound(sNames->begin(), sNames->end(), Key(iName, iLength), Compare_PNRep_Key_t());

	if (theIter == sNames->end() || sDifferent(*theIter, iName, iLength))
		return nil;

	return reinterpret_cast<const PNRep*>(reinterpret_cast<intptr_t>(*theIter) | 1);
	}

static const PNRep* sLookupAndTag(const string& iName)
	{
	if (iName.empty())
		return reinterpret_cast<const PNRep*>(1);

	if (!sNames)
		return nil;

	vector<const PNRep*>::iterator theIter =
		lower_bound(sNames->begin(), sNames->end(), Key(iName.data(), iName.length()), Compare_PNRep_Key_t());

	if (theIter == sNames->end() || sDifferent(*theIter, iName.data(), iName.length()))
		return nil;

	return reinterpret_cast<const PNRep*>(reinterpret_cast<intptr_t>(*theIter) | 1);
	}

static inline const PNRep* sGetPNRep(const void* iData)
	{ return reinterpret_cast<const PNRep*>(reinterpret_cast<intptr_t>(iData) & ~1); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTuplePropName

int ZTuplePropName::sPreRegister(const char* const* iNames, size_t iCount)
	{
	if (!sNames)
		sNames = new vector<const PNRep*>;

	// This function should be called under very constrained
	// circumstances, generally from a static initialization.
	while (iCount--)
		{
		const char* theName = *iNames++;
		if (size_t theLength = strlen(theName))
			{
			// It's important that we only allow strings < 255 (ie 254 or fewer).
			// ZTuplePropName::ToStream writes the entirety of the
			// PNRep in a single call, and a string of length 255 would look
			// like a marker byte followed by 4 bytes of length, and things will break.
			if (theLength < 255)
				{
				vector<const PNRep*>::iterator theIter =
					lower_bound(sNames->begin(), sNames->end(), Key(theName, theLength), Compare_PNRep_Key_t());

				if (theIter == sNames->end() || sDifferent(*theIter, theName, theLength))
					{
					PNRep* newRep = reinterpret_cast<PNRep*>(new char[theLength + 1]);
					newRep->fLength = theLength;
					ZBlockCopy(theName, newRep->fBuffer, theLength);
					sNames->insert(theIter, newRep);
					}
				}
			}
		}
	return 0;
	}

ZTuplePropName& ZTuplePropName::operator=(const ZTuplePropName& iOther)
	{
	ZAssertStop(kDebug_Tuple, fData);
	ZAssertStop(kDebug_Tuple, iOther.fData);

	if (!sIsPNRep(fData))
		delete fString;

	if (sIsPNRep(iOther.fData))
		fData = iOther.fData;
	else
		fString = new (iOther.fString->fSize) String(*iOther.fString);

	return *this;
	}

ZTuplePropName::ZTuplePropName(const ZStreamR& iStreamR)
	{
	uint32 theSize = sReadCount(iStreamR);
	if (theSize <= ZooLib::sStackBufferSize)
		{
		char buffer[ZooLib::sStackBufferSize];
		iStreamR.Read(buffer, theSize);
		fData = sLookupAndTag(buffer, theSize);
		if (!fData)
			fString = new (theSize) String(buffer, theSize);
		}
	else
		{
		// theSize must be > 0 (it's greater than ZooLib::sStackBufferSize).
		vector<char> buffer(theSize);
		iStreamR.Read(&buffer[0], theSize);
		fData = sLookupAndTag(&buffer[0], theSize);
		if (!fData)
			fString = new (theSize) String(&buffer[0], theSize);
		}
	}

ZTuplePropName::ZTuplePropName(const char* iString, size_t iLength)
	{
	fData = sLookupAndTag(iString, iLength);
	if (!fData)
		fString = new (iLength) String(iString, iLength);
	}

ZTuplePropName::ZTuplePropName(const std::string& iString)
	{
	fData = sLookupAndTag(iString);
	if (!fData)
		fString = new (iString.size()) String(iString.data(), iString.size());
	}

ZTuplePropName::ZTuplePropName(const char* iString)
	{
	size_t theLength = ::strlen(iString);
	fData = sLookupAndTag(iString, theLength);
	if (!fData)
		fString = new (theLength) String(iString, theLength);
	}

int ZTuplePropName::Compare(const ZTuplePropName& iOther) const
	{
	ZAssertStop(kDebug_Tuple, fData);
	ZAssertStop(kDebug_Tuple, iOther.fData);

	if (fData == iOther.fData)
		return 0;

	if (sIsPNRep(fData))
		{
		// This is a PNRep.
		if (sIsPNRep(iOther.fData))
			{
			// Other is a PNRep.
			if (const PNRep* thisPNRep = sGetPNRep(fData))
				{
				// This is allocated, thus not empty.
				if (const PNRep* otherPNRep = sGetPNRep(iOther.fData))
					{
					// Other is allocated, thus not empty.
					return sCompare(thisPNRep->fBuffer, thisPNRep->fLength, otherPNRep->fBuffer, otherPNRep->fLength);
					}
				else
					{
					// Other is unallocated, thus empty.

					// This is greater than other.
					return 1;
					}
				}
			else
				{
				// This is unallocated, thus empty.
				if (const PNRep* otherPNRep = sGetPNRep(iOther.fData))
					{
					// Other is allocated, and thus not empty.

					// This is less than other.
					return -1;
					}
				else
					{
					// Other is unallocated, thus empty.
					
					// This equals other.
					return 0;
					}
				}
			}
		else
			{
			// Other is not a PNRep.
			if (const PNRep* thisPNRep = sGetPNRep(fData))
				{
				// This is allocated. We could call other's String::Compare and negate
				// the result, but that little minus sign is easy to miss:
				// return - iOther.fString->Compare(thisPNRep->fBuffer, thisPNRep->fLength);

				// So we do the work explicitly:
				return sCompare(thisPNRep->fBuffer, thisPNRep->fLength,  iOther.fString->fBuffer, iOther.fString->fSize);
				}
			else
				{
				// This is unallocated, thus empty. iOther is a String, which
				// cannot be empty, so this must be less than other.
				return -1;
				}
			
			}
		}
	else if (sIsPNRep(iOther.fData))
		{
		if (const PNRep* otherPNRep = sGetPNRep(iOther.fData))
			{
			// Other is allocated.
			return fString->Compare(otherPNRep->fBuffer, otherPNRep->fLength);
			}
		else
			{
			// Other is unallocated, thus empty. This is a String, which
			// cannot be empty, so this must be greater than other.
			return 1;
			}
		}
	else
		{
		return fString->Compare(*iOther.fString);
		}
	}

bool ZTuplePropName::Equals(const char* iString, size_t iLength) const
	{
	ZAssertStop(kDebug_Tuple, fData);

	if (sIsPNRep(fData))
		{
		if (const PNRep* thePNRep = sGetPNRep(fData))
			{
			// We're allocated.
			return 0 == sCompare(thePNRep->fBuffer, thePNRep->fLength, iString, iLength);
			}
		else
			{
			// We're unallocated, and thus empty. We match if iString is empty.
			return 0 == iLength;
			}
		}
	else
		{
		return 0 == fString->Compare(iString, iLength);
		}
	}

bool ZTuplePropName::Equals(const std::string& iString) const
	{
	ZAssertStop(kDebug_Tuple, fData);

	if (sIsPNRep(fData))
		{
		if (const PNRep* thePNRep = sGetPNRep(fData))
			{
			// We're not unallocated.
			if (size_t otherLength = iString.size())
				{
				// Nor is iString -- match characters.
				return 0 == sCompare(thePNRep->fBuffer, thePNRep->fLength, iString.data(), otherLength);
				}
			else
				{
				return 0 == thePNRep->fLength;
				}
			}
		else
			{
			// We're unallocated, and thus empty. We match if iString is empty.
			return iString.empty();
			}
		}
	else
		{
		return 0 == fString->Compare(iString);
		}
	}

bool ZTuplePropName::Empty() const
	{
	ZAssertStop(kDebug_Tuple, fData);

	if (sIsPNRep(fData))
		return !sGetPNRep(fData);

	ZAssertStop(kDebug_Tuple, !fString->fSize);

	return false;
	}

string ZTuplePropName::AsString() const
	{
	ZAssertStop(kDebug_Tuple, fData);

	if (sIsPNRep(fData))
		{
		if (const PNRep* thePNRep = sGetPNRep(fData))
			return string(thePNRep->fBuffer, thePNRep->fLength);
		return string();
		}
	else
		{
		return fString->AsString();
		}
	}

void ZTuplePropName::ToStream(const ZStreamW& iStreamW) const
	{
	ZAssertStop(kDebug_Tuple, fData);

	if (sIsPNRep(fData))
		{
		if (const PNRep* thePNRep = sGetPNRep(fData))
			{
			// Because our length < 255 this preserves the
			// format used by sWriteCount.
			iStreamW.Write(thePNRep, thePNRep->fLength + 1);
			}
		else
			{
			iStreamW.WriteUInt8(0);
			}
		}
	else
		{
		fString->ToStream(iStreamW);
		}
	}

void ZTuplePropName::FromStream(const ZStreamR& iStreamR)
	{
	if (!sIsPNRep(fData))
		delete fString;

	uint32 theSize = sReadCount(iStreamR);
	if (theSize <= ZooLib::sStackBufferSize)
		{
		char buffer[ZooLib::sStackBufferSize];
		iStreamR.Read(buffer, theSize);
		fData = sLookupAndTag(buffer, theSize);
		if (!fData)
			fString = new (theSize) String(buffer, theSize);
		}
	else
		{
		// theSize must be > 0 (it's greater than ZooLib::sStackBufferSize).
		vector<char> buffer(theSize);
		iStreamR.Read(&buffer[0], theSize);
		fData = sLookupAndTag(&buffer[0], theSize);
		if (!fData)
			fString = new (theSize) String(&buffer[0], theSize);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleString

/** ZTupleString is a lighterweight implementation of immutable strings, used
for holding longer string properties in ZTupleValues. */

namespace ZANONYMOUS {

class ZTupleString
	{
	ZTupleString(); // Not implemented
	ZTupleString& operator=(const ZTupleString&); // Not implemented

public:
	~ZTupleString();

	ZTupleString(const ZTupleString& iOther);

	explicit ZTupleString(const string& iOther);

	ZTupleString(const char* iString, size_t iLength);

	ZTupleString(const ZStreamR& iStreamR, size_t iSize);

	int Compare(const ZTupleString& iOther) const;

	int Compare(const char* iString, size_t iSize) const;

	int Compare(const std::string& iString) const;

	bool Empty() const;

	void ToStream(const ZStreamW& iStreamW) const;

	void ToString(std::string& oString) const;

	std::string AsString() const;

private:
	const size_t fSize;
	char* fBuffer;
	};

} // namespace ZANONYMOUS

inline ZTupleString::~ZTupleString()
	{ delete[] fBuffer; }

inline bool ZTupleString::Empty() const
	{ return !fSize; }

inline std::string ZTupleString::AsString() const
	{ return string(fBuffer, fSize); }

ZTupleString::ZTupleString(const ZTupleString& iOther)
:	fSize(iOther.fSize)
	{
	fBuffer = new char[fSize];
	ZBlockCopy(iOther.fBuffer, fBuffer, fSize);
	}

ZTupleString::ZTupleString(const string& iOther)
:	fSize(iOther.size())
	{
	fBuffer = new char[fSize];
	if (fSize)
		ZBlockCopy(iOther.data(), fBuffer, fSize);
	}

ZTupleString::ZTupleString(const char* iString, size_t iLength)
:	fSize(iLength)
	{
	fBuffer = new char[fSize];
	ZBlockCopy(iString, fBuffer, fSize);
	}

ZTupleString::ZTupleString(const ZStreamR& iStreamR, size_t iSize)
:	fSize(iSize)
	{
	fBuffer = new char[iSize];
	try
		{
		iStreamR.Read(fBuffer, iSize);
		}
	catch (...)
		{
		delete fBuffer;
		throw;
		}
	}

inline int ZTupleString::Compare(const ZTupleString& iOther) const
	{ return sCompare(fBuffer, fSize, iOther.fBuffer, iOther.fSize); }

inline int ZTupleString::Compare(const char* iString, size_t iSize) const
	{ return sCompare(fBuffer, fSize, iString, iSize); }

inline int ZTupleString::Compare(const string& iString) const
	{
	if (size_t otherSize = iString.size())
		return sCompare(fBuffer, fSize, iString.data(), otherSize);

	// iString is empty. If fSize is zero, then fSize!=0 is true, and
	// will convert to 1. Otherwise it will be false, and convert
	// to zero, so we always return the correct result.
	return fSize != 0;
	}

void ZTupleString::ToStream(const ZStreamW& iStreamW) const
	{
	sWriteCount(iStreamW, fSize);
	iStreamW.Write(fBuffer, fSize);
	}

void ZTupleString::ToString(string& oString) const
	{
	oString.resize(fSize);
	if (fSize)
		ZBlockCopy(fBuffer, const_cast<char*>(oString.data()), fSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * Placement-new

namespace ZANONYMOUS {

template <class T>
inline T* sConstruct_T(void* iBytes)
	{
	new(iBytes) T();
	return static_cast<T*>(iBytes);
	}

template <class T>
inline T* sConstruct_T(void* iBytes, const T& iOther)
	{
	new(iBytes) T(iOther);
	return static_cast<T*>(iBytes);
	}

template <class T, typename P0>
inline T* sConstruct_T(void* iBytes, const P0& iP0)
	{
	new(iBytes) T(iP0);
	return static_cast<T*>(iBytes);
	}

template <class T, typename P0, typename P1>
inline T* sConstruct_T(void* iBytes, const P0& iP0, const P1& iP1)
	{
	new(iBytes) T(iP0, iP1);
	return static_cast<T*>(iBytes);
	}

template <class T>
inline T* sCopyConstruct_T(const void* iSource, void* iBytes)
	{
	new(iBytes) T(*static_cast<const T*>(iSource));
	return static_cast<T*>(iBytes);
	}

template <class T>
inline void sDestroy_T(void* iBytes)
	{
	static_cast<T*>(iBytes)->~T();
	}

template <class T>
inline const T* sFetch_T(const void* iBytes)
	{
	return static_cast<const T*>(iBytes);
	}

template <class T>
inline T* sFetch_T(void* iBytes)
	{
	return static_cast<T*>(iBytes);
	}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleValue

/** \class ZTupleValue
\nosubgrouping
*/

ZTupleValue::ZTupleValue()
	{
	fType.fType = eZType_Null;
	}

ZTupleValue::~ZTupleValue()
	{
	this->pRelease();
	}

ZTupleValue::ZTupleValue(const ZTupleValue& iOther)
	{
	this->pCopy(iOther);
	}

ZTupleValue& ZTupleValue::operator=(const ZTupleValue& iOther)
	{
	if (this != &iOther)
		{
		this->pRelease();
		this->pCopy(iOther);
		}
	return *this;
	}

ZTupleValue::ZTupleValue(const ZStreamR& iStreamR)
	{
	fType.fType = eZType_Null;
	this->pFromStream(iStreamR);
	}

void ZTupleValue::ToStream(const ZStreamW& iStreamW) const
	{
	if (fType.fType < 0)
		{
		iStreamW.WriteUInt8(eZType_String);
		size_t theSize = -fType.fType-1;
		ZAssertStop(kDebug_Tuple, theSize <= kBytesSize);
		sWriteCount(iStreamW, theSize);
		if (theSize)
			iStreamW.Write(fType.fBytes, theSize);
		return;
		}

	iStreamW.WriteUInt8(fType.fType);
	switch (fType.fType)
		{
		case eZType_Null:
			{
			// No data to write.
			break;
			}
		case eZType_Type:
			{
			iStreamW.WriteUInt8(fData.fAs_Type);
			break;
			}
		case eZType_ID:
			{
			iStreamW.WriteUInt64(fData.fAs_ID);
			break;
			}
		case eZType_Int8:
			{
			iStreamW.WriteUInt8(fData.fAs_Int8);
			break;
			}
		case eZType_Int16:
			{
			iStreamW.WriteUInt16(fData.fAs_Int16);
			break;
			}
		case eZType_Int32:
			{
			iStreamW.WriteUInt32(fData.fAs_Int32);
			break;
			}
		case eZType_Int64:
			{
			iStreamW.WriteUInt64(fData.fAs_Int64);
			break;
			}
		case eZType_Bool:
			{
			iStreamW.WriteUInt8(fData.fAs_Bool ? 1 : 0);
			break;
			}
		case eZType_Float:
			{
			iStreamW.WriteFloat(fData.fAs_Float);
			break;
			}
		case eZType_Double:
			{
			iStreamW.WriteDouble(fData.fAs_Double);
			break;
			}
		case eZType_Time:
			{
			iStreamW.WriteDouble(fData.fAs_Time);
			break;
			}
		case eZType_Pointer:
			{
			iStreamW.WriteUInt32(reinterpret_cast<uint32>(fData.fAs_Pointer));
			break;
			}
		case eZType_Rect:
			{
			iStreamW.WriteUInt32(fData.fAs_Rect->left);
			iStreamW.WriteUInt32(fData.fAs_Rect->top);
			iStreamW.WriteUInt32(fData.fAs_Rect->right);
			iStreamW.WriteUInt32(fData.fAs_Rect->bottom);
			break;
			}
		case eZType_Point:
			{
			iStreamW.WriteUInt32(fData.fAs_Point.h);
			iStreamW.WriteUInt32(fData.fAs_Point.v);
			break;
			}
		case eZType_String:
			{
			sFetch_T<ZTupleString>(fType.fBytes)->ToStream(iStreamW);
			break;
			}
		case eZType_Tuple:
			{
			sFetch_T<ZTuple>(fType.fBytes)->ToStream(iStreamW);
			break;
			}
		case eZType_RefCounted:
			{
			// Just do nothing. We'll construct a nil refcounted when we read.
			break;
			}
		case eZType_Raw:
			{
			const ZMemoryBlock* theMemoryBlock = sFetch_T<ZMemoryBlock>(fType.fBytes);
			sWriteCount(iStreamW, theMemoryBlock->GetSize());
			if (theMemoryBlock->GetSize())
				iStreamW.Write(theMemoryBlock->GetData(), theMemoryBlock->GetSize());
			break;
			}
		case eZType_Vector:
			{
			if (const vector<ZTupleValue>* theVector = fData.fAs_Vector)
				{
				size_t theCount = theVector->size();
				sWriteCount(iStreamW, theCount);
				for (size_t x = 0; x < theCount; ++x)
					{
					// Sigh, older linux headers don't have vector::at
					// theVector->at(x).ToStream(iStreamW);
					theVector->operator[](x).ToStream(iStreamW);
					}
				}
			else
				{
				// Empty vector optimization
				sWriteCount(iStreamW, 0);
				}
			break;
			}
		default:
			{
			ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
			}
		}
	}

void ZTupleValue::FromStream(const ZStreamR& iStreamR)
	{
	this->pRelease();
	fType.fType = eZType_Null;
	this->pFromStream(iStreamR);
	}

void ZTupleValue::ToStreamOld(const ZStreamW& iStreamW) const
	{
	switch (fType.fType)
		{
		case eZType_Time:
			{
			ZDebugStopf(kDebug_Tuple, ("Times must not have ToStreamOld called on them"));
			break;
			}
		case eZType_Vector:
			{
			ZDebugStopf(kDebug_Tuple, ("Vectors must not have ToStreamOld called on them"));
			break;
			}
		case eZType_Tuple:
			{
			iStreamW.WriteUInt8(eZType_Tuple);
			sFetch_T<ZTuple>(fType.fBytes)->ToStreamOld(iStreamW);
			break;
			}
		default:
			{
			this->ToStream(iStreamW);
			break;
			}
		}
	}

void ZTupleValue::FromStreamOld(const ZStreamR& iStreamR)
	{
	// This differs from ZTuple::FromStream only in its handling of values that are
	// tuples and that it does not support eZType_Time or eZType_Vector (which didn't exist
	// when this format was originally written).
	this->pRelease();
	int myType = iStreamR.ReadUInt8();
	switch (myType)
		{
		case eZType_Null:
			{
			// No data to read.
			break;
			}
		case eZType_Type:
			{
			fData.fAs_Type = ZType(iStreamR.ReadUInt8());
			break;
			}
		case eZType_ID:
			{
			fData.fAs_ID = iStreamR.ReadUInt64();
			break;
			}
		case eZType_Int8:
			{
			fData.fAs_Int8 = iStreamR.ReadUInt8();
			break;
			}
		case eZType_Int16:
			{
			fData.fAs_Int16 = iStreamR.ReadUInt16();
			break;
			}
		case eZType_Int32:
			{
			fData.fAs_Int32 = iStreamR.ReadUInt32();
			break;
			}
		case eZType_Int64:
			{
			fData.fAs_Int64 = iStreamR.ReadUInt64();
			break;
			}
		case eZType_Bool:
			{
			fData.fAs_Bool = iStreamR.ReadUInt8();
			break;
			}
		case eZType_Float:
			{
			fData.fAs_Float = iStreamR.ReadFloat();
			break;
			}
		case eZType_Double:
			{
			fData.fAs_Double = iStreamR.ReadDouble();
			break;
			}
		case eZType_Time:
			{
			// Not expecting a time.
			throw Ex_IllegalType(eZType_Time);
			break;
			}
		case eZType_Pointer:
			{
			fData.fAs_Pointer = reinterpret_cast<void*>(iStreamR.ReadUInt32());
			break;
			}
		case eZType_Rect:
			{
			fData.fAs_Rect = new ZRectPOD;
			fData.fAs_Rect->left = iStreamR.ReadUInt32();
			fData.fAs_Rect->top = iStreamR.ReadUInt32();
			fData.fAs_Rect->right = iStreamR.ReadUInt32();
			fData.fAs_Rect->bottom = iStreamR.ReadUInt32();
			break;
			}
		case eZType_Point:
			{
			fData.fAs_Point.h = iStreamR.ReadUInt32();
			fData.fAs_Point.v = iStreamR.ReadUInt32();
			break;
			}
		case eZType_String:
			{
			uint32 theSize = sReadCount(iStreamR);
			if (theSize <= kBytesSize)
				{
				myType = -theSize-1;
				if (theSize)
					iStreamR.Read(fType.fBytes, theSize);
				}
			else
				{
				sConstruct_T<ZTupleString>(fType.fBytes, iStreamR, theSize);
				}
			break;
			}
		case eZType_Tuple:
			{
			ZTuple* theTuple = sConstruct_T<ZTuple>(fType.fBytes);
			// This is what makes ZTupleValue::FromStreamOld
			// different from ZTupleValue::FromStream.
			theTuple->FromStreamOld(iStreamR);
			break;
			}
		case eZType_RefCounted:
			{
			// Construct a nil refcounted.
			sConstruct_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);
			break;
			}
		case eZType_Raw:
			{
			uint32 size = sReadCount(iStreamR);
			ZMemoryBlock* theRaw = sConstruct_T<ZMemoryBlock>(fType.fBytes, size);
			if (size)
				iStreamR.Read(theRaw->GetData(), size);
			break;
			}
		case eZType_Vector:
			{
			// Not expecting a vector.
			throw Ex_IllegalType(eZType_Vector);
			break;
			}
		default:
			{
			throw Ex_IllegalType(myType);
			}
		}
	fType.fType = ZType(myType);
	}

ZTupleValue::ZTupleValue(const ZTupleValue& iVal, bool iAsVector)
	{
	ZAssertStop(kDebug_Tuple, iAsVector);
	fType.fType = eZType_Vector;
	fData.fAs_Vector = new vector<ZTupleValue>(1, iVal);
	}

ZTupleValue::ZTupleValue(ZType iVal)
	{
	fType.fType = eZType_Type;
	fData.fAs_Type = iVal;
	}

ZTupleValue::ZTupleValue(uint64 iVal, bool iIsID)
	{
	ZAssertStop(kDebug_Tuple, iIsID);
	fType.fType = eZType_ID;
	fData.fAs_ID = iVal;
	}

ZTupleValue::ZTupleValue(uint64 iVal)
	{
	fType.fType = eZType_ID;
	fData.fAs_ID = iVal;
	}

ZTupleValue::ZTupleValue(int8 iVal)
	{
	fType.fType = eZType_Int8;
	fData.fAs_Int8 = iVal;
	}

ZTupleValue::ZTupleValue(int16 iVal)
	{
	fType.fType = eZType_Int16;
	fData.fAs_Int16 = iVal;
	}

ZTupleValue::ZTupleValue(int32 iVal)
	{
	fType.fType = eZType_Int32;
	fData.fAs_Int32 = iVal;
	}

ZTupleValue::ZTupleValue(int64 iVal)
	{
	fType.fType = eZType_Int64;
	fData.fAs_Int64 = iVal;
	}

ZTupleValue::ZTupleValue(bool iVal)
	{
	fType.fType = eZType_Bool;
	fData.fAs_Bool = iVal;
	}

ZTupleValue::ZTupleValue(float iVal)
	{
	fType.fType = eZType_Float;
	fData.fAs_Float = iVal;
	}

ZTupleValue::ZTupleValue(double iVal)
	{
	fType.fType = eZType_Double;
	fData.fAs_Double = iVal;
	}

ZTupleValue::ZTupleValue(ZTime iVal)
	{
	fType.fType = eZType_Time;
	fData.fAs_Time = iVal.fVal;
	}

ZTupleValue::ZTupleValue(void* inPointer)
	{
	fType.fType = eZType_Pointer;
	fData.fAs_Pointer = inPointer;
	}

ZTupleValue::ZTupleValue(const ZRectPOD& iVal)
	{
	fType.fType = eZType_Rect;
	fData.fAs_Rect = new ZRectPOD(iVal);
	}

ZTupleValue::ZTupleValue(const ZPointPOD& iVal)
	{
	fType.fType = eZType_Point;
	fData.fAs_Point = iVal;
	}

ZTupleValue::ZTupleValue(const char* iVal)
	{
	size_t theSize = strlen(iVal);
	if (theSize <= kBytesSize)
		{
		fType.fType = -theSize-1;
		if (theSize)
			ZBlockCopy(iVal, fType.fBytes, theSize);
		}
	else
		{
		fType.fType = eZType_String;
		sConstruct_T<ZTupleString>(fType.fBytes, iVal, theSize);
		}
	}

ZTupleValue::ZTupleValue(const string& iVal)
	{
	size_t theSize = iVal.size();
	if (theSize <= kBytesSize)
		{
		fType.fType = -theSize-1;
		if (theSize)
			ZBlockCopy(iVal.data(), fType.fBytes, theSize);
		}
	else
		{
		fType.fType = eZType_String;
		sConstruct_T<ZTupleString>(fType.fBytes, iVal);
		}
	}

ZTupleValue::ZTupleValue(const ZTuple& iVal)
	{
	fType.fType = eZType_Tuple;
	sConstruct_T(fType.fBytes, iVal);
	}

ZTupleValue::ZTupleValue(const ZRef<ZRefCountedWithFinalization>& iVal)
	{
	fType.fType = eZType_RefCounted;
	sConstruct_T(fType.fBytes, iVal);
	}

ZTupleValue::ZTupleValue(const ZMemoryBlock& iVal)
	{
	fType.fType = eZType_Raw;
	sConstruct_T(fType.fBytes, iVal);
	}

ZTupleValue::ZTupleValue(const void* iSource, size_t iSize)
	{
	fType.fType = eZType_Raw;
	sConstruct_T<ZMemoryBlock>(fType.fBytes, iSource, iSize);
	}

ZTupleValue::ZTupleValue(const ZStreamR& iStreamR, size_t iSize)
	{
	fType.fType = eZType_Raw;
	ZMemoryBlock* theRaw = sConstruct_T<ZMemoryBlock>(fType.fBytes);
	ZStreamRWPos_MemoryBlock(*theRaw).CopyFrom(iStreamR, iSize);
	}

ZTupleValue::ZTupleValue(const vector<ZTupleValue>& iVal)
	{
	fType.fType = eZType_Vector;
	if (iVal.empty())
		fData.fAs_Vector = nil;
	else
		fData.fAs_Vector = new vector<ZTupleValue>(iVal);
	}

int ZTupleValue::Compare(const ZTupleValue& iOther) const
	{
	if (this == &iOther)
		return 0;

	if (fType.fType < 0)
		{
		// We're a special string.
		if (iOther.fType.fType < 0)
			{
			// So is iOther
			return sCompare(fType.fBytes, -fType.fType-1,
				iOther.fType.fBytes, -iOther.fType.fType-1);
			}
		else if (iOther.fType.fType == eZType_String)
			{
			// iOther is a regular string.
			return -sFetch_T<ZTupleString>(iOther.fType.fBytes)
				->Compare(fType.fBytes, -fType.fType-1);
			}
		else
			{
			return int(eZType_String) - int(iOther.fType.fType);
			}
		}
	else
		{
		// We're not a special string.
		if (iOther.fType.fType < 0)
			{
			// iOther is a special string.
			if (fType.fType == eZType_String)
				{
				// We're a regular string.
				return sFetch_T<ZTupleString>(fType.fBytes)
					->Compare(iOther.fType.fBytes, -iOther.fType.fType-1);
				}
			else
				{
				return int(fType.fType) - int(eZType_String);
				}
			}
		}


	if (fType.fType < iOther.fType.fType)
		return -1;

	if (fType.fType > iOther.fType.fType)
		return 1;

	return this->pUncheckedCompare(iOther);
	}

bool ZTupleValue::operator==(const ZTupleValue& iOther) const
	{
	return this->Compare(iOther) == 0;
	}

bool ZTupleValue::operator<(const ZTupleValue& iOther) const
	{
	return this->Compare(iOther) < 0;
	}

bool ZTupleValue::IsSameAs(const ZTupleValue& iOther) const
	{
	if (this == &iOther)
		return true;

	if (fType.fType != iOther.fType.fType)
		return false;

	switch (fType.fType)
		{
		case eZType_Tuple:
			{
			return sFetch_T<ZTuple>(fType.fBytes)->IsSameAs(*sFetch_T<ZTuple>(iOther.fType.fBytes));
			}

		case eZType_Vector:
			{
			const vector<ZTupleValue>* thisVector = fData.fAs_Vector;
			const vector<ZTupleValue>* otherVector = iOther.fData.fAs_Vector;

			if (!thisVector || thisVector->empty())
				{
				// We're empty. We're the same if other is empty.
				return !otherVector || otherVector->empty();
				}
			else if (!otherVector || otherVector->empty())
				{
				// We're not empty, but other is, so we're different.
				return false;
				}

			vector<ZTupleValue>::const_iterator thisEnd = thisVector->end();
			vector<ZTupleValue>::const_iterator otherEnd = otherVector->end();
			
			vector<ZTupleValue>::const_iterator thisIter = thisVector->begin();
			vector<ZTupleValue>::const_iterator otherIter = otherVector->begin();
			
			for (;;)
				{
				if (thisIter == thisEnd)
					return otherIter == otherEnd;

				if (otherIter == otherEnd)
					return false;

				if (!(*thisIter).IsSameAs(*otherIter))
					return false;

				++thisIter;
				++otherIter;
				}
			}
		}

	return this->pUncheckedEqual(iOther);
	}

bool ZTupleValue::IsString(const char* iString) const
	{
	return this->GetString() == iString;
	}

bool ZTupleValue::IsString(const string& iString) const
	{
	return this->GetString() == iString;
	}

bool ZTupleValue::GetType(ZType& oVal) const
	{
	if (fType.fType == eZType_Type)
		{
		oVal = fData.fAs_Type;
		return true;
		}
	return false;
	}
	
ZType ZTupleValue::GetType() const
	{
	if (fType.fType == eZType_Type)
		return fData.fAs_Type;
	return eZType_Null;
	}

bool ZTupleValue::GetID(uint64& oVal) const
	{
	if (fType.fType == eZType_ID)
		{
		oVal = fData.fAs_ID;
		return true;
		}
	return false;
	}

uint64 ZTupleValue::GetID() const
	{
	if (fType.fType == eZType_ID)
		return fData.fAs_ID;
	return 0;
	}

bool ZTupleValue::GetInt8(int8& oVal) const
	{
	if (fType.fType == eZType_Int8)
		{
		oVal = fData.fAs_Int8;
		return true;
		}
	return false;
	}
	
int8 ZTupleValue::GetInt8() const
	{
	if (fType.fType == eZType_Int8)
		return fData.fAs_Int8;
	return 0;
	}

bool ZTupleValue::GetInt16(int16& oVal) const
	{
	if (fType.fType == eZType_Int16)
		{
		oVal = fData.fAs_Int16;
		return true;
		}
	return false;
	}

int16 ZTupleValue::GetInt16() const
	{
	if (fType.fType == eZType_Int16)
		return fData.fAs_Int16;
	return 0;
	}

bool ZTupleValue::GetInt32(int32& oVal) const
	{
	if (fType.fType == eZType_Int32)
		{
		oVal = fData.fAs_Int32;
		return true;
		}
	return false;
	}

int32 ZTupleValue::GetInt32() const
	{
	if (fType.fType == eZType_Int32)
		return fData.fAs_Int32;
	return 0;
	}

bool ZTupleValue::GetInt64(int64& oVal) const
	{
	if (fType.fType == eZType_Int64)
		{
		oVal = fData.fAs_Int64;
		return true;
		}
	return false;
	}

int64 ZTupleValue::GetInt64() const
	{
	if (fType.fType == eZType_Int64)
		return fData.fAs_Int64;
	return 0;
	}

bool ZTupleValue::GetBool(bool& oVal) const
	{
	if (fType.fType == eZType_Bool)
		{
		oVal = fData.fAs_Bool;
		return true;
		}
	return false;
	}

bool ZTupleValue::GetBool() const
	{
	if (fType.fType == eZType_Bool)
		return fData.fAs_Bool;
	return false;
	}

bool ZTupleValue::GetFloat(float& oVal) const
	{
	if (fType.fType == eZType_Float)
		{
		oVal = fData.fAs_Float;
		return true;
		}
	return false;
	}

float ZTupleValue::GetFloat() const
	{
	if (fType.fType == eZType_Float)
		return fData.fAs_Float;
	return 0.0f;
	}

bool ZTupleValue::GetDouble(double& oVal) const
	{
	if (fType.fType == eZType_Double)
		{
		oVal = fData.fAs_Double;
		return true;
		}
	return false;
	}

double ZTupleValue::GetDouble() const
	{
	if (fType.fType == eZType_Double)
		return fData.fAs_Double;
	return 0.0;
	}

bool ZTupleValue::GetTime(ZTime& oVal) const
	{
	if (fType.fType == eZType_Time)
		{
		oVal = fData.fAs_Time;
		return true;
		}
	return false;
	}

ZTime ZTupleValue::GetTime() const
	{
	if (fType.fType == eZType_Time)
		return fData.fAs_Time;
	return ZTime();
	}

bool ZTupleValue::GetPointer(void*& oVal) const
	{
	if (fType.fType == eZType_Pointer)
		{
		oVal = fData.fAs_Pointer;
		return true;
		}
	return false;
	}

void* ZTupleValue::GetPointer() const
	{
	if (fType.fType == eZType_Pointer)
		return fData.fAs_Pointer;
	return 0;
	}

bool ZTupleValue::GetRect(ZRectPOD& oVal) const
	{
	if (fType.fType == eZType_Rect)
		{
		oVal = *fData.fAs_Rect;
		return true;
		}
	return false;
	}

const ZRectPOD& ZTupleValue::GetRect() const
	{
	if (fType.fType == eZType_Rect)
		return *fData.fAs_Rect;
	return sNilRect;
	}

bool ZTupleValue::GetPoint(ZPointPOD& oVal) const
	{
	if (fType.fType == eZType_Point)
		{
		oVal = fData.fAs_Point;
		return true;
		}
	return false;
	}

const ZPointPOD& ZTupleValue::GetPoint() const
	{
	if (fType.fType == eZType_Point)
		return fData.fAs_Point;
	return sNilPoint;
	}

bool ZTupleValue::GetString(string& oVal) const
	{
	if (fType.fType == eZType_String)
		{
		sFetch_T<ZTupleString>(fType.fBytes)->ToString(oVal);
		return true;
		}
	else if (fType.fType < 0)
		{
		oVal = string(fType.fBytes, -fType.fType-1);
		return true;
		}
	return false;
	}

string ZTupleValue::GetString() const
	{
	if (fType.fType == eZType_String)
		{
		return sFetch_T<ZTupleString>(fType.fBytes)->AsString();
		}
	else if (fType.fType < 0)
		{
		return string(fType.fBytes, -fType.fType-1);
		}
	return sNilString;
	}

bool ZTupleValue::GetTuple(ZTuple& oVal) const
	{
	if (fType.fType == eZType_Tuple)
		{
		oVal = *sFetch_T<ZTuple>(fType.fBytes);
		return true;
		}
	return false;
	}

const ZTuple& ZTupleValue::GetTuple() const
	{
	if (fType.fType == eZType_Tuple)
		return *sFetch_T<ZTuple>(fType.fBytes);
	return sNilTuple;
	}

bool ZTupleValue::GetRefCounted(ZRef<ZRefCountedWithFinalization>& oVal) const
	{
	if (fType.fType == eZType_RefCounted)
		{
		oVal = *sFetch_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);
		return true;
		}
	return false;
	}

ZRef<ZRefCountedWithFinalization> ZTupleValue::GetRefCounted() const
	{
	if (fType.fType == eZType_RefCounted)
		return *sFetch_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);
	return ZRef<ZRefCountedWithFinalization>();
	}

bool ZTupleValue::GetRaw(ZMemoryBlock& oVal) const
	{
	if (fType.fType == eZType_Raw)
		{
		oVal = *sFetch_T<ZMemoryBlock>(fType.fBytes);
		return true;
		}
	return false;
	}

ZMemoryBlock ZTupleValue::GetRaw() const
	{
	if (fType.fType == eZType_Raw)
		return *sFetch_T<ZMemoryBlock>(fType.fBytes);
	return ZMemoryBlock();
	}

bool ZTupleValue::GetRaw(void* iDest, size_t iSize) const
	{
	if (fType.fType == eZType_Raw)
		{
		const ZMemoryBlock* theRaw = sFetch_T<ZMemoryBlock>(fType.fBytes);
		size_t sizeNeeded = theRaw->GetSize();
		if (iSize >= sizeNeeded)
			{
			theRaw->CopyTo(iDest, iSize);
			return true;
			}
		}
	return false;
	}

bool ZTupleValue::GetRawAttributes(const void** oAddress, size_t* oSize) const
	{
	if (fType.fType == eZType_Raw)
		{
		const ZMemoryBlock* theRaw = sFetch_T<ZMemoryBlock>(fType.fBytes);
		if (oAddress)
			*oAddress = theRaw->GetData();
		if (oSize)
			*oSize = theRaw->GetSize();
		return true;
		}
	return false;
	}

bool ZTupleValue::GetVector(vector<ZTupleValue>& oVal) const
	{
	if (fType.fType == eZType_Vector)
		{
		if (fData.fAs_Vector)
			oVal = *fData.fAs_Vector;
		else
			oVal.clear();
		return true;
		}
	return false;
	}

const vector<ZTupleValue>& ZTupleValue::GetVector() const
	{
	if (fType.fType == eZType_Vector)
		{
		if (fData.fAs_Vector)
			return *fData.fAs_Vector;
		}
	return sNilVector;
	}
	
ZType ZTupleValue::DGetType(ZType iDefault) const
	{
	if (fType.fType == eZType_Type)
		return fData.fAs_Type;
	return iDefault;
	}

uint64 ZTupleValue::DGetID(uint64 iDefault) const
	{
	if (fType.fType == eZType_ID)
		return fData.fAs_ID;
	return iDefault;
	}
	
int8 ZTupleValue::DGetInt8(int8 iDefault) const
	{
	if (fType.fType == eZType_Int8)
		return fData.fAs_Int8;
	return iDefault;
	}

int16 ZTupleValue::DGetInt16(int16 iDefault) const
	{
	if (fType.fType == eZType_Int16)
		return fData.fAs_Int16;
	return iDefault;
	}

int32 ZTupleValue::DGetInt32(int32 iDefault) const
	{
	if (fType.fType == eZType_Int32)
		return fData.fAs_Int32;
	return iDefault;
	}

int64 ZTupleValue::DGetInt64(int64 iDefault) const
	{
	if (fType.fType == eZType_Int64)
		return fData.fAs_Int64;
	return iDefault;
	}

bool ZTupleValue::DGetBool(bool iDefault) const
	{
	if (fType.fType == eZType_Bool)
		return fData.fAs_Bool;
	return iDefault;
	}

float ZTupleValue::DGetFloat(float iDefault) const
	{
	if (fType.fType == eZType_Float)
		return fData.fAs_Float;
	return iDefault;
	}

double ZTupleValue::DGetDouble(double iDefault) const
	{
	if (fType.fType == eZType_Double)
		return fData.fAs_Double;
	return iDefault;
	}

ZTime ZTupleValue::DGetTime(ZTime iDefault) const
	{
	if (fType.fType == eZType_Time)
		return fData.fAs_Time;
	return iDefault;
	}

ZRectPOD ZTupleValue::DGetRect(const ZRectPOD& iDefault) const
	{
	if (fType.fType == eZType_Rect)
		return *fData.fAs_Rect;
	return iDefault;
	}

ZPointPOD ZTupleValue::DGetPoint(const ZPointPOD& iDefault) const
	{
	if (fType.fType == eZType_Point)
		return fData.fAs_Point;
	return iDefault;
	}

string ZTupleValue::DGetString(const string& iDefault) const
	{
	if (fType.fType == eZType_String)
		{
		return sFetch_T<ZTupleString>(fType.fBytes)->AsString();
		}
	else if (fType.fType < 0)
		{
		return string(fType.fBytes, -fType.fType-1);
		}
	return iDefault;
	}

void ZTupleValue::SetNull()
	{
	this->pRelease();
	fType.fType = eZType_Null;
	}

void ZTupleValue::SetType(ZType iVal)
	{
	this->pRelease();
	fType.fType = eZType_Type;
	fData.fAs_Type = iVal;
	}

void ZTupleValue::SetID(uint64 iVal)
	{
	this->pRelease();
	fType.fType = eZType_ID;
	fData.fAs_ID = iVal;
	}

void ZTupleValue::SetInt8(int8 iVal)
	{
	this->pRelease();
	fType.fType = eZType_Int8;
	fData.fAs_Int8 = iVal;
	}

void ZTupleValue::SetInt16(int16 iVal)
	{
	this->pRelease();
	fType.fType = eZType_Int16;
	fData.fAs_Int16 = iVal;
	}

void ZTupleValue::SetInt32(int32 iVal)
	{
	this->pRelease();
	fType.fType = eZType_Int32;
	fData.fAs_Int32 = iVal;
	}

void ZTupleValue::SetInt64(int64 iVal)
	{
	this->pRelease();
	fType.fType = eZType_Int64;
	fData.fAs_Int64 = iVal;
	}

void ZTupleValue::SetBool(bool iVal)
	{
	this->pRelease();
	fType.fType = eZType_Bool;
	fData.fAs_Bool = iVal;
	}

void ZTupleValue::SetFloat(float iVal)
	{
	this->pRelease();
	fType.fType = eZType_Float;
	fData.fAs_Float = iVal;
	}

void ZTupleValue::SetDouble(double iVal)
	{
	this->pRelease();
	fType.fType = eZType_Double;
	fData.fAs_Double = iVal;
	}

void ZTupleValue::SetTime(ZTime iVal)
	{
	this->pRelease();
	fType.fType = eZType_Time;
	fData.fAs_Time = iVal.fVal;
	}

void ZTupleValue::SetPointer(void* iVal)
	{
	this->pRelease();
	fType.fType = eZType_Pointer;
	fData.fAs_Pointer = iVal;
	}

void ZTupleValue::SetRect(const ZRectPOD& iVal)
	{
	this->pRelease();
	fType.fType = eZType_Rect;
	fData.fAs_Rect = new ZRectPOD(iVal);
	}

void ZTupleValue::SetPoint(const ZPointPOD& iVal)
	{
	this->pRelease();
	fType.fType = eZType_Point;
	fData.fAs_Point = iVal;
	}

void ZTupleValue::SetString(const char* iVal)
	{
	this->pRelease();
	size_t theSize = strlen(iVal);
	if (theSize <= kBytesSize)
		{
		fType.fType = -theSize-1;
		if (theSize)
			ZBlockCopy(iVal, fType.fBytes, theSize);
		}
	else
		{
		fType.fType = eZType_String;
		sConstruct_T<ZTupleString>(fType.fBytes, iVal, theSize);
		}
	}

void ZTupleValue::SetString(const string& iVal)
	{
	this->pRelease();
	size_t theSize = iVal.size();
	if (theSize <= kBytesSize)
		{
		fType.fType = -theSize-1;
		if (theSize)
			ZBlockCopy(iVal.data(), fType.fBytes, theSize);
		}
	else
		{
		fType.fType = eZType_String;
		sConstruct_T<ZTupleString>(fType.fBytes, iVal);
		}
	}

void ZTupleValue::SetTuple(const ZTuple& iVal)
	{
	this->pRelease();
	fType.fType = eZType_Tuple;
	sConstruct_T(fType.fBytes, iVal);
	}

void ZTupleValue::SetRefCounted(const ZRef<ZRefCountedWithFinalization>& iVal)
	{
	this->pRelease();
	fType.fType = eZType_RefCounted;
	sConstruct_T(fType.fBytes, iVal);
	}

void ZTupleValue::SetRaw(const ZMemoryBlock& iVal)
	{
	this->pRelease();
	fType.fType = eZType_Raw;
	sConstruct_T(fType.fBytes, iVal);
	}

void ZTupleValue::SetRaw(const void* iSource, size_t iSize)
	{
	this->pRelease();
	fType.fType = eZType_Raw;
	sConstruct_T<ZMemoryBlock>(fType.fBytes, iSource, iSize);
	}

void ZTupleValue::SetRaw(const ZStreamR& iStreamR, size_t iSize)
	{
	ZMemoryBlock* theMemoryBlock;
	if (fType.fType != eZType_Raw)
		{
		this->pRelease();
		fType.fType = eZType_Raw;
		theMemoryBlock = sConstruct_T<ZMemoryBlock>(fType.fBytes, iSize);
		}
	else
		{
		theMemoryBlock = sFetch_T<ZMemoryBlock>(fType.fBytes);
		theMemoryBlock->SetSize(iSize);
		}
	ZStreamRWPos_MemoryBlock(*theMemoryBlock).CopyFrom(iStreamR, iSize);
	}

void ZTupleValue::SetRaw(const ZStreamR& iStreamR)
	{
	ZMemoryBlock* theMemoryBlock;
	if (fType.fType != eZType_Raw)
		{
		this->pRelease();
		fType.fType = eZType_Raw;
		theMemoryBlock = sConstruct_T<ZMemoryBlock>(fType.fBytes);
		}
	else
		{
		theMemoryBlock = sFetch_T<ZMemoryBlock>(fType.fBytes);
		theMemoryBlock->SetSize(0);
		}
	ZStreamRWPos_MemoryBlock(*theMemoryBlock).CopyAllFrom(iStreamR);
	}

void ZTupleValue::SetVector(const vector<ZTupleValue>& iVal)
	{
	this->pRelease();
	fType.fType = eZType_Vector;
	if (iVal.empty())
		fData.fAs_Vector= nil;
	else
		fData.fAs_Vector = new vector<ZTupleValue>(iVal);
	}

ZTuple& ZTupleValue::GetMutableTuple()
	{
	// We're returning a non-const tuple, one that can be manipulated
	// by the caller, so we must actually *be* a tuple.
	ZAssertStop(kDebug_Tuple, fType.fType == eZType_Tuple);

	return *sFetch_T<ZTuple>(fType.fBytes);
	}

vector<ZTupleValue>& ZTupleValue::GetMutableVector()
	{
	// We're returning a non-const vector, one that can be manipulated
	// by the caller. So we must actually *be* a vector, we can't
	// return the usual dummy default value.
	ZAssertStop(kDebug_Tuple, fType.fType == eZType_Vector);

	if (!fData.fAs_Vector)
		{
		// The empty vector optimization is in effect, but
		// we need a real vector to return.
		fData.fAs_Vector = new vector<ZTupleValue>;
		}
	return *fData.fAs_Vector;
	}

ZTuple& ZTupleValue::EnsureMutableTuple()
	{
	if (fType.fType == eZType_Tuple)
		return *sFetch_T<ZTuple>(fType.fBytes);

	this->pRelease();
	fType.fType = eZType_Tuple;
	return *sConstruct_T<ZTuple>(fType.fBytes);
	}

vector<ZTupleValue>& ZTupleValue::EnsureMutableVector()
	{
	if (fType.fType == eZType_Vector)
		{
		if (!fData.fAs_Vector)
			fData.fAs_Vector = new vector<ZTupleValue>;
		}
	else
		{
		this->pRelease();
		fType.fType = eZType_Vector;
		fData.fAs_Vector = nil;
		fData.fAs_Vector = new vector<ZTupleValue>;
		}
	return *fData.fAs_Vector;	
	}

ZTupleValue& ZTupleValue::SetMutableNull()
	{
	this->pRelease();
	fType.fType = eZType_Null;
	return *this;
	}

ZTuple& ZTupleValue::SetMutableTuple()
	{
	this->pRelease();
	fType.fType = eZType_Tuple;
	return *sConstruct_T<ZTuple>(fType.fBytes);
	}

vector<ZTupleValue>& ZTupleValue::SetMutableVector()
	{
	// SetMutableVector is usually called so that external code
	// can populate the vector that's actually stored by us.
	// So we don't want to use the empty vector optimization
	// and thus must ensure we actually have a vector allocated.
	if (fType.fType == eZType_Vector)
		{
		if (fData.fAs_Vector)
			fData.fAs_Vector->clear();
		else
			fData.fAs_Vector = new vector<ZTupleValue>;
		}
	else
		{
		this->pRelease();
		fType.fType = eZType_Vector;
		fData.fAs_Vector = nil;
		fData.fAs_Vector = new vector<ZTupleValue>;
		}
	return *fData.fAs_Vector;
	}

void ZTupleValue::AppendValue(const ZTupleValue& iVal)
	{
	if (fType.fType == eZType_Vector)
		{
		if (!fData.fAs_Vector)
			fData.fAs_Vector = new vector<ZTupleValue>(1, iVal);
		else
			fData.fAs_Vector->push_back(iVal);
		}
	else if (fType.fType == eZType_Null)
		{
		fData.fAs_Vector = new vector<ZTupleValue>(1, iVal);
		fType.fType = eZType_Vector;
		}
	else
		{
		vector<ZTupleValue>* newVector = new vector<ZTupleValue>(1, *this);
		try
			{
			newVector->push_back(iVal);
			}
		catch (...)
			{
			delete newVector;
			throw;
			}

		this->pRelease();
		fType.fType = eZType_Vector;
		fData.fAs_Vector = newVector;
		}
	}

int ZTupleValue::pUncheckedCompare(const ZTupleValue& iOther) const
	{
	// Our types are assumed to be the same, and not special strings.
	ZAssertStop(kDebug_Tuple, fType.fType == iOther.fType.fType);
	switch (fType.fType)
		{
		case eZType_Null: return 0;
		case eZType_Type: return ZooLib::sCompare_T(fData.fAs_Type, iOther.fData.fAs_Type);
		case eZType_ID: return ZooLib::sCompare_T(fData.fAs_ID, iOther.fData.fAs_ID);
		case eZType_Int8: return ZooLib::sCompare_T(fData.fAs_Int8, iOther.fData.fAs_Int8);
		case eZType_Int16: return ZooLib::sCompare_T(fData.fAs_Int16, iOther.fData.fAs_Int16);
		case eZType_Int32: return ZooLib::sCompare_T(fData.fAs_Int32, iOther.fData.fAs_Int32);
		case eZType_Int64: return ZooLib::sCompare_T(fData.fAs_Int64, iOther.fData.fAs_Int64);
		case eZType_Bool: return ZooLib::sCompare_T(fData.fAs_Bool, iOther.fData.fAs_Bool);
		case eZType_Float:  return ZooLib::sCompare_T(fData.fAs_Float, iOther.fData.fAs_Float);
		case eZType_Double: return ZooLib::sCompare_T(fData.fAs_Double, iOther.fData.fAs_Double);
		case eZType_Time: return ZooLib::sCompare_T(fData.fAs_Time, iOther.fData.fAs_Time);
		case eZType_Pointer: return ZooLib::sCompare_T(fData.fAs_Pointer, iOther.fData.fAs_Pointer);
		case eZType_Rect: return ZooLib::sCompare_T(*fData.fAs_Rect, *iOther.fData.fAs_Rect);
		case eZType_Point: return ZooLib::sCompare_T(fData.fAs_Point, iOther.fData.fAs_Point);
		case eZType_String:
			{
			return sFetch_T<ZTupleString>(fType.fBytes)
				->Compare(*sFetch_T<ZTupleString>(iOther.fType.fBytes));
			}
		case eZType_Tuple:
			{
			return sFetch_T<ZTuple>(fType.fBytes)->Compare(*sFetch_T<ZTuple>(iOther.fType.fBytes));
			}
		case eZType_RefCounted:
			{
			const ZRef<ZRefCountedWithFinalization>* thisZRef
				= sFetch_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);

			const ZRef<ZRefCountedWithFinalization>* otherZRef
				= sFetch_T<ZRef<ZRefCountedWithFinalization> >(iOther.fType.fBytes);
			
			if (*thisZRef < *otherZRef)
				return -1;
			else if (*otherZRef < *thisZRef)
				return 1;
			return 0;
			}
		case eZType_Raw:
			{
			return sFetch_T<ZMemoryBlock>(fType.fBytes)->Compare(
				*sFetch_T<ZMemoryBlock>(iOther.fType.fBytes));
			}
		case eZType_Vector:
			{
			if (!fData.fAs_Vector || fData.fAs_Vector->empty())
				{
				// We're empty.
				if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
					{
					// And so is other, so we're equal.
					return 0;
					}
				else
					{
					// Other has data, so we're less than it.
					return -1;
					}
				}
			else if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
				{
				// We're not empty, but iOther is, so we're greater.
				return 1;
				}
			return ZooLib::sCompare_T(*fData.fAs_Vector, *iOther.fData.fAs_Vector);
			}
		}
	ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
	return 0;
	}

bool ZTupleValue::pUncheckedLess(const ZTupleValue& iOther) const
	{
	// Our types are assumed to be the same, and not special strings.
	ZAssertStop(kDebug_Tuple, fType.fType == iOther.fType.fType);
	switch (fType.fType)
		{
		case eZType_Null: return false;
		case eZType_Type: return fData.fAs_Type < iOther.fData.fAs_Type;
		case eZType_ID: return fData.fAs_ID < iOther.fData.fAs_ID;
		case eZType_Int8: return fData.fAs_Int8 < iOther.fData.fAs_Int8;
		case eZType_Int16: return fData.fAs_Int16 < iOther.fData.fAs_Int16;
		case eZType_Int32: return fData.fAs_Int32 < iOther.fData.fAs_Int32;
		case eZType_Int64: return fData.fAs_Int64 < iOther.fData.fAs_Int64;
		case eZType_Bool: return fData.fAs_Bool < iOther.fData.fAs_Bool;
		case eZType_Float: return fData.fAs_Float < iOther.fData.fAs_Float;
		case eZType_Double: return fData.fAs_Double < iOther.fData.fAs_Double;
		case eZType_Time: return fData.fAs_Time < iOther.fData.fAs_Time;
		case eZType_Pointer: return fData.fAs_Pointer < iOther.fData.fAs_Pointer;
		case eZType_Rect: return ZooLib::sCompare_T(*fData.fAs_Rect, *iOther.fData.fAs_Rect) < 0;
		case eZType_Point: return ZooLib::sCompare_T(fData.fAs_Point, iOther.fData.fAs_Point) < 0;

		case eZType_String:
			{
			return 0 > sFetch_T<ZTupleString>(fType.fBytes)
				->Compare(*sFetch_T<ZTupleString>(iOther.fType.fBytes));
			}
		case eZType_Tuple:
			{
			return *sFetch_T<ZTuple>(fType.fBytes) < *sFetch_T<ZTuple>(iOther.fType.fBytes);
			}
		case eZType_RefCounted:
			{
			return *sFetch_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes)
				< *sFetch_T<ZRef<ZRefCountedWithFinalization> >(iOther.fType.fBytes);
			}
		case eZType_Raw:
			{
			return *sFetch_T<ZMemoryBlock>(fType.fBytes)
				< *sFetch_T<ZMemoryBlock>(iOther.fType.fBytes);
			}
		case eZType_Vector:
			{
			if (!fData.fAs_Vector || fData.fAs_Vector->empty())
				{
				// We're empty.
				if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
					{
					// And so is other, so we're not less than it.
					return false;
					}
				else
					{
					// Other has data, so we're less than it.
					return true;
					}
				}
			else if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
				{
				// We're not empty, but iOther is, so we're not less than it.
				return false;
				}
			return (*fData.fAs_Vector) < (*iOther.fData.fAs_Vector);
			}
		}
	ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
	return false;
	}

bool ZTupleValue::pUncheckedEqual(const ZTupleValue& iOther) const
	{
	// Our types are assumed to be the same, and not special strings.
	ZAssertStop(kDebug_Tuple, fType.fType == iOther.fType.fType);
	switch (fType.fType)
		{
		case eZType_Null: return true;
		case eZType_Type: return fData.fAs_Type == iOther.fData.fAs_Type;
		case eZType_ID: return fData.fAs_ID == iOther.fData.fAs_ID;
		case eZType_Int8: return fData.fAs_Int8 == iOther.fData.fAs_Int8;
		case eZType_Int16: return fData.fAs_Int16 == iOther.fData.fAs_Int16;
		case eZType_Int32: return fData.fAs_Int32 == iOther.fData.fAs_Int32;
		case eZType_Int64: return fData.fAs_Int64 == iOther.fData.fAs_Int64;
		case eZType_Bool: return fData.fAs_Bool == iOther.fData.fAs_Bool;
		case eZType_Float: return fData.fAs_Float == iOther.fData.fAs_Float;
		case eZType_Double: return fData.fAs_Double == iOther.fData.fAs_Double;
		case eZType_Time: return fData.fAs_Time == iOther.fData.fAs_Time;
		case eZType_Pointer: return fData.fAs_Pointer == iOther.fData.fAs_Pointer;
		case eZType_Rect: return fData.fAs_Rect->left == iOther.fData.fAs_Rect->left
							&& fData.fAs_Rect->top == iOther.fData.fAs_Rect->top
							&& fData.fAs_Rect->right == iOther.fData.fAs_Rect->right
							&& fData.fAs_Rect->bottom == iOther.fData.fAs_Rect->bottom;
		case eZType_Point: return fData.fAs_Point.h == iOther.fData.fAs_Point.h
							&& fData.fAs_Point.v == iOther.fData.fAs_Point.v;

		case eZType_String:
			{
			return 0 == sFetch_T<ZTupleString>(fType.fBytes)
				->Compare(*sFetch_T<ZTupleString>(iOther.fType.fBytes));
			}
		case eZType_Tuple:
			{
			return *sFetch_T<ZTuple>(fType.fBytes) == *sFetch_T<ZTuple>(iOther.fType.fBytes);
			}
		case eZType_RefCounted:
			{
			return *sFetch_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes)
				== *sFetch_T<ZRef<ZRefCountedWithFinalization> >(iOther.fType.fBytes);
			}
		case eZType_Raw:
			{
			return *sFetch_T<ZMemoryBlock>(fType.fBytes)
				== *sFetch_T<ZMemoryBlock>(iOther.fType.fBytes);
			}
		case eZType_Vector:
			{
			if (!fData.fAs_Vector || fData.fAs_Vector->empty())
				{
				// We're empty.
				if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
					{
					// And so is other, so we're equal.
					return true;
					}
				else
					{
					// Other has data, so we're not equal.
					return false;
					}
				}
			else if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
				{
				// We're not empty, but iOther is, so we're not equal.
				return false;
				}
			return (*fData.fAs_Vector) == (*iOther.fData.fAs_Vector);
			}
		}
	ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
	return false;
	}

void ZTupleValue::pRelease()
	{
	switch (fType.fType)
		{
		case eZType_Null:
		case eZType_Type:
		case eZType_ID:
		case eZType_Int8:
		case eZType_Int16:
		case eZType_Int32:
		case eZType_Int64:
		case eZType_Bool:
		case eZType_Float:
		case eZType_Double:
		case eZType_Time:
		case eZType_Pointer:
		case eZType_Point:
			break;
		case eZType_Rect: delete fData.fAs_Rect; break;

		case eZType_String: sDestroy_T<ZTupleString>(fType.fBytes); break;
		case eZType_Tuple: sDestroy_T<ZTuple>(fType.fBytes); break;
		case eZType_RefCounted:
			{
			sDestroy_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);
			break;
			}
		case eZType_Raw: sDestroy_T<ZMemoryBlock>(fType.fBytes); break;
		case eZType_Vector: delete fData.fAs_Vector; break;
		default:
			{
			if (fType.fType >= 0)
				ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
			}
		}
	}

void ZTupleValue::pCopy(const ZTupleValue& iOther)
	{
	if (iOther.fType.fType < 0)
		{
		fFastCopy = iOther.fFastCopy;
		}
	else
		{
		fType.fType = iOther.fType.fType;
		switch (fType.fType)
			{
			case eZType_Null:
				// No data to be copied.
				break;
			case eZType_Type:
				fData.fAs_Type = iOther.fData.fAs_Type;
				break;
			case eZType_ID:
				fData.fAs_ID = iOther.fData.fAs_ID;
				break;
			case eZType_Int8:
				fData.fAs_Int8 = iOther.fData.fAs_Int8;
				break;
			case eZType_Int16:
				fData.fAs_Int16 = iOther.fData.fAs_Int16;
				break;
			case eZType_Int32:
				fData.fAs_Int32 = iOther.fData.fAs_Int32;
				break;
			case eZType_Int64:
				fData.fAs_Int64 = iOther.fData.fAs_Int64;
				break;
			case eZType_Bool:
				fData.fAs_Bool = iOther.fData.fAs_Bool;
				break;
			case eZType_Float:
				fData.fAs_Float = iOther.fData.fAs_Float;
				break;
			case eZType_Double:
				fData.fAs_Double = iOther.fData.fAs_Double;
				break;
			case eZType_Time:
				fData.fAs_Time = iOther.fData.fAs_Time;
				break;
			case eZType_Pointer:
				fData.fAs_Pointer = iOther.fData.fAs_Pointer;
				break;
			case eZType_Rect:
				fData.fAs_Rect = new ZRectPOD(*iOther.fData.fAs_Rect);
				break;
			case eZType_Point:
				fData.fAs_Point = iOther.fData.fAs_Point;
				break;
			case eZType_String:
				sCopyConstruct_T<ZTupleString>(iOther.fType.fBytes, fType.fBytes);
				break;
			case eZType_Tuple:
				sCopyConstruct_T<ZTuple>(iOther.fType.fBytes, fType.fBytes);
				break;
			case eZType_RefCounted:
				sCopyConstruct_T<ZRef<ZRefCountedWithFinalization> >
					(iOther.fType.fBytes, fType.fBytes);				
				break;
			case eZType_Raw:
				sCopyConstruct_T<ZMemoryBlock>(iOther.fType.fBytes, fType.fBytes);
				break;
			case eZType_Vector:
				if (!iOther.fData.fAs_Vector || iOther.fData.fAs_Vector->empty())
					fData.fAs_Vector = nil;
				else
					fData.fAs_Vector = new vector<ZTupleValue>(*iOther.fData.fAs_Vector);
				break;
			default:
				ZDebugStopf(kDebug_Tuple, ("Unknown type (%d)", fType.fType));
			}
		}
	}

static vector<ZTupleValue>* sAllocVector(size_t iSize)
	{
	return new vector<ZTupleValue>(iSize, ZTupleValue());
	}

static ZRectPOD* sAllocRect()
	{
	return new ZRectPOD;
	}

void ZTupleValue::pFromStream(const ZStreamR& iStreamR)
	{
	int myType = iStreamR.ReadUInt8();
	switch (myType)
		{
		case eZType_Null:
			{
			// No data to read.
			break;
			}
		case eZType_Type:
			{
			fData.fAs_Type = ZType(iStreamR.ReadUInt8());
			break;
			}
		case eZType_ID:
			{
			fData.fAs_ID = iStreamR.ReadUInt64();
			break;
			}
		case eZType_Int8:
			{
			fData.fAs_Int8 = iStreamR.ReadUInt8();
			break;
			}
		case eZType_Int16:
			{
			fData.fAs_Int16 = iStreamR.ReadUInt16();
			break;
			}
		case eZType_Int32:
			{
			fData.fAs_Int32 = iStreamR.ReadUInt32();
			break;
			}
		case eZType_Int64:
			{
			fData.fAs_Int64 = iStreamR.ReadUInt64();
			break;
			}
		case eZType_Bool:
			{
			fData.fAs_Bool = iStreamR.ReadUInt8();
			break;
			}
		case eZType_Float:
			{
			fData.fAs_Float = iStreamR.ReadFloat();
			break;
			}
		case eZType_Double:
			{
			fData.fAs_Double = iStreamR.ReadDouble();
			break;
			}
		case eZType_Time:
			{
			fData.fAs_Time = iStreamR.ReadDouble();
			break;
			}
		case eZType_Pointer:
			{
			fData.fAs_Pointer = reinterpret_cast<void*>(iStreamR.ReadUInt32());
			break;
			}
		case eZType_Rect:
			{
			ZRectPOD* theRect = sAllocRect();
			try
				{
				theRect->left = iStreamR.ReadUInt32();
				theRect->top = iStreamR.ReadUInt32();
				theRect->right = iStreamR.ReadUInt32();
				theRect->bottom = iStreamR.ReadUInt32();
				}
			catch (...)
				{
				delete theRect;
				throw;
				}
			fData.fAs_Rect = theRect;
			break;
			}
		case eZType_Point:
			{
			fData.fAs_Point.h = iStreamR.ReadUInt32();
			fData.fAs_Point.v = iStreamR.ReadUInt32();
			break;
			}
		case eZType_String:
			{
			uint32 theSize = sReadCount(iStreamR);
			if (theSize <= kBytesSize)
				{
				myType = -theSize-1;
				if (theSize)
					iStreamR.Read(fType.fBytes, theSize);
				}
			else
				{
				sConstruct_T<ZTupleString>(fType.fBytes, iStreamR, theSize);
				}
			break;
			}
		case eZType_Tuple:
			{
			sConstruct_T<ZTuple>(fType.fBytes, iStreamR);
			break;
			}
		case eZType_RefCounted:
			{
			// Construct a nil refcounted.
			sConstruct_T<ZRef<ZRefCountedWithFinalization> >(fType.fBytes);
			break;
			}
		case eZType_Raw:
			{
			uint32 size = sReadCount(iStreamR);
			ZMemoryBlock* theRaw = sConstruct_T<ZMemoryBlock>(fType.fBytes, size);
			if (size)
				{
				try
					{
					iStreamR.Read(theRaw->GetData(), size);
					}
				catch (...)
					{
					delete theRaw;
					throw;
					}
				}
			break;
			}
		case eZType_Vector:
			{
			if (uint32 theCount = sReadCount(iStreamR))
				{
				vector<ZTupleValue>* theVector = sAllocVector(theCount);
				for (size_t x = 0; x < theCount; ++x)
					{
					try
						{
						(*theVector)[x].FromStream(iStreamR);
						}
					catch (...)
						{
						delete theVector;
						throw;
						}
					}
				fData.fAs_Vector = theVector;
				}
			else
				{
				fData.fAs_Vector = nil;
				}
			break;
			}
		default:
			{
			throw Ex_IllegalType(myType);
			}
		}
	fType.fType = ZType(myType);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple

/** \class ZTuple
\nosubgrouping
*/

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple::NameTV

inline ZTuple::NameTV::NameTV()
	{}

inline ZTuple::NameTV::NameTV(const NameTV& iOther)
:	fTV(iOther.fTV),
	fName(iOther.fName)
	{}

inline ZTuple::NameTV::NameTV(const char* iName, const ZTupleValue& iTV)
:	fTV(iTV),
	fName(iName)
	{}

inline ZTuple::NameTV::NameTV(const string& iName, const ZTupleValue& iTV)
:	fTV(iTV),
	fName(iName)
	{}

inline ZTuple::NameTV::NameTV(const char* iName)
:	fName(iName)
	{}

inline ZTuple::NameTV::NameTV(const string& iName)
:	fName(iName)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Constructing/assigning from stream

ZTuple::ZTuple(const ZStreamR& iStreamR)
:	fRep(sRepFromStream(iStreamR))
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple streaming

void ZTuple::ToStream(const ZStreamW& iStreamW) const
	{
	if (!fRep || fRep->fProperties.empty())
		{
		sWriteCount(iStreamW, 0);
		}
	else
		{
		const PropList& properties = fRep->fProperties;
		sWriteCount(iStreamW, properties.size());
		for (PropList::const_iterator i = properties.begin(); i != properties.end(); ++i)
			{
			(*i).fName.ToStream(iStreamW);
			(*i).fTV.ToStream(iStreamW);
			}
		}
	}

void ZTuple::FromStream(const ZStreamR& iStreamR)
	{ fRep = sRepFromStream(iStreamR); }

void ZTuple::ToStreamOld(const ZStreamW& iStreamW) const
	{
	if (!fRep || fRep->fProperties.empty())
		{
		sWriteCount(iStreamW, 0);
		}
	else
		{
		const PropList& properties = fRep->fProperties;
		sWriteCount(iStreamW, properties.size());
		for (PropList::const_iterator i = properties.begin(); i != properties.end(); ++i)
			{
			(*i).fName.ToStream(iStreamW);

			if (eZType_Vector == (*i).fTV.TypeOf())
				{
				const vector<ZTupleValue>& theVector = (*i).fTV.GetVector();
				sWriteCount(iStreamW, theVector.size());
				for (vector<ZTupleValue>::const_iterator j = theVector.begin();
					j != theVector.end(); ++j)
					{
					(*j).ToStreamOld(iStreamW);
					}
				}
			else
				{
				sWriteCount(iStreamW, 1);
				(*i).fTV.ToStreamOld(iStreamW);
				}
			}
		}
	}

void ZTuple::FromStreamOld(const ZStreamR& iStreamR)
	{
	ZRef<ZTupleRep> newRep;
	if (uint32 propertyCount = sReadCount(iStreamR))
		{
		newRep = new ZTupleRep;
		PropList& properties = newRep->fProperties;

		// It's a vector
		properties.reserve(propertyCount);
		properties.resize(propertyCount);
		for (PropList::iterator i = properties.begin(); i != properties.end(); ++i)
			{
			(*i).fName.FromStream(iStreamR);

			if (uint32 valueCount = sReadCount(iStreamR))
				{
				if (valueCount == 1)
					{
					(*i).fTV.FromStreamOld(iStreamR);
					}
				else
					{
					vector<ZTupleValue>& theVector = (*i).fTV.SetMutableVector();
					theVector.reserve(valueCount);
					theVector.resize(valueCount);
					for (vector<ZTupleValue>::iterator j = theVector.begin();
						j != theVector.end(); ++j)
						{
						(*j).FromStreamOld(iStreamR);
						}
					}
				}
			}
		}
	fRep = newRep;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple unioning

ZTuple ZTuple::Over(const ZTuple& iUnder) const
	{
	ZTuple result = iUnder;
	if (fRep)
		{
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			result.SetValue((*i).fName.AsString(), (*i).fTV);
			}
		}
	return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple comparison

int ZTuple::Compare(const ZTuple& iOther) const
	{
	if (fRep == iOther.fRep)
		{
		// We share the same rep, so we're identical.
		return 0;
		}

	if (!fRep)
		{
		// We have no rep, and iOther must have a rep (or fRep would be == iOther.fRep).
		if (iOther.fRep->fProperties.empty())
			{
			// And iOther's property list is empty, we're equivalent.
			return 0;
			}
		else
			{
			// iOther has properties, so we're less than it.
			return -1;
			}
		}

	if (!iOther.fRep)
		{
		// iOther has no rep.
		if (fRep->fProperties.empty())
			{
			// And our property list is empty, so we're equivalent.
			return 0;
			}
		else
			{
			// We have properties, so we're greater than iOther.
			return 1;
			}
		}

	for (PropList::const_iterator iterThis = fRep->fProperties.begin(),
			iterOther = iOther.fRep->fProperties.begin(),
			endThis = fRep->fProperties.end(),
			endOther = iOther.fRep->fProperties.end();
			/*no test*/; ++iterThis, ++iterOther)
		{
		if (iterThis != endThis)
			{
			// This is not exhausted.
			if (iterOther != endOther)
				{
				// Other is not exhausted either, so we compare their current values.
				if (int compare = (*iterThis).fName.Compare((*iterOther).fName))
					{
					// The names are different.
					return compare;
					}
				if (int compare = (*iterThis).fTV.Compare((*iterOther).fTV))
					{
					// The values are different.
					return compare;
					}
				}
			else
				{
				// Exhausted other, but still have this
				// remaining, so this is greater than other.
				return 1;
				}
			}
		else
			{
			// Exhausted this.
			if (iterOther != endOther)
				{
				// Still have other remaining, so this is less than other.
				return -1;
				}
			else
				{
				// Exhausted other. And as this is also
				// exhausted this equals other.
				return 0;
				}
			}
		}
	}

bool ZTuple::operator<(const ZTuple& iOther) const
	{
	return this->Compare(iOther) < 0;
	}

bool ZTuple::Contains(const ZTuple& iOther) const
	{
	if (fRep == iOther.fRep)
		{
		// We have the same rep, and so we contain other.
		return true;
		}

	if (!iOther.fRep || iOther.fRep->fProperties.empty())
		{
		// Other doesn't have a rep or doesn't have any properties, and thus we contain it.
		return true;
		}

	if (!fRep)
		{
		// Other has some properties, or we would have returned above, but
		// we don't have a rep so we can't contain other.
		return false;
		}


	if (fRep->fProperties.size() < iOther.fRep->fProperties.size())
		{
		// We have fewer properties than iOther, and thus can't contain it.
		return false;
		}

	for (PropList::const_iterator iterOther = iOther.fRep->fProperties.begin(),
			endOther = iOther.fRep->fProperties.end(),
			endThis = fRep->fProperties.end();
			iterOther != endOther; ++iterOther)
		{
		const_iterator iterThis = this->IteratorOf((*iterOther).fName);
		if (iterThis == endThis)
			return false;
		if (!(*iterThis).fTV.IsSameAs((*iterOther).fTV))
			return false;
		}

	return true;
	}

bool ZTuple::IsString(const_iterator iPropIter, const char* iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropIter))
		return theValue->IsString(iString);
	return false;
	}

bool ZTuple::IsString(const char* iPropName, const char* iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return theValue->IsString(iString);
	return false;
	}

bool ZTuple::IsString(const string& iPropName, const char* iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return theValue->IsString(iString);
	return false;
	}

bool ZTuple::IsString(const_iterator iPropIter, const string& iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropIter))
		return theValue->IsString(iString);
	return false;
	}

bool ZTuple::IsString(const char* iPropName, const string& iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return theValue->IsString(iString);
	return false;
	}

bool ZTuple::IsString(const string& iPropName, const string& iString) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return theValue->IsString(iString);
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple property meta-data

ZTuple::const_iterator ZTuple::begin() const
	{
	if (fRep)
		return fRep->fProperties.begin();
	return sEmptyProperties.end();
	}

ZTuple::const_iterator ZTuple::end() const
	{
	if (fRep)
		return fRep->fProperties.end();
	return sEmptyProperties.end();
	}

bool ZTuple::Empty() const
	{ return !fRep || fRep->fProperties.empty(); }

ZTuple::const_iterator ZTuple::IteratorOf(const ZTuplePropName& iPropName) const
	{
	if (fRep)
		{
		const_iterator end = fRep->fProperties.end();
		for (const_iterator i = fRep->fProperties.begin(); i != end; ++i)
			{
			if ((*i).fName.Equals(iPropName))
				return i;
			}
		return end;
		}
	return sEmptyProperties.end();
	}

ZTuple::const_iterator ZTuple::IteratorOf(const char* iPropName) const
	{
	if (fRep)
		{
		size_t propNameLength = strlen(iPropName);
		const_iterator end = fRep->fProperties.end();
		for (const_iterator i = fRep->fProperties.begin(); i != end; ++i)
			{
			if ((*i).fName.Equals(iPropName, propNameLength))
				return i;
			}
		return end;
		}
	return sEmptyProperties.end();
	}

ZTuple::const_iterator ZTuple::IteratorOf(const std::string& iPropName) const
	{
	if (fRep)
		{
		const_iterator end = fRep->fProperties.end();
		for (const_iterator i = fRep->fProperties.begin(); i != end; ++i)
			{
			if ((*i).fName.Equals(iPropName))
				return i;
			}
		return end;
		}
	return sEmptyProperties.end();
	}

size_t ZTuple::Count() const
	{
	if (fRep)
		return fRep->fProperties.size();
	return 0;
	}

string ZTuple::NameOf(const_iterator iPropIter) const
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		return (*iPropIter).fName.AsString();
	return sNilString;
	}

bool ZTuple::Has(const char* iPropName) const
	{ return this->pLookupAddressConst(iPropName); }

bool ZTuple::Has(const string& iPropName) const
	{ return this->pLookupAddressConst(iPropName); }

bool ZTuple::TypeOf(const_iterator iIterator, ZType& oPropertyType) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iIterator))
		{
		oPropertyType = theValue->TypeOf();
		return true;
		}
	return false;
	}

bool ZTuple::TypeOf(const char* iPropName, ZType& oPropertyType) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		{
		oPropertyType = theValue->TypeOf();
		return true;
		}
	return false;
	}

bool ZTuple::TypeOf(const string& iPropName, ZType& oPropertyType) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		{
		oPropertyType = theValue->TypeOf();
		return true;
		}
	return false;
	}

ZType ZTuple::TypeOf(const_iterator iPropIter) const
	{ return this->pLookupConst(iPropIter).TypeOf(); }

ZType ZTuple::TypeOf(const char* iPropName) const
	{ return this->pLookupConst(iPropName).TypeOf(); }

ZType ZTuple::TypeOf(const string& iPropName) const
	{ return this->pLookupConst(iPropName).TypeOf(); }

void ZTuple::Erase(const_iterator iPropIter)
	{
	if (fRep)
		this->pErase(iPropIter);
	}

void ZTuple::Erase(const char* iPropName)
	{
	if (fRep)
		this->pErase(this->IteratorOf(iPropName));
	}

void ZTuple::Erase(const string& iPropName)
	{
	if (fRep)
		this->pErase(this->IteratorOf(iPropName));
	}

ZTuple::const_iterator ZTuple::EraseAndReturn(const_iterator iPropIter)
	{
	if (!fRep)
		return sEmptyProperties.end();

	if (iPropIter == fRep->fProperties.end())
		return iPropIter;

	if (fRep->GetRefCount() == 1)
		{
		return fRep->fProperties.erase(iPropIter);
		}
	else
		{
		size_t theEraseIndex = iPropIter - fRep->fProperties.begin();

		ZRef<ZTupleRep> oldRep = fRep;
		fRep = new ZTupleRep;

		fRep->fProperties.reserve(oldRep->fProperties.size() - 1);

		copy(oldRep->fProperties.begin(), iPropIter, back_inserter(fRep->fProperties));
		copy(++iPropIter, oldRep->fProperties.end(), back_inserter(fRep->fProperties));

		return fRep->fProperties.begin() + theEraseIndex;
		}
	}

void ZTuple::Clear()
	{
	fRep.Clear();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Get

bool ZTuple::GetValue(const_iterator iPropIter, ZTupleValue& oVal) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropIter))
		{
		oVal = *theValue;
		return true;
		}
	return false;
	}

bool ZTuple::GetValue(const char* iPropName, ZTupleValue& oVal) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		{
		oVal = *theValue;
		return true;
		}
	return false;
	}

bool ZTuple::GetValue(const string& iPropName, ZTupleValue& oVal) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		{
		oVal = *theValue;
		return true;
		}
	return false;
	}

const ZTupleValue& ZTuple::GetValue(const_iterator iPropIter) const
	{ return this->pLookupConst(iPropIter); }

const ZTupleValue& ZTuple::GetValue(const char* iPropName) const
	{ return this->pLookupConst(iPropName); }

const ZTupleValue& ZTuple::GetValue(const string& iPropName) const
	{ return this->pLookupConst(iPropName); }

const ZTupleValue& ZTuple::GetValue(const ZTuplePropName& iPropName) const
	{
	if (fRep)
		{
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			if ((*i).fName.Equals(iPropName))
				return (*i).fTV;
			}
		}
	return sNilValue;
	}

bool ZTuple::GetTuple(const_iterator iPropIter, ZTuple& oVal) const
	{ return this->pLookupConst(iPropIter).GetTuple(oVal); }

const ZTuple& ZTuple::GetTuple(const_iterator iPropIter) const
	{ return this->pLookupConst(iPropIter).GetTuple(); }

bool ZTuple::GetTuple(const char* iPropName, ZTuple& oVal) const
	{ return this->pLookupConst(iPropName).GetTuple(oVal); }

const ZTuple& ZTuple::GetTuple(const char* iPropName) const
	{ return this->pLookupConst(iPropName).GetTuple(); }

bool ZTuple::GetTuple(const string& iPropName, ZTuple& oVal) const
	{ return this->pLookupConst(iPropName).GetTuple(oVal); }

const ZTuple& ZTuple::GetTuple(const string& iPropName) const
	{ return this->pLookupConst(iPropName).GetTuple(); }

bool ZTuple::GetRaw(const_iterator iPropIter, void* iDest, size_t iSize) const
	{ return this->pLookupConst(iPropIter).GetRaw(iDest, iSize); }

bool ZTuple::GetRaw(const char* iPropName, void* iDest, size_t iSize) const
	{ return this->pLookupConst(iPropName).GetRaw(iDest, iSize); }

bool ZTuple::GetRaw(const string& iPropName, void* iDest, size_t iSize) const
	{ return this->pLookupConst(iPropName).GetRaw(iDest, iSize); }

bool ZTuple::GetRawAttributes(const_iterator iPropIter, const void** oAddress, size_t* oSize) const
	{ return this->pLookupConst(iPropIter).GetRawAttributes(oAddress, oSize); }

bool ZTuple::GetRawAttributes(const char* iPropName, const void** oAddress, size_t* oSize) const
	{ return this->pLookupConst(iPropName).GetRawAttributes(oAddress, oSize); }

bool ZTuple::GetRawAttributes(const string& iPropName, const void** oAddress, size_t* oSize) const
	{ return this->pLookupConst(iPropName).GetRawAttributes(oAddress, oSize); }

bool ZTuple::GetVector(const_iterator iPropIter, vector<ZTupleValue>& oVal) const
	{ return this->pLookupConst(iPropIter).GetVector(oVal); }

bool ZTuple::GetVector(const char* iPropName, vector<ZTupleValue>& oVal) const
	{ return this->pLookupConst(iPropName).GetVector(oVal); }

bool ZTuple::GetVector(const string& iPropName, vector<ZTupleValue>& oVal) const
	{ return this->pLookupConst(iPropName).GetVector(oVal); }

const vector<ZTupleValue>& ZTuple::GetVector(const_iterator iPropIter) const
	{ return this->pLookupConst(iPropIter).GetVector(); }

const vector<ZTupleValue>& ZTuple::GetVector(const char* iPropName) const
	{ return this->pLookupConst(iPropName).GetVector(); }

const vector<ZTupleValue>& ZTuple::GetVector(const string& iPropName) const
	{ return this->pLookupConst(iPropName).GetVector(); }

const ZTupleValue ZTuple::DGetValue(const_iterator iPropIter, const ZTupleValue& iDefault) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropIter))
		return *theValue;
	return iDefault;
	}

const ZTupleValue ZTuple::DGetValue(const char* iPropName, const ZTupleValue& iDefault) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return *theValue;
	return iDefault;
	}

const ZTupleValue ZTuple::DGetValue(const std::string& iPropName, const ZTupleValue& iDefault) const
	{
	if (const ZTupleValue* theValue = this->pLookupAddressConst(iPropName))
		return *theValue;
	return iDefault;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Set

bool ZTuple::SetNull(const_iterator iPropIter)
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		{
		iPropIter = this->pTouch(iPropIter);
		(*iPropIter).fTV.SetNull();
		return true;
		}
	return false;
	}

ZTuple& ZTuple::SetNull(const char* iPropName)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetNull();
	return *this;
	}

ZTuple& ZTuple::SetNull(const string& iPropName)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetNull();
	return *this;
	}

bool ZTuple::SetRaw(const_iterator iPropIter, const void* iSource, size_t iSize)
	{ return this->pSet(iPropIter, ZTupleValue(iSource, iSize)); }

ZTuple& ZTuple::SetRaw(const char* iPropName, const void* iSource, size_t iSize)
	{ return this->pSet(iPropName, ZTupleValue(iSource, iSize)); }

ZTuple& ZTuple::SetRaw(const string& iPropName, const void* iSource, size_t iSize)
	{ return this->pSet(iPropName, ZTupleValue(iSource, iSize)); }

bool ZTuple::SetRaw(const_iterator iPropIter, const ZStreamR& iStreamR, size_t iSize)
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		{
		iPropIter = this->pTouch(iPropIter);
		(*iPropIter).fTV.SetRaw(iStreamR, iSize);
		return true;
		}
	return false;
	}

ZTuple& ZTuple::SetRaw(const char* iPropName, const ZStreamR& iStreamR, size_t iSize)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetRaw(iStreamR, iSize);
	return *this;
	}

ZTuple& ZTuple::SetRaw(const string& iPropName, const ZStreamR& iStreamR, size_t iSize)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetRaw(iStreamR, iSize);
	return *this;
	}

bool ZTuple::SetRaw(const_iterator iPropIter, const ZStreamR& iStreamR)
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		{
		iPropIter = this->pTouch(iPropIter);
		(*iPropIter).fTV.SetRaw(iStreamR);
		return true;
		}
	return false;
	}

ZTuple& ZTuple::SetRaw(const char* iPropName, const ZStreamR& iStreamR)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetRaw(iStreamR);
	return *this;
	}

ZTuple& ZTuple::SetRaw(const string& iPropName, const ZStreamR& iStreamR)
	{
	this->pTouch();
	this->pFindOrAllocate(iPropName)->SetRaw(iStreamR);
	return *this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Mutable Get

ZTupleValue& ZTuple::GetMutableValue(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	if (ZTupleValue* theValue = this->pLookupAddress(iPropIter))
		return *theValue;
	ZDebugStopf(0, ("GetMutableValue must only be called on extant properties"));
	return sNilValue;
	}

ZTupleValue& ZTuple::GetMutableValue(const char* iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		return *theValue;
	ZDebugStopf(0, ("GetMutableValue must only be called on extant properties"));
	return sNilValue;
	}

ZTupleValue& ZTuple::GetMutableValue(const string& iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		return *theValue;
	ZDebugStopf(0, ("GetMutableValue must only be called on extant properties"));
	return sNilValue;
	}

ZTuple& ZTuple::GetMutableTuple(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	if (ZTupleValue* theValue = this->pLookupAddress(iPropIter))
		{
		if (theValue->fType.fType == eZType_Tuple)
			return *sFetch_T<ZTuple>(theValue->fType.fBytes);
		}
	ZDebugStopf(0, ("GetMutableTuple must only be called on extant tuple properties"));
	return sNilTuple;
	}

ZTuple& ZTuple::GetMutableTuple(const char* iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		{
		if (theValue->fType.fType == eZType_Tuple)
			return *sFetch_T<ZTuple>(theValue->fType.fBytes);
		}
	ZDebugStopf(0, ("GetMutableTuple must only be called on extant tuple properties"));
	return sNilTuple;
	}

ZTuple& ZTuple::GetMutableTuple(const string& iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		{
		if (theValue->fType.fType == eZType_Tuple)
			return *sFetch_T<ZTuple>(theValue->fType.fBytes);
		}
	ZDebugStopf(0, ("GetMutableTuple must only be called on extant tuple properties"));
	return sNilTuple;
	}

vector<ZTupleValue>& ZTuple::GetMutableVector(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	if (ZTupleValue* theValue = this->pLookupAddress(iPropIter))
		{
		if (theValue->fType.fType == eZType_Vector)
			{
			if (!theValue->fData.fAs_Vector)
				theValue->fData.fAs_Vector = new vector<ZTupleValue>;
			return *theValue->fData.fAs_Vector;
			}
		}
	ZDebugStopf(0, ("GetMutableVector must only be called on extant vector properties"));
	return sNilVector;
	}

vector<ZTupleValue>& ZTuple::GetMutableVector(const char* iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		{
		if (theValue->fType.fType == eZType_Vector)
			{
			if (!theValue->fData.fAs_Vector)
				theValue->fData.fAs_Vector = new vector<ZTupleValue>;
			return *theValue->fData.fAs_Vector;
			}
		}
	ZDebugStopf(0, ("GetMutableVector must only be called on extant vector properties"));
	return sNilVector;
	}

vector<ZTupleValue>& ZTuple::GetMutableVector(const string& iPropName)
	{
	this->pTouch();
	if (ZTupleValue* theValue = this->pLookupAddress(iPropName))
		{
		if (theValue->fType.fType == eZType_Vector)
			{
			if (!theValue->fData.fAs_Vector)
				theValue->fData.fAs_Vector = new vector<ZTupleValue>;
			return *theValue->fData.fAs_Vector;
			}
		}
	ZDebugStopf(0, ("GetMutableVector must only be called on extant vector properties"));
	return sNilVector;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Mutable Set

ZTupleValue& ZTuple::SetMutableNull(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV.SetMutableNull();
	}

ZTupleValue& ZTuple::SetMutableNull(const char* iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableNull();
	}

ZTupleValue& ZTuple::SetMutableNull(const string& iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableNull();
	}

ZTuple& ZTuple::SetMutableTuple(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV.SetMutableTuple();
	}

ZTuple& ZTuple::SetMutableTuple(const char* iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableTuple();
	}

ZTuple& ZTuple::SetMutableTuple(const string& iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableTuple();
	}

vector<ZTupleValue>& ZTuple::SetMutableVector(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV.SetMutableVector();
	}

vector<ZTupleValue>& ZTuple::SetMutableVector(const char* iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableVector();
	}

vector<ZTupleValue>& ZTuple::SetMutableVector(const string& iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->SetMutableVector();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple ensure a property is of the type, and return a mutable reference

ZTupleValue& ZTuple::EnsureMutableValue(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV;
	}

ZTupleValue& ZTuple::EnsureMutableValue(const char* iPropName)
	{
	this->pTouch();
	return *this->pFindOrAllocate(iPropName);
	}

ZTupleValue& ZTuple::EnsureMutableValue(const std::string& iPropName)
	{
	this->pTouch();
	return *this->pFindOrAllocate(iPropName);
	}

ZTuple& ZTuple::EnsureMutableTuple(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV.EnsureMutableTuple();
	}

ZTuple& ZTuple::EnsureMutableTuple(const char* iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->EnsureMutableTuple();
	}

ZTuple& ZTuple::EnsureMutableTuple(const string& iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->EnsureMutableTuple();
	}

vector<ZTupleValue>& ZTuple::EnsureMutableVector(const_iterator iPropIter)
	{
	iPropIter = this->pTouch(iPropIter);
	return (*iPropIter).fTV.EnsureMutableVector();
	}

vector<ZTupleValue>& ZTuple::EnsureMutableVector(const char * iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->EnsureMutableVector();
	}

vector<ZTupleValue>& ZTuple::EnsureMutableVector(const string& iPropName)
	{
	this->pTouch();
	return this->pFindOrAllocate(iPropName)->EnsureMutableVector();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple minimize

// Ensure that our vector's capacity is just enough to hold our data.
ZTuple& ZTuple::Minimize()
	{
	if (fRep)
		fRep = new ZTupleRep(fRep.GetObject());
	return *this;
	}

ZTuple ZTuple::Minimized() const
	{ return ZTuple(*this).Minimize(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Internals

ZTupleValue* ZTuple::pFindOrAllocate(const char* iPropName)
	{
	ZAssertStop(kDebug_Tuple, fRep);

	size_t propNameLength = strlen(iPropName);
	for (PropList::iterator i = fRep->fProperties.begin(),
		theEnd = fRep->fProperties.end();
		i != theEnd; ++i)
		{
		if ((*i).fName.Equals(iPropName, propNameLength))
			return &(*i).fTV;
		}
	fRep->fProperties.push_back(NameTV(iPropName));
	return &fRep->fProperties.back().fTV;
	}

ZTupleValue* ZTuple::pFindOrAllocate(const std::string& iPropName)
	{
	ZAssertStop(kDebug_Tuple, fRep);

	for (PropList::iterator i = fRep->fProperties.begin(),
		theEnd = fRep->fProperties.end();
		i != theEnd; ++i)
		{
		if ((*i).fName.Equals(iPropName))
			return &(*i).fTV;
		}
	fRep->fProperties.push_back(NameTV(iPropName));
	return &fRep->fProperties.back().fTV;
	}

bool ZTuple::pSet(const_iterator iPropIter, const ZTupleValue& iVal)
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		{
		iPropIter = this->pTouch(iPropIter);
		(*iPropIter).fTV = iVal;
		return true;
		}
	return false;
	}

ZTuple& ZTuple::pSet(const char* iPropName, const ZTupleValue& iVal)
	{
	this->pTouch();
	*pFindOrAllocate(iPropName) = iVal;
	return *this;
	}

ZTuple& ZTuple::pSet(const string& iPropName, const ZTupleValue& iVal)
	{
	this->pTouch();
	*pFindOrAllocate(iPropName) = iVal;
	return *this;
	}

ZTuple& ZTuple::pAppend(const char* iPropName, const ZTupleValue& iVal)
	{
	this->pTouch();
	pFindOrAllocate(iPropName)->AppendValue(iVal);
	return *this;
	}

ZTuple& ZTuple::pAppend(const string& iPropName, const ZTupleValue& iVal)
	{
	this->pTouch();
	pFindOrAllocate(iPropName)->AppendValue(iVal);
	return *this;
	}

const ZTupleValue* ZTuple::pLookupAddressConst(const_iterator iPropIter) const
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		return &(*iPropIter).fTV;
	return nil;
	}

const ZTupleValue* ZTuple::pLookupAddressConst(const char* iPropName) const
	{
	if (fRep)
		{
		size_t propNameLength = strlen(iPropName);
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			if ((*i).fName.Equals(iPropName, propNameLength))
				return &(*i).fTV;
			}
		}
	return nil;
	}

const ZTupleValue* ZTuple::pLookupAddressConst(const string& iPropName) const
	{
	if (fRep)
		{
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			if ((*i).fName.Equals(iPropName))
				return &(*i).fTV;
			}
		}
	return nil;
	}

ZTupleValue* ZTuple::pLookupAddress(const_iterator iPropIter)
	{
	// We're the non-const version, being called from a mutating operation
	// so pTouch must have been called and we must have a rep.
	ZAssertStop(kDebug_Tuple, fRep);

	if (fRep && iPropIter != fRep->fProperties.end())
		return &(*iPropIter).fTV;
	return nil;
	}

ZTupleValue* ZTuple::pLookupAddress(const char* iPropName)
	{
	ZAssertStop(kDebug_Tuple, fRep);

	size_t propNameLength = strlen(iPropName);
	for (PropList::iterator i = fRep->fProperties.begin(),
		theEnd = fRep->fProperties.end();
		i != theEnd; ++i)
		{
		if ((*i).fName.Equals(iPropName, propNameLength))
			return &(*i).fTV;
		}
	return nil;
	}

ZTupleValue* ZTuple::pLookupAddress(const string& iPropName)
	{
	ZAssertStop(kDebug_Tuple, fRep);

	for (PropList::iterator i = fRep->fProperties.begin(),
		theEnd = fRep->fProperties.end();
		i != theEnd; ++i)
		{
		if ((*i).fName.Equals(iPropName))
			return &(*i).fTV;
		}
	return nil;
	}

const ZTupleValue& ZTuple::pLookupConst(const_iterator iPropIter) const
	{
	if (fRep && iPropIter != fRep->fProperties.end())
		return (*iPropIter).fTV;
	return sNilValue;
	}

const ZTupleValue& ZTuple::pLookupConst(const char* iPropName) const
	{
	if (fRep)
		{
		size_t propNameLength = strlen(iPropName);
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			if ((*i).fName.Equals(iPropName, propNameLength))
				return (*i).fTV;
			}
		}
	return sNilValue;
	}

const ZTupleValue& ZTuple::pLookupConst(const string& iPropName) const
	{
	if (fRep)
		{
		for (PropList::const_iterator i = fRep->fProperties.begin(),
			theEnd = fRep->fProperties.end();
			i != theEnd; ++i)
			{
			if ((*i).fName.Equals(iPropName))
				return (*i).fTV;
			}
		}
	return sNilValue;
	}

void ZTuple::pErase(const_iterator iPropIter)
	{
	if (iPropIter == fRep->fProperties.end())
		return;

	if (fRep->GetRefCount() == 1)
		{
		fRep->fProperties.erase(iPropIter);
		}
	else
		{
		ZRef<ZTupleRep> oldRep = fRep;
		fRep = new ZTupleRep;

		fRep->fProperties.reserve(oldRep->fProperties.size() - 1);
		copy(oldRep->fProperties.begin(), iPropIter, back_inserter(fRep->fProperties));
		copy(++iPropIter, oldRep->fProperties.end(), back_inserter(fRep->fProperties));
		}
	}

void ZTuple::pTouch()
	{
	if (fRep)
		{
		if (fRep->GetRefCount() > 1)
			fRep = new ZTupleRep(fRep.GetObject());
		}
	else
		{
		fRep = new ZTupleRep;
		}
	}

ZTuple::const_iterator ZTuple::pTouch(const_iterator iPropIter)
	{
	ZAssertStop(kDebug_Tuple, fRep);

	if (fRep->GetRefCount() == 1)
		return iPropIter;

	size_t index = iPropIter - fRep->fProperties.begin();
	fRep = new ZTupleRep(fRep.GetObject());
	return fRep->fProperties.begin() + index;
	}

bool ZTuple::pIsEqual(const ZTuple& iOther) const
	{
	// The inline implementation of operator==, which is our only caller,
	// has already determined that fRep != iOther.fRep.
	ZAssertStop(kDebug_Tuple, fRep != iOther.fRep);

	if (!fRep || !iOther.fRep)
		return false;

	if (fRep->fProperties.size() != iOther.fRep->fProperties.size())
		return false;

	for (PropList::iterator iterThis = fRep->fProperties.begin(),
		iterOther = iOther.fRep->fProperties.begin(),
		endThis = fRep->fProperties.end();
		iterThis != endThis;
		++iterThis, ++iterOther)
		{
		if (!(*iterThis).fName.Equals((*iterOther).fName))
			return false;
		if ((*iterThis).fTV != (*iterOther).fTV)
			return false;
		}

	return true;
	}

bool ZTuple::pIsSameAs(const ZTuple& iOther) const
	{
	// The inline implementation of IsSameAs, which is our only caller,
	// has already determined that fRep != iOther.fRep.
	ZAssertStop(kDebug_Tuple, fRep != iOther.fRep);

	if (!fRep || !iOther.fRep)
		return false;

	if (fRep->fProperties.size() != iOther.fRep->fProperties.size())
		return false;

	for (PropList::iterator iterThis = fRep->fProperties.begin(),
		endThis = fRep->fProperties.end();
		iterThis != endThis; ++iterThis)
		{
		const_iterator iterOther = iOther.IteratorOf((*iterThis).fName);
		if (iterOther == iOther.end())
			return false;
		if (!(*iterThis).fTV.IsSameAs((*iterOther).fTV))
			return false;
		}

	return true;
	}

ZRef<ZTupleRep> ZTuple::sRepFromStream(const ZStreamR& iStreamR)
	{
	ZRef<ZTupleRep> theRep;
	if (uint32 propertyCount = sReadCount(iStreamR))
		{
		theRep = new ZTupleRep;
		PropList& properties = theRep->fProperties;

		// It's a vector
		// Note that we set the rep's vector's size here, then iterate over it to
		// read in the data in place.
		properties.reserve(propertyCount);
		properties.resize(propertyCount);
		for (PropList::iterator i = properties.begin(); i != properties.end(); ++i)
			{
			(*i).fName.FromStream(iStreamR);
			(*i).fTV.FromStream(iStreamR);
			}
		}
	return theRep;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleRep

ZTupleRep::ZTupleRep()
	{}

ZTupleRep::ZTupleRep(const ZTupleRep* iOther)
:	fProperties(iOther->fProperties)
	{}

ZTupleRep::~ZTupleRep()
	{}

#ifndef SKIPOMPARSE
// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Get

#define ZTupleGetByIter(TYPENAME, TYPE_O, TYPE_RET) \
bool ZTuple::Get##TYPENAME(const_iterator iPropIter, TYPE_O& oVal) const \
	{ return this->pLookupConst(iPropIter).Get##TYPENAME(oVal); } \
TYPE_RET ZTuple::Get##TYPENAME(const_iterator iPropIter) const \
	{ return this->pLookupConst(iPropIter).Get##TYPENAME(); }

#define ZTupleGetByName(TYPENAME, STRINGTYPE, TYPE_O, TYPE_RET) \
bool ZTuple::Get##TYPENAME(STRINGTYPE iPropName, TYPE_O& oVal) const \
	{ return this->pLookupConst(iPropName).Get##TYPENAME(oVal); } \
TYPE_RET ZTuple::Get##TYPENAME(STRINGTYPE iPropName) const \
	{ return this->pLookupConst(iPropName).Get##TYPENAME(); } \

#define ZTupleGet(TYPENAME, TYPE) \
	ZTupleGetByIter(TYPENAME, TYPE, TYPE) \
	ZTupleGetByName(TYPENAME, const char*, TYPE, TYPE) \
	ZTupleGetByName(TYPENAME, const string&, TYPE, TYPE)

#define ZTupleGetRet(TYPENAME, TYPE_O, TYPE_RET) \
	ZTupleGetByIter(TYPENAME, TYPE_O, TYPE_RET) \
	ZTupleGetByName(TYPENAME, const char*, TYPE_O, TYPE_RET) \
	ZTupleGetByName(TYPENAME, const string&, TYPE_O, TYPE_RET)

ZTupleGet(Type, ZType)
ZTupleGet(ID, uint64)
ZTupleGet(Int8, int8)
ZTupleGet(Int16, int16)
ZTupleGet(Int32, int32)
ZTupleGet(Int64, int64)
ZTupleGet(Bool, bool)
ZTupleGet(Float, float)
ZTupleGet(Double, double)
ZTupleGet(Time, ZTime)
ZTupleGet(Pointer, void*)
ZTupleGetRet(Rect, ZRectPOD, const ZRectPOD&)
ZTupleGetRet(Point, ZPointPOD, const ZPointPOD&)
ZTupleGet(String, string)
ZTupleGet(RefCounted, ZRef<ZRefCountedWithFinalization>)
ZTupleGet(Raw, ZMemoryBlock)

#undef ZTupleGet
#undef ZTupleGetRet
#undef ZTupleGetByIter
#undef ZTupleGetByName

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple DGet

#define ZTupleDGetByIter(TYPENAME, TYPE_I, TYPE_RET) \
TYPE_RET ZTuple::DGet##TYPENAME(const_iterator iPropIter, TYPE_I iDefault) const \
	{ return this->pLookupConst(iPropIter).DGet##TYPENAME(iDefault); }

#define ZTupleDGetByName(TYPENAME, STRINGTYPE, TYPE_I, TYPE_RET) \
TYPE_RET ZTuple::DGet##TYPENAME(STRINGTYPE iPropName, TYPE_I iDefault) const \
	{ return this->pLookupConst(iPropName).DGet##TYPENAME(iDefault); } \

#define ZTupleDGet(TYPENAME, TYPE) \
	ZTupleDGetByIter(TYPENAME, TYPE, TYPE) \
	ZTupleDGetByName(TYPENAME, const char*, TYPE, TYPE) \
	ZTupleDGetByName(TYPENAME, const string&, TYPE, TYPE)

#define ZTupleDGetRet(TYPENAME, TYPE_I, TYPE_RET) \
	ZTupleDGetByIter(TYPENAME, TYPE_I, TYPE_RET) \
	ZTupleDGetByName(TYPENAME, const char*, TYPE_I, TYPE_RET) \
	ZTupleDGetByName(TYPENAME, const string&, TYPE_I, TYPE_RET)

ZTupleDGet(Type, ZType)
ZTupleDGet(ID, uint64)
ZTupleDGet(Int8, int8)
ZTupleDGet(Int16, int16)
ZTupleDGet(Int32, int32)
ZTupleDGet(Int64, int64)
ZTupleDGet(Bool, bool)
ZTupleDGet(Float, float)
ZTupleDGet(Double, double)
ZTupleDGet(Time, ZTime)
ZTupleDGetRet(Rect, const ZRectPOD&, ZRectPOD)
ZTupleDGetRet(Point, const ZPointPOD&, ZPointPOD)
ZTupleDGetRet(String, const string&, string)

#undef ZTupleDGet
#undef ZTupleDGetByIter
#undef ZTupleDGetByName

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Set

#define ZTupleSetByIter(TYPENAME, PARAMTYPE, STOREDVALUE) \
bool ZTuple::Set##TYPENAME(const_iterator iPropIter, PARAMTYPE iVal) \
	{ return this->pSet(iPropIter, STOREDVALUE); }

#define ZTupleSetByName(TYPENAME, STRINGTYPE, PARAMTYPE, STOREDVALUE) \
ZTuple& ZTuple::Set##TYPENAME(STRINGTYPE iPropName, PARAMTYPE iVal) \
	{ return this->pSet(iPropName, STOREDVALUE); }

#define ZTupleSet(TYPENAME, PARAMTYPE) \
	ZTupleSetByIter(TYPENAME, PARAMTYPE, ZTupleValue(iVal)) \
	ZTupleSetByName(TYPENAME, const char*, PARAMTYPE, ZTupleValue(iVal)) \
	ZTupleSetByName(TYPENAME, const string&, PARAMTYPE, ZTupleValue(iVal))

ZTupleSetByIter(Value, const ZTupleValue&, iVal)
ZTupleSetByName(Value, const char*, const ZTupleValue&, iVal)
ZTupleSetByName(Value, const string&, const ZTupleValue&, iVal)

ZTupleSetByIter(ID, uint64, ZTupleValue(iVal, true))
ZTupleSetByName(ID, const char*, uint64, ZTupleValue(iVal, true))
ZTupleSetByName(ID, const string&, uint64, ZTupleValue(iVal, true))

ZTupleSetByIter(Type, ZType, ZTupleValue(iVal))
ZTupleSetByName(Type, const char*, ZType, ZTupleValue(iVal))
ZTupleSetByName(Type, const string&, ZType, ZTupleValue(iVal))

ZTupleSet(Int8, int8)
ZTupleSet(Int16, int16)
ZTupleSet(Int32, int32)
ZTupleSet(Int64, int64)
ZTupleSet(Bool, bool)
ZTupleSet(Float, float)
ZTupleSet(Double, double)
ZTupleSet(Time, ZTime)
ZTupleSet(Pointer, void*)
ZTupleSet(Rect, const ZRectPOD&)
ZTupleSet(Point, const ZPointPOD&)
ZTupleSet(String, const char*)
ZTupleSet(String, const string&)
ZTupleSet(Tuple, const ZTuple&)
ZTupleSet(RefCounted, const ZRef<ZRefCountedWithFinalization>&)
ZTupleSet(Raw, const ZMemoryBlock&)
ZTupleSet(Vector, const vector<ZTupleValue>&)

#undef ZTupleSetByIter
#undef ZTupleSetByName
#undef ZTupleSet

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple Append

void ZTuple::AppendRaw(const char* iPropName, const void* iSource, size_t iSize)
	{ this->pAppend(iPropName, ZTupleValue(iSource, iSize)); }

void ZTuple::AppendRaw(const string& iPropName, const void* iSource, size_t iSize)
	{ this->pAppend(iPropName, ZTupleValue(iSource, iSize)); }

#define ZTupleAppendByName(TYPENAME, STRINGTYPE, PARAMTYPE, STOREDVALUE) \
void ZTuple::Append##TYPENAME(STRINGTYPE iPropName, PARAMTYPE iVal) \
	{ this->pAppend(iPropName, STOREDVALUE); }

#define ZTupleAppend(TYPENAME, PARAMTYPE) \
	ZTupleAppendByName(TYPENAME, const char*, PARAMTYPE, ZTupleValue(iVal)) \
	ZTupleAppendByName(TYPENAME, const string&, PARAMTYPE, ZTupleValue(iVal))

ZTupleAppendByName(Value, const char*, const ZTupleValue&, iVal)
ZTupleAppendByName(Value, const string&, const ZTupleValue&, iVal)

ZTupleAppendByName(ID, const char*, uint64, ZTupleValue(iVal, true))
ZTupleAppendByName(ID, const string&, uint64, ZTupleValue(iVal, true))

ZTupleAppendByName(Type, const char*, ZType, ZTupleValue(iVal))
ZTupleAppendByName(Type, const string&, ZType, ZTupleValue(iVal))

ZTupleAppend(Int8, int8)
ZTupleAppend(Int16, int16)
ZTupleAppend(Int32, int32)
ZTupleAppend(Int64, int64)
ZTupleAppend(Bool, bool)
ZTupleAppend(Float, float)
ZTupleAppend(Double, double)
ZTupleAppend(Time, ZTime)
ZTupleAppend(Pointer, void*)
ZTupleAppend(Rect, const ZRectPOD&)
ZTupleAppend(Point, const ZPointPOD&)
ZTupleAppend(String, const char*)
ZTupleAppend(String, const string&)
ZTupleAppend(Tuple, const ZTuple&)
ZTupleAppend(RefCounted, const ZRef<ZRefCountedWithFinalization>&)
ZTupleAppend(Raw, const ZMemoryBlock&)

#undef ZTupleAppend
#undef ZTupleAppendByName

#endif // SKIPOMPARSE
