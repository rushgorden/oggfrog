/* @(#) $Id: ZTuple.h,v 1.48 2006/11/17 21:03:34 agreen Exp $ */

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

#ifndef __ZTuple__
#define __ZTuple__ 1
#include "zconfig.h"

#include "ZCompare.h"
#include "ZMemory.h" // For ZBlockCopy
#include "ZRefCount.h"
#include "ZTime.h"
#include "ZTypes.h"

#include <string>
#include <vector>

#ifndef ZCONFIG_Tuple_PackedTVs
#	define ZCONFIG_Tuple_PackedTVs 1
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Forward and external declarations

class ZTuple;
class ZTupleRep;
class ZTupleValue;

class ZMemoryBlock;

class ZStreamR;
class ZStreamW;

// =================================================================================================
#pragma mark -
#pragma mark * ZTuplePropName

class ZTuplePropName
	{
	// The pointer references a PNRep if the low bit is set.
	static bool sIsPNRep(const void* iData);

public:
	/// Call this to pre-register property names that will be shareable across all tuples.
	static int sPreRegister(const char* const* iNames, size_t iCount);

	ZTuplePropName();

	~ZTuplePropName();

	ZTuplePropName(const ZTuplePropName& iOther);

	ZTuplePropName& operator=(const ZTuplePropName& iOther);

	explicit ZTuplePropName(const ZStreamR& iStreamR);

	ZTuplePropName(const char* iString, size_t iLength);
	ZTuplePropName(const std::string& iString);
	ZTuplePropName(const char* iString);

	bool operator==(const ZTuplePropName& iOther) const;
	bool operator!=(const ZTuplePropName& iOther) const;

	bool operator<(const ZTuplePropName& iOther) const;
	bool operator<=(const ZTuplePropName& iOther) const;
	bool operator>(const ZTuplePropName& iOther) const;
	bool operator>=(const ZTuplePropName& iOther) const;

	int Compare(const ZTuplePropName& iOther) const;

	bool Equals(const ZTuplePropName& iOther) const;
	bool Equals(const std::string& iString) const;
	bool Equals(const char* iString, size_t iLength) const;

	void ToStream(const ZStreamW& iStreamW) const;
	void FromStream(const ZStreamR& iStreamR);

	bool Empty() const;

	std::string AsString() const;

private:
	class String
		{
	private:
		String(); // Not implemented
		String& operator=(const String&); // Not implemented

	public:
		void* operator new(size_t iObjectSize, size_t iDataSize)
			{ return new char[iObjectSize + iDataSize]; }

		void operator delete(void* iPtr, size_t iObjectSize)
			{ delete[] static_cast<char*>(iPtr); }

		String(const String& iOther)
		:	fSize(iOther.fSize)
			{ ZBlockCopy(iOther.fBuffer, fBuffer, fSize); }

		String(const char* iString, size_t iSize);		

		int Compare(const String& iOther) const;
		int Compare(const char* iString, size_t iSize) const;
		int Compare(const std::string& iString) const;

		void ToStream(const ZStreamW& iStreamW) const;

		std::string AsString() const;

		const size_t fSize;
		char fBuffer[0];
		};

	union
		{
		const void* fData;
		const String* fString;
		};
	};

inline bool ZTuplePropName::sIsPNRep(const void* iData)
	{ return reinterpret_cast<intptr_t>(iData) & 1; }

inline ZTuplePropName::ZTuplePropName()
:	fData(reinterpret_cast<void*>(1))
	{}

inline ZTuplePropName::~ZTuplePropName()
	{
	if (!sIsPNRep(fData))
		delete fString;
	}

inline ZTuplePropName::ZTuplePropName(const ZTuplePropName& iOther)
:	fData(iOther.fData)
	{
	if (!sIsPNRep(fData))
		fString = new (fString->fSize) String(*fString);
	}

inline bool ZTuplePropName::operator==(const ZTuplePropName& iOther) const
	{ return 0 == this->Compare(iOther); }
	
inline bool ZTuplePropName::operator!=(const ZTuplePropName& iOther) const
	{ return !(*this == iOther); }

inline bool ZTuplePropName::operator<(const ZTuplePropName& iOther) const
	{ return 0 > this->Compare(iOther); }

inline bool ZTuplePropName::operator<=(const ZTuplePropName& iOther) const
	{ return !(iOther < *this); }

inline bool ZTuplePropName::operator>(const ZTuplePropName& iOther) const
	{ return iOther < *this; }

inline bool ZTuplePropName::operator>=(const ZTuplePropName& iOther) const
	{ return !(*this < iOther); }

inline bool ZTuplePropName::Equals(const ZTuplePropName& iOther) const
	{ return 0 == this->Compare(iOther); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleValue

/// Holds a value which can be one of several different types.

#if ZCONFIG_Tuple_PackedTVs
#	pragma pack(4)
#endif

class ZTupleValue
	{
	ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZTupleValue,
		operator_bool_generator_type, operator_bool_type);
public:
	class Ex_IllegalType;

/** \name Canonical Methods
*/	//@{
	ZTupleValue();
	~ZTupleValue();
	ZTupleValue(const ZTupleValue& iOther);
	ZTupleValue& operator=(const ZTupleValue& iOther);
	//@}

/** \name Constructors taking various types
*/	//@{
	ZTupleValue(const ZTupleValue& iVal, bool iAsVector);
	ZTupleValue(ZType iVal);
	ZTupleValue(uint64 iVal, bool iIsID);
	ZTupleValue(uint64 iVal);
	ZTupleValue(int8 iVal);
	ZTupleValue(int16 iVal);
	ZTupleValue(int32 iVal);
	ZTupleValue(int64 iVal);
	ZTupleValue(bool iVal);
	ZTupleValue(float iVal);
	ZTupleValue(double iVal);
	ZTupleValue(ZTime iVal);
	ZTupleValue(void* iVal);
	ZTupleValue(const ZRectPOD& iVal);
	ZTupleValue(const ZPointPOD& iVal);
	ZTupleValue(const char* iVal);
	ZTupleValue(const std::string& iVal);
	ZTupleValue(const ZTuple& iVal);
	ZTupleValue(const ZRef<ZRefCountedWithFinalization>& iVal);
	ZTupleValue(const ZMemoryBlock& iVal);
	ZTupleValue(const void* iSource, size_t iSize);
	ZTupleValue(const ZStreamR& iStreamR, size_t iSize);
	ZTupleValue(const std::vector<ZTupleValue>& iVal);
	//@}

/** \name Constructing from flattened data in a stream
*/	//@{
	explicit ZTupleValue(const ZStreamR& iStreamR);
	//@}

/** \name Efficient swapping with another instance
*/	//@{
	void swap(ZTupleValue& iOther);
	//@}

/** \name Writing to and reading from a stream
*/	//@{
	void ToStream(const ZStreamW& iStreamW) const;
	void FromStream(const ZStreamR& iStreamR);

	void ToStreamOld(const ZStreamW& iStreamW) const;
	void FromStreamOld(const ZStreamR& iStreamR);
	//@}

/** \name Comparison
*/	//@{
	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(fType.fType != eZType_Null); }

	int Compare(const ZTupleValue& iOther) const;

	bool operator==(const ZTupleValue& iOther) const;
	bool operator!=(const ZTupleValue& iOther) const;

	bool operator<(const ZTupleValue& iOther) const;
	bool operator<=(const ZTupleValue& iOther) const;
	bool operator>(const ZTupleValue& iOther) const;
	bool operator>=(const ZTupleValue& iOther) const;

	bool IsSameAs(const ZTupleValue& iOther) const;

	bool IsString(const char* iString) const;
	bool IsString(const std::string& iString) const;
	//@}


/** \name Getting the type of the value
*/	//@{
	ZType TypeOf() const { return fType.fType < 0 ? eZType_String : ZType(fType.fType); }
	//@}

/** \name Getting the real value
*/	//@{
	bool GetType(ZType& oVal) const;
	ZType GetType() const;

	bool GetID(uint64& oVal) const;
	uint64 GetID() const;

	bool GetInt8(int8& oVal) const;
	int8 GetInt8() const;

	bool GetInt16(int16& oVal) const;
	int16 GetInt16() const;

	bool GetInt32(int32& oVal) const;
	int32 GetInt32() const;

	bool GetInt64(int64& oVal) const;
	int64 GetInt64() const;

	bool GetBool(bool& oVal) const;
	bool GetBool() const;

	bool GetFloat(float& oVal) const;
	float GetFloat() const;

	bool GetDouble(double& oVal) const;
	double GetDouble() const;

	bool GetTime(ZTime& oVal) const;
	ZTime GetTime() const;

	bool GetPointer(void*& oVal) const;
	void* GetPointer() const;

	bool GetRect(ZRectPOD& oVal) const;
	const ZRectPOD& GetRect() const;

	bool GetPoint(ZPointPOD& oVal) const;
	const ZPointPOD& GetPoint() const;

	bool GetString(std::string& oVal) const;
	std::string GetString() const;

	bool GetTuple(ZTuple& oVal) const;
	const ZTuple& GetTuple() const;

	bool GetRefCounted(ZRef<ZRefCountedWithFinalization>& oVal) const;
	ZRef<ZRefCountedWithFinalization> GetRefCounted() const;

	bool GetRaw(ZMemoryBlock& oVal) const;
	ZMemoryBlock GetRaw() const;

	bool GetRaw(void* iDest, size_t iSize) const;

	bool GetRawAttributes(const void** oAddress, size_t* oSize) const;

	bool GetVector(std::vector<ZTupleValue>& oVal) const;
	const std::vector<ZTupleValue>& GetVector() const;
	//@}

/** \name Getting, with a default value
*/	//@{
	ZType DGetType(ZType iDefault) const;

	uint64 DGetID(uint64 iDefault) const;

	int8 DGetInt8(int8 iDefault) const;

	int16 DGetInt16(int16 iDefault) const;

	int32 DGetInt32(int32 iDefault) const;

	int64 DGetInt64(int64 iDefault) const;

	bool DGetBool(bool iDefault) const;

	float DGetFloat(float iDefault) const;

	double DGetDouble(double iDefault) const;

	ZTime DGetTime(ZTime iDefault) const;

	ZRectPOD DGetRect(const ZRectPOD& iDefault) const;

	ZPointPOD DGetPoint(const ZPointPOD& iDefault) const;

	std::string DGetString(const std::string& iDefault) const;
	//@}

/** \name Setting type and value
*/	//@{
	void SetNull();
	void SetType(ZType iVal);
	void SetID(uint64 iVal);
	void SetInt8(int8 iVal);
	void SetInt16(int16 iVal);
	void SetInt32(int32 iVal);
	void SetInt64(int64 iVal);
	void SetBool(bool iVal);
	void SetFloat(float iVal);
	void SetDouble(double iVal);
	void SetTime(ZTime iVal);
	void SetPointer(void* iVal);
	void SetRect(const ZRectPOD& iVal);
	void SetPoint(const ZPointPOD& iVal);
	void SetString(const char* iVal);
	void SetString(const std::string& iVal);
	void SetTuple(const ZTuple& iVal);
	void SetRefCounted(const ZRef<ZRefCountedWithFinalization>& iVal);
	void SetRaw(const ZMemoryBlock& iVal);
	void SetRaw(const void* iSource, size_t iSize);
	void SetRaw(const ZStreamR& iStreamR, size_t iSize);
	void SetRaw(const ZStreamR& iStreamR);
	void SetVector(const std::vector<ZTupleValue>& iVal);
	//@}

/** \name Appending a value
*/	//@{
	void AppendValue(const ZTupleValue& iVal);
	//@}

/** \name Getting mutable reference to actual value, must be of correct type
*/	//@{
	ZTuple& GetMutableTuple();
	std::vector<ZTupleValue>& GetMutableVector();
	//@}

/** \name Setting empty value and returning a mutable reference to it
*/	//@{
	ZTupleValue& SetMutableNull();
	ZTuple& SetMutableTuple();
	std::vector<ZTupleValue>& SetMutableVector();
	//@}

/** \name Getting mutable reference to actual value, changing type if necessary
*/	//@{
	ZTuple& EnsureMutableTuple();
	std::vector<ZTupleValue>& EnsureMutableVector();
	//@}

/** \name Template member functions
*/	//@{
	template <typename T> bool Get_T(T& oVal) const;

	template <typename T> T Get_T() const;

	template <typename T> T DGet_T(T iDefault) const;

	template <typename OutputIterator>
	void GetVector_T(OutputIterator iIter) const;

	template <typename OutputIterator, typename T>
	void GetVector_T(OutputIterator iIter, const T& iDummy) const;

	template <typename InputIterator>
	void SetVector_T(InputIterator iBegin, InputIterator iEnd);

	template <typename InputIterator, typename T>
	void SetVector_T(InputIterator iBegin, InputIterator iEnd, const T& iDummy);
	//@}

private:
	int pUncheckedCompare(const ZTupleValue& iOther) const;
	bool pUncheckedLess(const ZTupleValue& iOther) const;
	bool pUncheckedEqual(const ZTupleValue& iOther) const;

	void pRelease();
	void pCopy(const ZTupleValue& iOther);
	void pFromStream(const ZStreamR& iStream);

	friend class ZTuple;
	
	// Data is stored in one of several ways.
	// * For POD data <= 8 bytes in length, we simply store the
	// data directly, using named fields of fData.
	// * For ZRectPOD and vector<ZTupleValue> in fData we store a
	// pointer to a heap-allocated object, because they're 16 bytes
	// and probably 12 bytes in size, respectively.
	// * For ZTuple, ZRef and ZMemoryBlock we use placement-new and
	// explicit destructor invocation on fType.fBytes -- they're
	// all 4 bytes in size, but have constructors/destructors, so
	// we can't declare them in the union.

	// Finally, strings are funky. For short (<= 11 bytes) string
	// instances, we put the characters in fType.fBytes, and
	// store an encoded length in fType.fType.
	// If they're longer, we do placement new of a ZTupleString.

	static const int kBytesSize = 11;

	union Data
		{
		ZType fAs_Type;
		uint64 fAs_ID;
		int8 fAs_Int8;
		int16 fAs_Int16;
		int32 fAs_Int32;
		int64 fAs_Int64;
		bool fAs_Bool;
		float fAs_Float;
		double fAs_Double;
		double fAs_Time;
		void* fAs_Pointer;
		ZPointPOD fAs_Point;

		ZRectPOD* fAs_Rect;
		vector<ZTupleValue>* fAs_Vector;
		};

	struct Type
		{
		char fBytes[kBytesSize];
		int8 fType;
		};

	struct FastCopy
		{
		char dummy[kBytesSize + 1];
		};

	union
		{
		Data fData;
		Type fType;
		FastCopy fFastCopy;
		};
	};

#if ZCONFIG_Tuple_PackedTVs
#	pragma pack()
#endif


inline void ZTupleValue::swap(ZTupleValue& iOther)
	{ std::swap(fFastCopy, iOther.fFastCopy); }

namespace std {
inline void swap(ZTupleValue& a, ZTupleValue& b)
	{ a.swap(b); }
} // namespace std


inline bool ZTupleValue::operator!=(const ZTupleValue& iOther) const
	{ return !(*this == iOther); }

inline bool ZTupleValue::operator<=(const ZTupleValue& iOther) const
	{ return !(iOther < *this); }

inline bool ZTupleValue::operator>(const ZTupleValue& iOther) const
	{ return iOther < *this; }

inline bool ZTupleValue::operator>=(const ZTupleValue& iOther) const
	{ return !(*this < iOther); }

/** \name Template member functions
*/	//@{
template <> inline bool ZTupleValue::Get_T<ZTupleValue>(ZTupleValue& oVal) const
	{ oVal = *this; return true; }

template <> inline bool ZTupleValue::Get_T<ZType>(ZType& oVal) const
	{ return this->GetType(oVal); }

template <> inline bool ZTupleValue::Get_T<uint64>(uint64& oVal) const
	{ return this->GetID(oVal); }

template <> inline bool ZTupleValue::Get_T<int8>(int8& oVal) const
	{ return this->GetInt8(oVal); }

template <> inline bool ZTupleValue::Get_T<int16>(int16& oVal) const
	{ return this->GetInt16(oVal); }

template <> inline bool ZTupleValue::Get_T<int32>(int32& oVal) const
	{ return this->GetInt32(oVal); }

template <> inline bool ZTupleValue::Get_T<int64>(int64& oVal) const
	{ return this->GetInt64(oVal); }

template <> inline bool ZTupleValue::Get_T<bool>(bool& oVal) const
	{ return this->GetBool(oVal); }

template <> inline bool ZTupleValue::Get_T<float>(float& oVal) const
	{ return this->GetFloat(oVal); }

template <> inline bool ZTupleValue::Get_T<double>(double& oVal) const
	{ return this->GetDouble(oVal); }

template <> inline bool ZTupleValue::Get_T<ZTime>(ZTime& oVal) const
	{ return this->GetTime(oVal); }

template <> inline bool ZTupleValue::Get_T<std::string>(std::string& oVal) const
	{ return this->GetString(oVal); }


template <> inline ZTupleValue ZTupleValue::Get_T<ZTupleValue>() const
	{ return *this; }

template <> inline ZType ZTupleValue::Get_T<ZType>() const
	{ return this->GetType(); }

template <> inline uint64 ZTupleValue::Get_T<uint64>() const
	{ return this->GetID(); }

template <> inline int8 ZTupleValue::Get_T<int8>() const
	{ return this->GetInt8(); }

template <> inline int16 ZTupleValue::Get_T<int16>() const
	{ return this->GetInt16(); }

template <> inline int32 ZTupleValue::Get_T<int32>() const
	{ return this->GetInt32(); }

template <> inline int64 ZTupleValue::Get_T<int64>() const
	{ return this->GetInt64(); }

template <> inline bool ZTupleValue::Get_T<bool>() const
	{ return this->GetBool(); }

template <> inline float ZTupleValue::Get_T<float>() const
	{ return this->GetFloat(); }

template <> inline double ZTupleValue::Get_T<double>() const
	{ return this->GetDouble(); }

template <> inline ZTime ZTupleValue::Get_T<ZTime>() const
	{ return this->GetTime(); }

template <> inline std::string ZTupleValue::Get_T<std::string>() const
	{ return this->GetString(); }


template <> inline ZType ZTupleValue::DGet_T<ZType>(ZType iDefault) const
	{ return this->DGetType(iDefault); }

template <> inline uint64 ZTupleValue::DGet_T<uint64>(uint64 iDefault) const
	{ return this->DGetID(iDefault); }

template <> inline int8 ZTupleValue::DGet_T<int8>(int8 iDefault) const
	{ return this->DGetInt8(iDefault); }

template <> inline int16 ZTupleValue::DGet_T<int16>(int16 iDefault) const
	{ return this->DGetInt16(iDefault); }

template <> inline int32 ZTupleValue::DGet_T<int32>(int32 iDefault) const
	{ return this->DGetInt32(iDefault); }

template <> inline int64 ZTupleValue::DGet_T<int64>(int64 iDefault) const
	{ return this->DGetInt64(iDefault); }

template <> inline bool ZTupleValue::DGet_T<bool>(bool iDefault) const
	{ return this->DGetBool(iDefault); }

template <> inline float ZTupleValue::DGet_T<float>(float iDefault) const
	{ return this->DGetFloat(iDefault); }

template <> inline double ZTupleValue::DGet_T<double>(double iDefault) const
	{ return this->DGetDouble(iDefault); }

template <> inline ZTime ZTupleValue::DGet_T<ZTime>(ZTime iDefault) const
	{ return this->DGetTime(iDefault); }

template <> inline std::string ZTupleValue::DGet_T<std::string>(std::string iDefault) const
	{ return this->DGetString(iDefault); }

template <typename OutputIterator>
inline void ZTupleValue::GetVector_T(OutputIterator iIter) const
	{
	const std::vector<ZTupleValue>& theVector = this->GetVector();
	for (std::vector<ZTupleValue>::const_iterator i = theVector.begin(), theEnd = theVector.end();
		i != theEnd; ++i)
		{
		(*i).Get_T(*iIter++);
		}
	}

template <typename OutputIterator, typename T>
inline void ZTupleValue::GetVector_T(OutputIterator iIter, const T& iDummy) const
	{
	const std::vector<ZTupleValue>& theVector = this->GetVector();
	for (std::vector<ZTupleValue>::const_iterator i = theVector.begin(), theEnd = theVector.end();
		i != theEnd; ++i)
		{
		*iIter++ = (*i).Get_T<T>();
		}
	}

template <typename InputIterator>
inline void ZTupleValue::SetVector_T(InputIterator iBegin, InputIterator iEnd)
	{ std::copy(iBegin, iEnd, back_inserter(this->SetMutableVector())); }

template <typename InputIterator, typename T>
inline void ZTupleValue::SetVector_T(InputIterator iBegin, InputIterator iEnd, const T& iDummy)
	{
	std::vector<ZTupleValue>& theVector = this->SetMutableVector();
	while (iBegin != iEnd)
		theVector.push_back(T(*iBegin++));
	}
//@}

class ZTupleValue::Ex_IllegalType : public runtime_error
	{
public:
	Ex_IllegalType(int iType)
	:	runtime_error("Ex_IllegalType"),
		fType(iType)
		{}

	int fType;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple

/// Associative array mapping names to ZTupleValues.

class ZTuple
	{
	ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZTuple, operator_bool_generator_type, operator_bool_type);

	struct NameTV
		{
		friend class ZTuple;
		NameTV();
		NameTV(const NameTV& iOther);

	private:
		NameTV(const char* iName, const ZTupleValue& iTV);
		NameTV(const std::string& iName, const ZTupleValue& iTV);

		NameTV(const char* iName);
		NameTV(const std::string& iName);

		ZTupleValue fTV;
		ZTuplePropName fName;
		};

public:
	/// The type used to store properties.
	typedef std::vector<NameTV> PropList;

	/// Used to iterate through a tuple's properties.
	typedef PropList::iterator const_iterator;


/** \name Canonical Methods
*/	//@{
	ZTuple();
	ZTuple(const ZTuple& iOther);
	~ZTuple();
	ZTuple& operator=(const ZTuple& iOther);
	//@}


/** \name Constructing from stream
*/	//@{
	explicit ZTuple(const ZStreamR& iStreamR);
	//@}


/** \name Streaming
*/	//@{
	void ToStream(const ZStreamW& iStreamW) const;
	void FromStream(const ZStreamR& iStreamR);

	void ToStreamOld(const ZStreamW& iStreamW) const;
	void FromStreamOld(const ZStreamR& iStreamR);
	//@}


/** \name Unioning tuples
*/	//@{
	ZTuple Over(const ZTuple& iUnder) const;
	ZTuple Under(const ZTuple& iOver) const;
	//@}


/** \name Comparison
*/	//@{
	operator operator_bool_type() const;

	int Compare(const ZTuple& iOther) const;

	bool operator==(const ZTuple& iOther) const;
	bool operator!=(const ZTuple& iOther) const;

	bool operator<(const ZTuple& iOther) const;
	bool operator<=(const ZTuple& iOther) const;
	bool operator>(const ZTuple& iOther) const;
	bool operator>=(const ZTuple& iOther) const;

	bool IsSameAs(const ZTuple& iOther) const;

	bool Contains(const ZTuple& iOther) const;
	//@}


/** \name Efficiently determining if a property is a particular string
*/	//@{
	bool IsString(const_iterator iPropIter, const char* iString) const;
	bool IsString(const char* iPropName, const char* iString) const;
	bool IsString(const std::string& iPropName, const char* iString) const;

	bool IsString(const_iterator iPropIter, const std::string& iString) const;
	bool IsString(const char* iPropName, const std::string& iString) const;
	bool IsString(const std::string& iPropName, const std::string& iString) const;
	//@}


/** \name Property meta-data
*/	//@{
	const_iterator begin() const;
	const_iterator end() const;

	bool Empty() const;
	size_t Count() const;

	std::string NameOf(const_iterator iPropIter) const;

	const_iterator IteratorOf(const ZTuplePropName& iPropName) const;
	const_iterator IteratorOf(const char* iPropName) const;
	const_iterator IteratorOf(const std::string& iPropName) const;

	bool Has(const char* iPropName) const;
	bool Has(const std::string& iPropName) const;

	bool TypeOf(const_iterator iPropIter, ZType& oPropertyType) const;
	bool TypeOf(const char* iPropName, ZType& oPropertyType) const;
	bool TypeOf(const std::string& iPropName, ZType& oPropertyType) const;

	ZType TypeOf(const_iterator iPropIter) const;
	ZType TypeOf(const char* iPropName) const;
	ZType TypeOf(const std::string& iPropName) const;
	//@}


/** \name Erasing
*/	//@{
	void Erase(const_iterator iPropIter);
	void Erase(const char* iPropName);
	void Erase(const std::string& iPropName);

	const_iterator EraseAndReturn(const_iterator iPropIter);

	void Clear();
	//@}


/** \name Getting
*/	//@{
	bool GetValue(const_iterator iPropIter, ZTupleValue& oVal) const;
	bool GetValue(const char* iPropName, ZTupleValue& oVal) const;
	bool GetValue(const std::string& iPropName, ZTupleValue& oVal) const;

	const ZTupleValue& GetValue(const_iterator iPropIter) const;
	const ZTupleValue& GetValue(const char* iPropName) const;
	const ZTupleValue& GetValue(const std::string& iPropName) const;
	const ZTupleValue& GetValue(const ZTuplePropName& iPropName) const;


	bool GetType(const_iterator iPropIter, ZType& oVal) const;
	bool GetType(const char* iPropName, ZType& oVal) const;
	bool GetType(const std::string& iPropName, ZType& oVal) const;

	ZType GetType(const_iterator iPropIter) const;
	ZType GetType(const char* iPropName) const;
	ZType GetType(const std::string& iPropName) const;


	bool GetID(const_iterator iPropIter, uint64& oVal) const;
	bool GetID(const char* iPropName, uint64& oVal) const;
	bool GetID(const std::string& iPropName, uint64& oVal) const;

	uint64 GetID(const_iterator iPropIter) const;
	uint64 GetID(const char* iPropName) const;
	uint64 GetID(const std::string& iPropName) const;


	bool GetInt8(const_iterator iPropIter, int8& oVal) const;
	bool GetInt8(const char* iPropName, int8& oVal) const;
	bool GetInt8(const std::string& iPropName, int8& oVal) const;

	int8 GetInt8(const_iterator iPropIter) const;
	int8 GetInt8(const char* iPropName) const;
	int8 GetInt8(const std::string& iPropName) const;


	bool GetInt16(const_iterator iPropIter, int16& oVal) const;
	bool GetInt16(const char* iPropName, int16& oVal) const;
	bool GetInt16(const std::string& iPropName, int16& oVal) const;

	int16 GetInt16(const_iterator iPropIter) const;
	int16 GetInt16(const char* iPropName) const;
	int16 GetInt16(const std::string& iPropName) const;


	bool GetInt32(const_iterator iPropIter, int32& oVal) const;
	bool GetInt32(const char* iPropName, int32& oVal) const;
	bool GetInt32(const std::string& iPropName, int32& oVal) const;

	int32 GetInt32(const_iterator iPropIter) const;
	int32 GetInt32(const char* iPropName) const;
	int32 GetInt32(const std::string& iPropName) const;


	bool GetInt64(const_iterator iPropIter, int64& oVal) const;
	bool GetInt64(const char* iPropName, int64& oVal) const;
	bool GetInt64(const std::string& iPropName, int64& oVal) const;

	int64 GetInt64(const_iterator iPropIter) const;
	int64 GetInt64(const char* iPropName) const;
	int64 GetInt64(const std::string& iPropName) const;


	bool GetBool(const_iterator iPropIter, bool& oVal) const;
	bool GetBool(const char* iPropName, bool& oVal) const;
	bool GetBool(const std::string& iPropName, bool& oVal) const;

	bool GetBool(const_iterator iPropIter) const;
	bool GetBool(const char* iPropName) const;
	bool GetBool(const std::string& iPropName) const;


	bool GetFloat(const_iterator iPropIter, float& oVal) const;
	bool GetFloat(const char* iPropName, float& oVal) const;
	bool GetFloat(const std::string& iPropName, float& oVal) const;

	float GetFloat(const_iterator iPropIter) const;
	float GetFloat(const char* iPropName) const;
	float GetFloat(const std::string& iPropName) const;


	bool GetDouble(const_iterator iPropIter, double& oVal) const;
	bool GetDouble(const char* iPropName, double& oVal) const;
	bool GetDouble(const std::string& iPropName, double& oVal) const;

	double GetDouble(const_iterator iPropIter) const;
	double GetDouble(const char* iPropName) const;
	double GetDouble(const std::string& iPropName) const;


	bool GetTime(const_iterator iPropIter, ZTime& oVal) const;
	bool GetTime(const char* iPropName, ZTime& oVal) const;
	bool GetTime(const std::string& iPropName, ZTime& oVal) const;

	ZTime GetTime(const_iterator iPropIter) const;
	ZTime GetTime(const char* iPropName) const;
	ZTime GetTime(const std::string& iPropName) const;


	bool GetPointer(const_iterator iPropIter, void*& oVal) const;
	bool GetPointer(const char* iPropName, void*& oVal) const;
	bool GetPointer(const std::string& iPropName, void*& oVal) const;

	void* GetPointer(const_iterator iPropIter) const;
	void* GetPointer(const char* iPropName) const;
	void* GetPointer(const std::string& iPropName) const;


	bool GetRect(const_iterator iPropIter, ZRectPOD& oVal) const;
	bool GetRect(const char* iPropName, ZRectPOD& oVal) const;
	bool GetRect(const std::string& iPropName, ZRectPOD& oVal) const;

	const ZRectPOD& GetRect(const_iterator iPropIter) const;
	const ZRectPOD& GetRect(const char* iPropName) const;
	const ZRectPOD& GetRect(const std::string& iPropName) const;


	bool GetPoint(const_iterator iPropIter, ZPointPOD& oVal) const;
	bool GetPoint(const char* iPropName, ZPointPOD& oVal) const;
	bool GetPoint(const std::string& iPropName, ZPointPOD& oVal) const;

	const ZPointPOD& GetPoint(const_iterator iPropIter) const;
	const ZPointPOD& GetPoint(const char* iPropName) const;
	const ZPointPOD& GetPoint(const std::string& iPropName) const;


	bool GetString(const_iterator iPropIter, std::string& oVal) const;
	bool GetString(const char* iPropName, std::string& oVal) const;
	bool GetString(const std::string& iPropName, std::string& oVal) const;

	std::string GetString(const_iterator iPropIter) const;
	std::string GetString(const char* iPropName) const;
	std::string GetString(const std::string& iPropName) const;


	bool GetTuple(const_iterator iPropIter, ZTuple& oVal) const;
	bool GetTuple(const char* iPropName, ZTuple& oVal) const;
	bool GetTuple(const std::string& iPropName, ZTuple& oVal) const;

	const ZTuple& GetTuple(const_iterator iPropIter) const;
	const ZTuple& GetTuple(const char* iPropName) const;
	const ZTuple& GetTuple(const std::string& iPropName) const;


	bool GetRefCounted(const_iterator iPropIter, ZRef<ZRefCountedWithFinalization>& oVal) const;
	bool GetRefCounted(const char* iPropName, ZRef<ZRefCountedWithFinalization>& oVal) const;
	bool GetRefCounted(const std::string& iPropName, ZRef<ZRefCountedWithFinalization>& oVal) const;

	ZRef<ZRefCountedWithFinalization> GetRefCounted(const_iterator iPropIter) const;
	ZRef<ZRefCountedWithFinalization> GetRefCounted(const char* iPropName) const;
	ZRef<ZRefCountedWithFinalization> GetRefCounted(const std::string& iPropName) const;


	bool GetRaw(const_iterator iPropIter, ZMemoryBlock& oVal) const;
	bool GetRaw(const char* iPropName, ZMemoryBlock& oVal) const;
	bool GetRaw(const std::string& iPropName, ZMemoryBlock& oVal) const;

	ZMemoryBlock GetRaw(const_iterator iPropIter) const;
	ZMemoryBlock GetRaw(const char* iPropName) const;
	ZMemoryBlock GetRaw(const std::string& iPropName) const;


	bool GetRaw(const_iterator iPropIter, void* iDest, size_t iSize) const;
	bool GetRaw(const char* iPropName, void* iDest, size_t iSize) const;
	bool GetRaw(const std::string& iPropName, void* iDest, size_t iSize) const;

	bool GetRawAttributes(const_iterator iPropIter, const void** oAddress, size_t* oSize) const;
	bool GetRawAttributes(const char* iPropName, const void** oAddress, size_t* oSize) const;
	bool GetRawAttributes(const std::string& iPropName, const void** oAddress, size_t* oSize) const;


	bool GetVector(const_iterator iPropIter, std::vector<ZTupleValue>& oVal) const;
	bool GetVector(const char* iPropName, std::vector<ZTupleValue>& oVal) const;
	bool GetVector(const std::string& iPropName, std::vector<ZTupleValue>& oVal) const;

	const std::vector<ZTupleValue>& GetVector(const_iterator iPropIter) const;
	const std::vector<ZTupleValue>& GetVector(const char* iPropName) const;
	const std::vector<ZTupleValue>& GetVector(const std::string& iPropName) const;

	template <typename OutputIterator>
	void GetVector_T(const_iterator iPropIter, OutputIterator iIter) const;

	template <typename OutputIterator>
	void GetVector_T(const char* iPropName, OutputIterator iIter) const;

	template <typename OutputIterator>
	void GetVector_T(const std::string& iPropName, OutputIterator iIter) const;

	template <typename OutputIterator, typename T>
	void GetVector_T(const_iterator iPropIter, OutputIterator iIter, const T& iDummy) const;

	template <typename OutputIterator, typename T>
	void GetVector_T(const char* iPropName, OutputIterator iIter, const T& iDummy) const;

	template <typename OutputIterator, typename T>
	void GetVector_T(const std::string& iPropName, OutputIterator iIter, const T& iDummy) const;
	//@}


/** \name Getting types specified by template parameter
*/	//@{
	template <typename T> bool Get_T(const_iterator iPropIter, T& oVal) const;
	template <typename T> bool Get_T(const char* iPropName, T& oVal) const;
	template <typename T> bool Get_T(const std::string& iPropName, T& oVal) const;

	template <typename T> T Get_T(const_iterator iPropIter) const;
	template <typename T> T Get_T(const char* iPropName) const;
	template <typename T> T Get_T(const std::string& iPropName) const;
	//@}


/** \name DGetting
*/	//@{
	const ZTupleValue DGetValue(const_iterator iPropIter, const ZTupleValue& iDefault) const;
	const ZTupleValue DGetValue(const char* iPropName, const ZTupleValue& iDefault) const;
	const ZTupleValue DGetValue(const std::string& iPropName, const ZTupleValue& iDefault) const;


	ZType DGetType(const_iterator iPropIter, ZType iDefault) const;
	ZType DGetType(const char* iPropName, ZType iDefault) const;
	ZType DGetType(const std::string& iPropName, ZType iDefault) const;


	uint64 DGetID(const_iterator iPropIter, uint64 iDefault) const;
	uint64 DGetID(const char* iPropName, uint64 iDefault) const;
	uint64 DGetID(const std::string& iPropName, uint64 iDefault) const;


	int8 DGetInt8(const_iterator iPropIter, int8 iDefault) const;
	int8 DGetInt8(const char* iPropName, int8 iDefault) const;
	int8 DGetInt8(const std::string& iPropName, int8 iDefault) const;


	int16 DGetInt16(const_iterator iPropIter, int16 iDefault) const;
	int16 DGetInt16(const char* iPropName, int16 iDefault) const;
	int16 DGetInt16(const std::string& iPropName, int16 iDefault) const;


	int32 DGetInt32(const_iterator iPropIter, int32 iDefault) const;
	int32 DGetInt32(const char* iPropName, int32 iDefault) const;
	int32 DGetInt32(const std::string& iPropName, int32 iDefault) const;


	int64 DGetInt64(const_iterator iPropIter, int64 iDefault) const;
	int64 DGetInt64(const char* iPropName, int64 iDefault) const;
	int64 DGetInt64(const std::string& iPropName, int64 iDefault) const;


	bool DGetBool(const_iterator iPropIter, bool iDefault) const;
	bool DGetBool(const char* iPropName, bool iDefault) const;
	bool DGetBool(const std::string& iPropName, bool iDefault) const;


	float DGetFloat(const_iterator iPropIter, float iDefault) const;
	float DGetFloat(const char* iPropName, float iDefault) const;
	float DGetFloat(const std::string& iPropName, float iDefault) const;


	double DGetDouble(const_iterator iPropIter, double iDefault) const;
	double DGetDouble(const char* iPropName, double iDefault) const;
	double DGetDouble(const std::string& iPropName, double iDefault) const;


	ZTime DGetTime(const_iterator iPropIter, ZTime iDefault) const;
	ZTime DGetTime(const char* iPropName, ZTime iDefault) const;
	ZTime DGetTime(const std::string& iPropName, ZTime iDefault) const;


	ZRectPOD DGetRect(const_iterator iPropIter, const ZRectPOD& iDefault) const;
	ZRectPOD DGetRect(const char* iPropName, const ZRectPOD& iDefault) const;
	ZRectPOD DGetRect(const std::string& iPropName, const ZRectPOD& iDefault) const;


	ZPointPOD DGetPoint(const_iterator iPropIter, const ZPointPOD& iDefault) const;
	ZPointPOD DGetPoint(const char* iPropName, const ZPointPOD& iDefault) const;
	ZPointPOD DGetPoint(const std::string& iPropName, const ZPointPOD& iDefault) const;


	std::string DGetString(const_iterator iPropIter, const std::string& iDefault) const;
	std::string DGetString(const char* iPropName, const std::string& iDefault) const;
	std::string DGetString(const std::string& iPropName, const std::string& iDefault) const;


	template <typename T> T DGet_T(const_iterator iPropIter, T iDefault) const;
	template <typename T> T DGet_T(const char* iPropName, T iDefault) const;
	template <typename T> T DGet_T(const std::string& iPropName, T iDefault) const;
	//@}


/** \name Setting
*/	//@{
	bool SetValue(const_iterator iPropIter, const ZTupleValue& iVal);
	ZTuple& SetValue(const char* iPropName, const ZTupleValue& iVal);
	ZTuple& SetValue(const std::string& iPropName, const ZTupleValue& iVal);


	bool SetNull(const_iterator iPropIter);
	ZTuple& SetNull(const char* iPropName);
	ZTuple& SetNull(const std::string& iPropName);


	bool SetType(const_iterator iPropIter, ZType iVal);
	ZTuple& SetType(const char* iPropName, ZType iVal);
	ZTuple& SetType(const std::string& iPropName, ZType iVal);


	bool SetID(const_iterator iPropIter, uint64 iVal);
	ZTuple& SetID(const char* iPropName, uint64 iVal);
	ZTuple& SetID(const std::string& iPropName, uint64 iVal);


	bool SetInt8(const_iterator iPropIter, int8 iVal);
	ZTuple& SetInt8(const char* iPropName, int8 iVal);
	ZTuple& SetInt8(const std::string& iPropName, int8 iVal);


	bool SetInt16(const_iterator iPropIter, int16 iVal);
	ZTuple& SetInt16(const char* iPropName, int16 iVal);
	ZTuple& SetInt16(const std::string& iPropName, int16 iVal);


	bool SetInt32(const_iterator iPropIter, int32 iVal);
	ZTuple& SetInt32(const char* iPropName, int32 iVal);
	ZTuple& SetInt32(const std::string& iPropName, int32 iVal);


	bool SetInt64(const_iterator iPropIter, int64 iVal);
	ZTuple& SetInt64(const char* iPropName, int64 iVal);
	ZTuple& SetInt64(const std::string& iPropName, int64 iVal);


	bool SetBool(const_iterator iPropIter, bool iVal);
	ZTuple& SetBool(const char* iPropName, bool iVal);
	ZTuple& SetBool(const std::string& iPropName, bool iVal);


	bool SetFloat(const_iterator iPropIter, float iVal);
	ZTuple& SetFloat(const char* iPropName, float iVal);
	ZTuple& SetFloat(const std::string& iPropName, float iVal);


	bool SetDouble(const_iterator iPropIter, double iVal);
	ZTuple& SetDouble(const char* iPropName, double iVal);
	ZTuple& SetDouble(const std::string& iPropName, double iVal);


	bool SetTime(const_iterator iPropIter, ZTime iVal);
	ZTuple& SetTime(const char* iPropName, ZTime iVal);
	ZTuple& SetTime(const std::string& iPropName, ZTime iVal);


	bool SetPointer(const_iterator iPropIter, void* iVal);
	ZTuple& SetPointer(const char* iPropName, void* iVal);
	ZTuple& SetPointer(const std::string& iPropName, void* iVal);


	bool SetRect(const_iterator iPropIter, const ZRectPOD& iVal);
	ZTuple& SetRect(const char* iPropName, const ZRectPOD& iVal);
	ZTuple& SetRect(const std::string& iPropName, const ZRectPOD& iVal);


	bool SetPoint(const_iterator iPropIter, const ZPointPOD& iVal);
	ZTuple& SetPoint(const char* iPropName, const ZPointPOD& iVal);
	ZTuple& SetPoint(const std::string& iPropName, const ZPointPOD& iVal);


	bool SetString(const_iterator iPropIter, const char* iVal);
	ZTuple& SetString(const char* iPropName, const char* iVal);
	ZTuple& SetString(const std::string& iPropName, const char* iVal);


	bool SetString(const_iterator iPropIter, const std::string& iVal);
	ZTuple& SetString(const char* iPropName, const std::string& iVal);
	ZTuple& SetString(const std::string& iPropName, const std::string& iVal);


	bool SetTuple(const_iterator iPropIter, const ZTuple& iVal);
	ZTuple& SetTuple(const char* iPropName, const ZTuple& iVal);
	ZTuple& SetTuple(const std::string& iPropName, const ZTuple& iVal);

	bool SetRefCounted(const_iterator iPropIter, const ZRef<ZRefCountedWithFinalization>& iVal);
	ZTuple& SetRefCounted(const char* iPropName, const ZRef<ZRefCountedWithFinalization>& iVal);
	ZTuple& SetRefCounted(const std::string& iPropName,
		const ZRef<ZRefCountedWithFinalization>& iVal);


	bool SetRaw(const_iterator iPropIter, const ZMemoryBlock& iVal);
	ZTuple& SetRaw(const char* iPropName, const ZMemoryBlock& iVal);
	ZTuple& SetRaw(const std::string& iPropName, const ZMemoryBlock& iVal);


	bool SetRaw(const_iterator iPropIter, const void* iSource, size_t iSize);
	ZTuple& SetRaw(const char* iPropName, const void* iSource, size_t iSize);
	ZTuple& SetRaw(const std::string& iPropName, const void* iSource, size_t iSize);


	bool SetRaw(const_iterator iPropIter, const ZStreamR& iStreamR, size_t iSize);
	ZTuple& SetRaw(const char* iPropName, const ZStreamR& iStreamR, size_t iSize);
	ZTuple& SetRaw(const std::string& iPropName, const ZStreamR& iStreamR, size_t iSize);


	bool SetRaw(const_iterator iPropIter, const ZStreamR& iStreamR);
	ZTuple& SetRaw(const char* iPropName, const ZStreamR& iStreamR);
	ZTuple& SetRaw(const std::string& iPropName, const ZStreamR& iStreamR);


	bool SetVector(const_iterator iPropIter, const std::vector<ZTupleValue>& iVal);
	ZTuple& SetVector(const char* iPropName, const std::vector<ZTupleValue>& iVal);
	ZTuple& SetVector(const std::string& iPropName, const std::vector<ZTupleValue>& iVal);

	template <typename InputIterator>
	void SetVector_T(const_iterator iPropIter, InputIterator iBegin, InputIterator iEnd);

	template <typename InputIterator>
	void SetVector_T(const char* iPropName, InputIterator iBegin, InputIterator iEnd);

	template <typename InputIterator>
	void SetVector_T(const std::string& iPropName, InputIterator iBegin, InputIterator iEnd);


	template <typename InputIterator, typename T>
	void SetVector_T(const_iterator iPropIter,
		InputIterator iBegin, InputIterator iEnd, const T& iDummy);

	template <typename InputIterator, typename T>
	void SetVector_T(const char* iPropName,
		InputIterator iBegin, InputIterator iEnd, const T& iDummy);

	template <typename InputIterator, typename T>
	void SetVector_T(const std::string& iPropName,
		InputIterator iBegin, InputIterator iEnd, const T& iDummy);
	//@}


/** \name Getting mutable references to extant properties, must be of correct type
*/	//@{
	ZTupleValue& GetMutableValue(const_iterator iPropIter);
	ZTupleValue& GetMutableValue(const char* iPropName);
	ZTupleValue& GetMutableValue(const std::string& iPropName);


	ZTuple& GetMutableTuple(const_iterator iPropIter);
	ZTuple& GetMutableTuple(const char* iPropName);
	ZTuple& GetMutableTuple(const std::string& iPropName);


	std::vector<ZTupleValue>& GetMutableVector(const_iterator iPropIter);
	std::vector<ZTupleValue>& GetMutableVector(const char* iPropName);
	std::vector<ZTupleValue>& GetMutableVector(const std::string& iPropName);
	//@}


/** \name Setting empty types and return reference to the actual entity
*/	//@{
	ZTupleValue& SetMutableNull(const_iterator iPropIter);
	ZTupleValue& SetMutableNull(const char* iPropName);
	ZTupleValue& SetMutableNull(const std::string& iPropName);


	ZTuple& SetMutableTuple(const_iterator iPropIter);
	ZTuple& SetMutableTuple(const char* iPropName);
	ZTuple& SetMutableTuple(const std::string& iPropName);


	std::vector<ZTupleValue>& SetMutableVector(const_iterator iPropIter);
	std::vector<ZTupleValue>& SetMutableVector(const char* iPropName);
	std::vector<ZTupleValue>& SetMutableVector(const std::string& iPropName);
	//@}


/** \name Getting mutable reference to actual value, changing type if necessary
*/	//@{
	ZTupleValue& EnsureMutableValue(const_iterator iPropIter);
	ZTupleValue& EnsureMutableValue(const char* iPropName);
	ZTupleValue& EnsureMutableValue(const std::string& iPropName);


	ZTuple& EnsureMutableTuple(const_iterator iPropIter);
	ZTuple& EnsureMutableTuple(const char* iPropName);
	ZTuple& EnsureMutableTuple(const std::string& iPropName);


	std::vector<ZTupleValue>& EnsureMutableVector(const_iterator iPropIter);
	std::vector<ZTupleValue>& EnsureMutableVector(const char* iPropName);
	std::vector<ZTupleValue>& EnsureMutableVector(const std::string& iPropName);
	//@}


/** \name Appending data
*/	//@{
	void AppendValue(const char* iPropName, const ZTupleValue& iVal);
	void AppendValue(const std::string& iPropName, const ZTupleValue& iVal);


	void AppendType(const char* iPropName, ZType iVal);
	void AppendType(const std::string& iPropName, ZType iVal);


	void AppendID(const char* iPropName, uint64 iVal);
	void AppendID(const std::string& iPropName, uint64 iVal);


	void AppendInt8(const char* iPropName, int8 iVal);
	void AppendInt8(const std::string& iPropName, int8 iVal);


	void AppendInt16(const char* iPropName, int16 iVal);
	void AppendInt16(const std::string& iPropName, int16 iVal);


	void AppendInt32(const char* iPropName, int32 iVal);
	void AppendInt32(const std::string& iPropName, int32 iVal);


	void AppendInt64(const char* iPropName, int64 iVal);
	void AppendInt64(const std::string& iPropName, int64 iVal);


	void AppendBool(const char* iPropName, bool iVal);
	void AppendBool(const std::string& iPropName, bool iVal);


	void AppendFloat(const char* iPropName, float iVal);
	void AppendFloat(const std::string& iPropName, float iVal);


	void AppendDouble(const char* iPropName, double iVal);
	void AppendDouble(const std::string& iPropName, double iVal);


	void AppendTime(const char* iPropName, ZTime iVal);
	void AppendTime(const std::string& iPropName, ZTime iVal);


	void AppendPointer(const char* iPropName, void* iVal);
	void AppendPointer(const std::string& iPropName, void* iVal);


	void AppendRect(const char* iPropName, const ZRectPOD& iVal);
	void AppendRect(const std::string& iPropName, const ZRectPOD& iVal);


	void AppendPoint(const char* iPropName, const ZPointPOD& iVal);
	void AppendPoint(const std::string& iPropName, const ZPointPOD& iVal);


	void AppendString(const char* iPropName, const char* iVal);
	void AppendString(const std::string& iPropName, const char* iVal);


	void AppendString(const char* iPropName, const std::string& iVal);
	void AppendString(const std::string& iPropName, const std::string& iVal);


	void AppendTuple(const char* iPropName, const ZTuple& iVal);
	void AppendTuple(const std::string& iPropName, const ZTuple& iVal);


	void AppendRefCounted(const char* iPropName, const ZRef<ZRefCountedWithFinalization>& iVal);
	void AppendRefCounted(const std::string& iPropName,
		const ZRef<ZRefCountedWithFinalization>& iVal);


	void AppendRaw(const char* iPropName, const ZMemoryBlock& iVal);
	void AppendRaw(const std::string& iPropName, const ZMemoryBlock& iVal);


	void AppendRaw(const char* iPropName, const void* iSource, size_t iSize);
	void AppendRaw(const std::string& iPropName, const void* iSource, size_t iSize);
	//@}


	void swap(ZTuple& iOther);

	ZTuple& Minimize();
	ZTuple Minimized() const;

protected:
	bool pSet(const_iterator iPropIter, const ZTupleValue& iVal);
	ZTuple& pSet(const char* iPropName, const ZTupleValue& iVal);
	ZTuple& pSet(const std::string& iPropName, const ZTupleValue& iVal);

	ZTupleValue* pFindOrAllocate(const char* iPropName);
	ZTupleValue* pFindOrAllocate(const std::string& iPropName);

	ZTuple& pAppend(const char* iPropName, const ZTupleValue& iVal);
	ZTuple& pAppend(const std::string& iPropName, const ZTupleValue& iVal);

	const ZTupleValue* pLookupAddressConst(const_iterator iPropIter) const;
	const ZTupleValue* pLookupAddressConst(const char* iPropName) const;
	const ZTupleValue* pLookupAddressConst(const std::string& iPropName) const;

	ZTupleValue* pLookupAddress(const_iterator iPropIter);
	ZTupleValue* pLookupAddress(const char* iPropName);
	ZTupleValue* pLookupAddress(const std::string& iPropName);

	const ZTupleValue& pLookupConst(const_iterator iPropIter) const;
	const ZTupleValue& pLookupConst(const char* iPropName) const;
	const ZTupleValue& pLookupConst(const std::string& iPropName) const;

	void pErase(const_iterator iPropIter);
	void pTouch();
	const_iterator pTouch(const_iterator iPropIter);
	bool pIsEqual(const ZTuple& iOther) const;
	bool pIsSameAs(const ZTuple& iOther) const;

	static ZRef<ZTupleRep> sRepFromStream(const ZStreamR& iStreamR);

	ZRef<ZTupleRep> fRep;
	};

template <typename OutputIterator>
inline void ZTuple::GetVector_T(const_iterator iPropIter, OutputIterator iIter) const
	{ this->GetValue(iPropIter).GetVector_T(iIter); }

template <typename OutputIterator>
inline void ZTuple::GetVector_T(const char* iPropName, OutputIterator iIter) const
	{ this->GetValue(iPropName).GetVector_T(iIter); }

template <typename OutputIterator>
inline void ZTuple::GetVector_T(const std::string& iPropName, OutputIterator iIter) const
	{ this->GetValue(iPropName).GetVector_T(iIter); }

template <typename OutputIterator, typename T>
inline void ZTuple::GetVector_T(const_iterator iPropIter,
	OutputIterator iIter, const T& iDummy) const
	{ this->GetValue(iPropIter).GetVector_T(iIter, iDummy); }

template <typename OutputIterator, typename T>
inline void ZTuple::GetVector_T(const char* iPropName, OutputIterator iIter, const T& iDummy) const
	{ this->GetValue(iPropName).GetVector_T(iIter, iDummy); }

template <typename OutputIterator, typename T>
inline void ZTuple::GetVector_T(const std::string& iPropName,
	OutputIterator iIter, const T& iDummy) const
	{ this->GetValue(iPropName).GetVector_T(iIter, iDummy); }

template <typename InputIterator>
inline void ZTuple::SetVector_T(const_iterator iPropIter, InputIterator iBegin, InputIterator iEnd)
	{ this->SetMutableNull(iPropIter).SetVector_T(iBegin, iEnd); }

template <typename InputIterator>
inline void ZTuple::SetVector_T(const char* iPropName, InputIterator iBegin, InputIterator iEnd)
	{ this->SetMutableNull(iPropName).SetVector_T(iBegin, iEnd); }

template <typename InputIterator>
inline void ZTuple::SetVector_T(const std::string& iPropName,
	InputIterator iBegin, InputIterator iEnd)
	{ this->SetMutableNull(iPropName).SetVector_T(iBegin, iEnd); }

template <typename InputIterator, typename T>
inline void ZTuple::SetVector_T(const_iterator iPropIter,
	InputIterator iBegin, InputIterator iEnd, const T& iDummy)
	{ this->SetMutableNull(iPropIter).SetVector_T(iBegin, iEnd, iDummy); }

template <typename InputIterator, typename T>
inline void ZTuple::SetVector_T(const char* iPropName,
	InputIterator iBegin, InputIterator iEnd, const T& iDummy)
	{ this->SetMutableNull(iPropName).SetVector_T(iBegin, iEnd, iDummy); }

template <typename InputIterator, typename T>
inline void ZTuple::SetVector_T(const std::string& iPropName,
	InputIterator iBegin, InputIterator iEnd, const T& iDummy)
	{ this->SetMutableNull(iPropName).SetVector_T(iBegin, iEnd, iDummy); }

template <>
inline bool ZTuple::Get_T<ZTupleValue>(const_iterator iPropIter, ZTupleValue& oVal) const
	{ return this->GetValue(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<ZType>(const_iterator iPropIter, ZType& oVal) const
	{ return this->GetType(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<uint64>(const_iterator iPropIter, uint64& oVal) const
	{ return this->GetID(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<int8>(const_iterator iPropIter, int8& oVal) const
	{ return this->GetInt8(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<int16>(const_iterator iPropIter, int16& oVal) const
	{ return this->GetInt16(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<int32>(const_iterator iPropIter, int32& oVal) const
	{ return this->GetInt32(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<int64>(const_iterator iPropIter, int64& oVal) const
	{ return this->GetInt64(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<bool>(const_iterator iPropIter, bool& oVal) const
	{ return this->GetBool(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<float>(const_iterator iPropIter, float& oVal) const
	{ return this->GetFloat(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<double>(const_iterator iPropIter, double& oVal) const
	{ return this->GetDouble(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<ZTime>(const_iterator iPropIter, ZTime& oVal) const
	{ return this->GetTime(iPropIter, oVal); }

template <>
inline bool ZTuple::Get_T<std::string>(const_iterator iPropIter, std::string& oVal) const
	{ return this->GetString(iPropIter, oVal); }


template <>
inline bool ZTuple::Get_T<ZTupleValue>(const char* iPropName, ZTupleValue& oVal) const
	{ return this->GetValue(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<ZType>(const char* iPropName, ZType& oVal) const
	{ return this->GetType(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<uint64>(const char* iPropName, uint64& oVal) const
	{ return this->GetID(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int8>(const char* iPropName, int8& oVal) const
	{ return this->GetInt8(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int16>(const char* iPropName, int16& oVal) const
	{ return this->GetInt16(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int32>(const char* iPropName, int32& oVal) const
	{ return this->GetInt32(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int64>(const char* iPropName, int64& oVal) const
	{ return this->GetInt64(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<bool>(const char* iPropName, bool& oVal) const
	{ return this->GetBool(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<float>(const char* iPropName, float& oVal) const
	{ return this->GetFloat(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<double>(const char* iPropName, double& oVal) const
	{ return this->GetDouble(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<ZTime>(const char* iPropName, ZTime& oVal) const
	{ return this->GetTime(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<std::string>(const char* iPropName, std::string& oVal) const
	{ return this->GetString(iPropName, oVal); }


template <>
inline bool ZTuple::Get_T<ZTupleValue>(const std::string& iPropName, ZTupleValue& oVal) const
	{ return this->GetValue(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<ZType>(const std::string& iPropName, ZType& oVal) const
	{ return this->GetType(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<uint64>(const std::string& iPropName, uint64& oVal) const
	{ return this->GetID(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int8>(const std::string& iPropName, int8& oVal) const
	{ return this->GetInt8(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int16>(const std::string& iPropName, int16& oVal) const
	{ return this->GetInt16(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int32>(const std::string& iPropName, int32& oVal) const
	{ return this->GetInt32(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<int64>(const std::string& iPropName, int64& oVal) const
	{ return this->GetInt64(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<bool>(const std::string& iPropName, bool& oVal) const
	{ return this->GetBool(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<float>(const std::string& iPropName, float& oVal) const
	{ return this->GetFloat(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<double>(const std::string& iPropName, double& oVal) const
	{ return this->GetDouble(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<ZTime>(const std::string& iPropName, ZTime& oVal) const
	{ return this->GetTime(iPropName, oVal); }

template <>
inline bool ZTuple::Get_T<std::string>(const std::string& iPropName, std::string& oVal) const
	{ return this->GetString(iPropName, oVal); }


template <>
inline ZTupleValue ZTuple::Get_T<ZTupleValue>(const_iterator iPropIter) const
	{ return this->GetValue(iPropIter); }

template <>
inline ZType ZTuple::Get_T<ZType>(const_iterator iPropIter) const
	{ return this->GetType(iPropIter); }

template <>
inline uint64 ZTuple::Get_T<uint64>(const_iterator iPropIter) const
	{ return this->GetID(iPropIter); }

template <>
inline int8 ZTuple::Get_T<int8>(const_iterator iPropIter) const
	{ return this->GetInt8(iPropIter); }

template <>
inline int16 ZTuple::Get_T<int16>(const_iterator iPropIter) const
	{ return this->GetInt16(iPropIter); }

template <>
inline int32 ZTuple::Get_T<int32>(const_iterator iPropIter) const
	{ return this->GetInt32(iPropIter); }

template <>
inline int64 ZTuple::Get_T<int64>(const_iterator iPropIter) const
	{ return this->GetInt64(iPropIter); }

template <>
inline bool ZTuple::Get_T<bool>(const_iterator iPropIter) const
	{ return this->GetBool(iPropIter); }

template <>
inline float ZTuple::Get_T<float>(const_iterator iPropIter) const
	{ return this->GetFloat(iPropIter); }

template <>
inline double ZTuple::Get_T<double>(const_iterator iPropIter) const
	{ return this->GetDouble(iPropIter); }

template <>
inline ZTime ZTuple::Get_T<ZTime>(const_iterator iPropIter) const
	{ return this->GetTime(iPropIter); }

template <>
inline std::string ZTuple::Get_T<std::string>(const_iterator iPropIter) const
	{ return this->GetString(iPropIter); }


template <>
inline ZTupleValue ZTuple::Get_T<ZTupleValue>(const char* iPropName) const
	{ return this->GetValue(iPropName); }

template <>
inline ZType ZTuple::Get_T<ZType>(const char* iPropName) const
	{ return this->GetType(iPropName); }

template <>
inline uint64 ZTuple::Get_T<uint64>(const char* iPropName) const
	{ return this->GetID(iPropName); }

template <>
inline int8 ZTuple::Get_T<int8>(const char* iPropName) const
	{ return this->GetInt8(iPropName); }

template <>
inline int16 ZTuple::Get_T<int16>(const char* iPropName) const
	{ return this->GetInt16(iPropName); }

template <>
inline int32 ZTuple::Get_T<int32>(const char* iPropName) const
	{ return this->GetInt32(iPropName); }

template <>
inline int64 ZTuple::Get_T<int64>(const char* iPropName) const
	{ return this->GetInt64(iPropName); }

template <>
inline bool ZTuple::Get_T<bool>(const char* iPropName) const
	{ return this->GetBool(iPropName); }

template <>
inline float ZTuple::Get_T<float>(const char* iPropName) const
	{ return this->GetFloat(iPropName); }

template <>
inline double ZTuple::Get_T<double>(const char* iPropName) const
	{ return this->GetDouble(iPropName); }

template <>
inline ZTime ZTuple::Get_T<ZTime>(const char* iPropName) const
	{ return this->GetTime(iPropName); }

template <>
inline std::string ZTuple::Get_T<std::string>(const char* iPropName) const
	{ return this->GetString(iPropName); }


template <>
inline ZTupleValue ZTuple::Get_T<ZTupleValue>(const std::string& iPropName) const
	{ return this->GetValue(iPropName); }

template <>
inline ZType ZTuple::Get_T<ZType>(const std::string& iPropName) const
	{ return this->GetType(iPropName); }

template <>
inline uint64 ZTuple::Get_T<uint64>(const std::string& iPropName) const
	{ return this->GetID(iPropName); }

template <>
inline int8 ZTuple::Get_T<int8>(const std::string& iPropName) const
	{ return this->GetInt8(iPropName); }

template <>
inline int16 ZTuple::Get_T<int16>(const std::string& iPropName) const
	{ return this->GetInt16(iPropName); }

template <>
inline int32 ZTuple::Get_T<int32>(const std::string& iPropName) const
	{ return this->GetInt32(iPropName); }

template <>
inline int64 ZTuple::Get_T<int64>(const std::string& iPropName) const
	{ return this->GetInt64(iPropName); }

template <>
inline bool ZTuple::Get_T<bool>(const std::string& iPropName) const
	{ return this->GetBool(iPropName); }

template <>
inline float ZTuple::Get_T<float>(const std::string& iPropName) const
	{ return this->GetFloat(iPropName); }

template <>
inline double ZTuple::Get_T<double>(const std::string& iPropName) const
	{ return this->GetDouble(iPropName); }

template <>
inline ZTime ZTuple::Get_T<ZTime>(const std::string& iPropName) const
	{ return this->GetTime(iPropName); }

template <>
inline std::string ZTuple::Get_T<std::string>(const std::string& iPropName) const
	{ return this->GetString(iPropName); }


template <>
inline ZType ZTuple::DGet_T<ZType>(const_iterator iPropIter, ZType iDefault) const
	{ return this->DGetType(iPropIter, iDefault); }

template <>
inline uint64 ZTuple::DGet_T<uint64>(const_iterator iPropIter, uint64 iDefault) const
	{ return this->DGetID(iPropIter, iDefault); }

template <>
inline int8 ZTuple::DGet_T<int8>(const_iterator iPropIter, int8 iDefault) const
	{ return this->DGetInt8(iPropIter, iDefault); }

template <>
inline int16 ZTuple::DGet_T<int16>(const_iterator iPropIter, int16 iDefault) const
	{ return this->DGetInt16(iPropIter, iDefault); }

template <>
inline int32 ZTuple::DGet_T<int32>(const_iterator iPropIter, int32 iDefault) const
	{ return this->DGetInt32(iPropIter, iDefault); }

template <>
inline int64 ZTuple::DGet_T<int64>(const_iterator iPropIter, int64 iDefault) const
	{ return this->DGetInt64(iPropIter, iDefault); }

template <>
inline bool ZTuple::DGet_T<bool>(const_iterator iPropIter, bool iDefault) const
	{ return this->DGetBool(iPropIter, iDefault); }

template <>
inline float ZTuple::DGet_T<float>(const_iterator iPropIter, float iDefault) const
	{ return this->DGetFloat(iPropIter, iDefault); }

template <>
inline double ZTuple::DGet_T<double>(const_iterator iPropIter, double iDefault) const
	{ return this->DGetDouble(iPropIter, iDefault); }

template <>
inline ZTime ZTuple::DGet_T<ZTime>(const_iterator iPropIter, ZTime iDefault) const
	{ return this->DGetTime(iPropIter, iDefault); }

template <>
inline std::string ZTuple::DGet_T<std::string>(const_iterator iPropIter, std::string iDefault) const
	{ return this->DGetString(iPropIter, iDefault); }


template <>
inline ZType ZTuple::DGet_T<ZType>(const char* iPropName, ZType iDefault) const
	{ return this->DGetType(iPropName, iDefault); }

template <>
inline uint64 ZTuple::DGet_T<uint64>(const char* iPropName, uint64 iDefault) const
	{ return this->DGetID(iPropName, iDefault); }

template <>
inline int8 ZTuple::DGet_T<int8>(const char* iPropName, int8 iDefault) const
	{ return this->DGetInt8(iPropName, iDefault); }

template <>
inline int16 ZTuple::DGet_T<int16>(const char* iPropName, int16 iDefault) const
	{ return this->DGetInt16(iPropName, iDefault); }

template <>
inline int32 ZTuple::DGet_T<int32>(const char* iPropName, int32 iDefault) const
	{ return this->DGetInt32(iPropName, iDefault); }

template <>
inline int64 ZTuple::DGet_T<int64>(const char* iPropName, int64 iDefault) const
	{ return this->DGetInt64(iPropName, iDefault); }

template <>
inline bool ZTuple::DGet_T<bool>(const char* iPropName, bool iDefault) const
	{ return this->DGetBool(iPropName, iDefault); }

template <>
inline float ZTuple::DGet_T<float>(const char* iPropName, float iDefault) const
	{ return this->DGetFloat(iPropName, iDefault); }

template <>
inline double ZTuple::DGet_T<double>(const char* iPropName, double iDefault) const
	{ return this->DGetDouble(iPropName, iDefault); }

template <>
inline ZTime ZTuple::DGet_T<ZTime>(const char* iPropName, ZTime iDefault) const
	{ return this->DGetTime(iPropName, iDefault); }

template <>
inline std::string ZTuple::DGet_T<std::string>(const char* iPropName, std::string iDefault) const
	{ return this->DGetString(iPropName, iDefault); }


template <>
inline ZType ZTuple::DGet_T<ZType>(const std::string& iPropName, ZType iDefault) const
	{ return this->DGetType(iPropName, iDefault); }

template <>
inline uint64 ZTuple::DGet_T<uint64>(const std::string& iPropName, uint64 iDefault) const
	{ return this->DGetID(iPropName, iDefault); }

template <>
inline int8 ZTuple::DGet_T<int8>(const std::string& iPropName, int8 iDefault) const
	{ return this->DGetInt8(iPropName, iDefault); }

template <>
inline int16 ZTuple::DGet_T<int16>(const std::string& iPropName, int16 iDefault) const
	{ return this->DGetInt16(iPropName, iDefault); }

template <>
inline int32 ZTuple::DGet_T<int32>(const std::string& iPropName, int32 iDefault) const
	{ return this->DGetInt32(iPropName, iDefault); }

template <>
inline int64 ZTuple::DGet_T<int64>(const std::string& iPropName, int64 iDefault) const
	{ return this->DGetInt64(iPropName, iDefault); }

template <>
inline bool ZTuple::DGet_T<bool>(const std::string& iPropName, bool iDefault) const
	{ return this->DGetBool(iPropName, iDefault); }

template <>
inline float ZTuple::DGet_T<float>(const std::string& iPropName, float iDefault) const
	{ return this->DGetFloat(iPropName, iDefault); }

template <>
inline double ZTuple::DGet_T<double>(const std::string& iPropName, double iDefault) const
	{ return this->DGetDouble(iPropName, iDefault); }

template <>
inline ZTime ZTuple::DGet_T<ZTime>(const std::string& iPropName, ZTime iDefault) const
	{ return this->DGetTime(iPropName, iDefault); }

template <>
inline std::string ZTuple::DGet_T<std::string>(const std::string& iPropName,
	std::string iDefault) const
	{ return this->DGetString(iPropName, iDefault); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleRep

/// ZTupleRep is an implementation detail. Try not to build anything that depends on it.

class ZTupleRep : public ZRefCounted
	{
public:
	static inline bool sCheckAccessEnabled() { return false; }

	ZTupleRep();
	ZTupleRep(const ZTupleRep* iOther);
	virtual ~ZTupleRep();

	/** fProperties is public to allow direct manipulation by utility code. But it
	is subject to change, so be careful. */
	ZTuple::PropList fProperties;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTuple inlines

inline ZTuple::ZTuple()
	{}

inline ZTuple::ZTuple(const ZTuple& iOther)
:	fRep(iOther.fRep)
	{}

inline ZTuple::~ZTuple()
	{}

inline ZTuple& ZTuple::operator=(const ZTuple& iOther)
	{
	fRep = iOther.fRep;
	return *this;
	}

inline ZTuple::operator operator_bool_type() const
	{ return operator_bool_generator_type::translate(fRep && !fRep->fProperties.empty()); }

inline ZTuple ZTuple::Under(const ZTuple& iOver) const
	{ return iOver.Over(*this); }

inline bool ZTuple::operator==(const ZTuple& iOther) const
	{ return fRep == iOther.fRep || this->pIsEqual(iOther); }

inline bool ZTuple::operator!=(const ZTuple& iOther) const
	{ return !(*this == iOther); }

inline bool ZTuple::operator<=(const ZTuple& iOther) const
	{ return !(iOther < *this); }

inline bool ZTuple::operator>(const ZTuple& iOther) const
	{ return iOther < *this; }

inline bool ZTuple::operator>=(const ZTuple& iOther) const
	{ return !(*this < iOther); }

inline bool ZTuple::IsSameAs(const ZTuple& iOther) const
	{ return fRep == iOther.fRep || this->pIsSameAs(iOther); }

inline void ZTuple::swap(ZTuple& iOther)
	{ std::swap(fRep, iOther.fRep); }

namespace std {
inline void swap(ZTuple& a, ZTuple& b)
	{ a.swap(b); }
} // namespace std

namespace ZooLib {
template <> inline int sCompare_T(const ZTupleValue& iL, const ZTupleValue& iR)
	{ return iL.Compare(iR); }
} // namespace ZooLib

#endif // __ZTuple__
