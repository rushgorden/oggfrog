static const char rcsid[] = "@(#) $Id: ZSlotStore.cpp,v 1.6 2006/07/17 18:33:16 agreen Exp $";

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

#include "ZSlotStore.h"
#include "ZDebug.h"
#include "ZMemory.h"
#include "ZByteSwap.h"

#include <string> // may be needed for calls to runtime_error's constructor

using std::runtime_error;

// =================================================================================================

#pragma mark -
#pragma mark * ZSlotStore::BitSet

class ZSlotStore::BitSet
	{
public:
	BitSet(uint32 inMaxIndex);
	BitSet(const BitSet& b);
	~BitSet();

	void ToStream(const ZStreamW& inStream) const;
	void FromStream(const ZStreamR& inStream);
	size_t SizeInStream();
	static size_t sSizeInStream(uint32 inMaxIndex);

	void Add(uint32 inMemberIndex);
	void Remove(uint32 inMemberIndex);
	bool Includes(uint32 inMemberIndex) const;

	uint32 SmallestMember() const;
	uint32 LargestMember() const;
	uint32 SmallestNonMember() const;
	uint32 SuccessorMember(uint32 inMemberIndex) const;

	bool IsEmpty() const;
	bool IsUniversal() const;

 protected:
	int32 fCount; // Current number of set bits
	int32 fPhysicalSize; // # uint32s in representation
	uint32* fRep;
	};

static inline uint32 _Div32(uint32 x) { return (x >> 5) & ((1L << 27) - 1); }
static inline uint32 _Mod32(uint32 x) { return x & ((1L << 5) - 1); }
static inline uint32 _Mul32(uint32 x) { return x << 5; }

static const size_t kBitsInLong = sizeof(uint32) * 8;

static uint32 sBitTable[32] =
		{
		 0x00000001L, 0x00000002L, 0x00000004L, 0x00000008L,
		 0x00000010L, 0x00000020L, 0x00000040L, 0x00000080L,
		 0x00000100L, 0x00000200L, 0x00000400L, 0x00000800L,
		 0x00001000L, 0x00002000L, 0x00004000L, 0x00008000L,
		 0x00010000L, 0x00020000L, 0x00040000L, 0x00080000L,
		 0x00100000L, 0x00200000L, 0x00400000L, 0x00800000L,
		 0x01000000L, 0x02000000L, 0x04000000L, 0x08000000L,
		 0x10000000L, 0x20000000L, 0x40000000L, 0x80000000L
		 };

ZSlotStore::BitSet::BitSet(uint32 inMaxIndex)
	{
	uint32 div = _Div32(inMaxIndex);
	uint32 mod = _Mod32(inMaxIndex);
	fCount = 0;
	fPhysicalSize = (mod == 0) ? div : div + 1;
	if (fPhysicalSize)
		{
		fRep = new uint32[fPhysicalSize];
		ZBlockSet(fRep, sizeof(uint32) * fPhysicalSize, 0);
		}
	else
		{
		fRep = nil;
		}
	}

ZSlotStore::BitSet::BitSet(const BitSet& inOther)
	{
	fCount = inOther.fCount;
	fPhysicalSize = inOther.fPhysicalSize;
	if (inOther.fRep)
		{
		fRep = new uint32[fPhysicalSize];
		ZBlockCopy(inOther.fRep, fRep, sizeof(uint32) * fPhysicalSize);
		}
	else
		{
		fRep = nil;
		}
	}

ZSlotStore::BitSet::~BitSet()
	{
	delete[] fRep;
	}

void ZSlotStore::BitSet::ToStream(const ZStreamW& inStream) const
	{
	inStream.WriteUInt32(fPhysicalSize);
	inStream.WriteUInt32(fCount);
	if (fPhysicalSize)
		{
#if ZCONFIG(Endian, Big)
		inStream.Write(fRep, sizeof(uint32) * fPhysicalSize);
#elif ZCONFIG(Endian, Little)
		uint32* tempRep = new uint32[fPhysicalSize];
		for (size_t x = 0; x < fPhysicalSize; ++x)
			tempRep[x] = ZByteSwap_HostToBig32(fRep[x]);
		inStream.Write(tempRep, sizeof(uint32) * fPhysicalSize);
		delete[] tempRep;
#else // ZCONFIG_Endian
		ZAssertCompile(false);
#endif // ZCONFIG_Endian
		}
	}

void ZSlotStore::BitSet::FromStream(const ZStreamR& inStream)
	{
	uint32 newPhysicalSize = inStream.ReadUInt32();
	fCount = inStream.ReadUInt32();
	if (newPhysicalSize != fPhysicalSize)
		{
		uint32* newRep = nil;
		if (newPhysicalSize > 0)
			newRep = new uint32[newPhysicalSize];
		delete [] fRep;
		fRep = newRep;
		fPhysicalSize = newPhysicalSize;
		}
	if (fPhysicalSize)
		{
		inStream.Read(fRep, sizeof(uint32) * fPhysicalSize);
#if ZCONFIG(Endian, Big)
#elif ZCONFIG(Endian, Little)
		for (size_t x = 0; x < fPhysicalSize; ++x)
			ZByteSwap_BigToHost32(&fRep[x]);
#else // ZCONFIG_Endian
		ZAssertCompile(false);
#endif // ZCONFIG_Endian
		}
	}

size_t ZSlotStore::BitSet::sSizeInStream(uint32 inMaxIndex)
	{
	return sizeof(uint32) + sizeof(uint32) + (inMaxIndex + 7) / 8;
	}

size_t ZSlotStore::BitSet::SizeInStream()
	{
	return sizeof(uint32) + sizeof(uint32) + sizeof(uint32) * fPhysicalSize;
	}

void ZSlotStore::BitSet::Add(uint32 inMemberIndex)
	{
	ZAssertStop(2, inMemberIndex < _Mul32(fPhysicalSize));

	uint32 mask = (1U << (_Mod32(inMemberIndex)));
	uint32 index = _Div32(inMemberIndex);
	if (!(fRep[index] & mask))
		{
		fRep[index] |= mask;
		++fCount;
		}
	}

void ZSlotStore::BitSet::Remove(uint32 inMemberIndex)
	{
	ZAssertStop(2, inMemberIndex < _Mul32(fPhysicalSize));

	uint32 mask = (1U << (_Mod32(inMemberIndex)));
	uint32 index = _Div32(inMemberIndex);
	if (fRep[index] & mask)
		{
		fRep[index] ^= mask;
		--fCount;
		}
	}

bool ZSlotStore::BitSet::Includes(uint32 inMemberIndex) const
	{
	ZAssertStop(2, inMemberIndex < _Mul32(fPhysicalSize));

	uint32 mask = (1U << (_Mod32(inMemberIndex)));
	uint32 index = _Div32(inMemberIndex);
	return (fRep[index] & mask) != 0;
	}

uint32 ZSlotStore::BitSet::SmallestMember() const
	{
	ZAssertStop(2, fCount != 0);

	register size_t index = 0;
	register uint32* p = fRep;
	while (index < fPhysicalSize && *p == 0)
		{
		++index;
		++p;
		}

	uint32 data = *p;
	size_t bitIndex;
	for (bitIndex = 0; bitIndex < 32; ++bitIndex)
		{
		if (data & sBitTable[bitIndex])
			return _Mul32(index) + bitIndex;
		}

// Can't get here
	ZUnimplemented();
	return 0;
	}

uint32 ZSlotStore::BitSet::LargestMember() const
	{
	ZAssertStop(2, fCount != 0);

	register int index = fPhysicalSize - 1;
	register uint32* p = fRep + (fPhysicalSize - 1);
	while (index >= 0 && *p == 0)
		{
		--index;
		--p;
		}
	ZAssertStop(2, index >= 0);

	uint32 data = *p;
	for (int bitIndex = 31; bitIndex >= 0; --bitIndex)
		{
		if (data & sBitTable[bitIndex])
			return _Mul32(index) + bitIndex;
		}

// Can't get here
	ZUnimplemented();
	return 0;
	}

uint32 ZSlotStore::BitSet::SmallestNonMember() const
	{
	if (fCount == 0)
		return 0;

	if (fCount == _Mul32(fPhysicalSize))
		return fCount;

	register size_t index = 0;
	register uint32* p = fRep;
	while (index < fPhysicalSize && *p == uint32(-1))
		{
		++index;
		++p;
		}

	ZAssertStop(2, index < fPhysicalSize);

	uint32 data = ~(*p);
	for (size_t bitIndex = 0; bitIndex < 32; ++bitIndex)
		{
		if (data & sBitTable[bitIndex])
			return _Mul32(index) + bitIndex;
		}

// Can't get here
	ZUnimplemented();
	return 0;
	}

uint32 ZSlotStore::BitSet::SuccessorMember(uint32 inMemberIndex) const
	{
	ZAssertStop(2, fCount != 0);

	uint32 index = _Div32(inMemberIndex);
	if (index >= fPhysicalSize)
		return _Mul32(fPhysicalSize);

// See if next larger element is in the same cell as inMemberIndex
	uint32 data = fRep[index];
	for (size_t bitIndex = _Mod32(inMemberIndex) + 1; bitIndex < 32; ++bitIndex)
		{
		if (data & sBitTable[bitIndex])
			return _Mul32(index) + bitIndex;
		}

// It isn't. Scan the rest of the cells, from index + 1 up
	while (++index < fPhysicalSize)
		{
		if (fRep[index])
			break;
		}
	if (index >= fPhysicalSize)
		return _Mul32(fPhysicalSize);

	data = fRep[index];
	for (size_t bitIndex = 0; bitIndex < 32; ++bitIndex)
		{
		if (data & sBitTable[bitIndex])
			return _Mul32(index) + bitIndex;
		}
// If we ran off the end we'll be returning fPhysicalSize * sizeof(uint32)
	return _Mul32(fPhysicalSize);
	}

bool ZSlotStore::BitSet::IsEmpty() const
	{
	return fCount == 0;
	}

bool ZSlotStore::BitSet::IsUniversal() const
	{
	return fCount == _Mul32(fPhysicalSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZSlotStore

// Notes on the slot store implementation:
// 
// The slot store is viewed as a tree with three levels: the root, its children and its grandchildren. The root has 2**14 = 16384 children,
// among which the the leftmost 2**12 = 4096 are actual slots, and the remaining are bitsets (instances of ZSlotStore::BitSet), called cluster maps.
// At the root are stored two bitsets, each containing 16384 bits: the fFullMap, in which a bit is on if the corresponding child of the root is
// full, and the fNonEmptyMap, in which a bit is on if the corresponding child of the root is not empty. Note that every element in the bitset
// fFullMap also occurs in fNonEmptyMap. The former bitset is useful for fast allocation, while the latter is useful for fast iteration.
 // This tree is stored in pre-order on the disk file.

// 2**kRootWidth is the number of children of the root; thus kRootWidth is the number of bits needed to specify a cluster bitset
static const size_t kRootWidth = 14;
// #bits needed for specifying a slot in a clust
static const size_t kClusterWidth = 16;

// #slots in a cluster
static const size_t kSlotsPerCluster = (1 << kClusterWidth);

// #children of the root that are slots and not clusters.
static const size_t kSpecialSlot = 4096;
// SlotHandle is broken down as follows:
// Bits 31 & 30 == 01 (the constant value ensures every valid slot handle is positive and non-zero)
// Bits 29 - 16 == Cluster #
// Bits 15 - 0 == Slot# in cluster

static const size_t sHeaderSizeInStream = 
			sizeof(uint32)
			+ sizeof(uint32)
			+ sizeof(uint32)
			+ sizeof(uint32)
			+ ZSlotStore::BitSet::sSizeInStream(1 << kRootWidth)
			+ ZSlotStore::BitSet::sSizeInStream(1 << kRootWidth);

static const size_t kClusterMapWidth = ZSlotStore::BitSet::sSizeInStream(kSlotsPerCluster);

// =================================================================================================
#pragma mark -
#pragma mark * Handle manipulation functions

static inline ZSlotStore::SlotHandle sMakeSlotHandle(long inSlotNumber, long inClusterMapIndex)
	{
	return ((uint32)inSlotNumber) |
				(((uint32)inClusterMapIndex) << kClusterWidth) |
				(((uint32)1) << (kRootWidth + kClusterWidth));
	}

static inline long sSlotHandleToSlotNumber(ZSlotStore::SlotHandle inSlotHandle)
	{ return inSlotHandle & (((1UL) << kClusterWidth) - 1); }

static inline long sSlotHandleToClusterMapIndex(ZSlotStore::SlotHandle inSlotHandle)
	{ return (inSlotHandle >> kClusterWidth) & (((1UL) << kRootWidth) - 1); }

static bool sSlotHandleIsValid(ZSlotStore::SlotHandle inHandle, size_t inSlotSize)
	{
// This is the old test, working in the two high bits. However, the generated instructions
// are pretty slow on the 68K, so we shift down into the low byte, and check *its* value.
//	unsigned long hi_bits = handle & (3L << (kClusterWidth + kRootWidth));
//	return hi_bits == (1L << (kClusterWidth + kRootWidth)));

// Check that bit 30 is set, and bit 31 is cleared -- this is our tag for a valid slot handle
	if ((inHandle >> (kClusterWidth + kRootWidth)) != 1)
		return false;

	size_t slotIndex = ::sSlotHandleToSlotNumber(inHandle);
	size_t clusterIndex = ::sSlotHandleToClusterMapIndex(inHandle);
	if (slotIndex < 0 || clusterIndex < 0)
		return false;
// If it's one of our special slots, is it formatted correctly?
	if (clusterIndex < kSpecialSlot)
		return slotIndex == clusterIndex;

	clusterIndex -= kSpecialSlot;

// Finally it's valid if it's in a sensible range
	size_t maxSize = ((1U<<31) - 1);
	maxSize -= sHeaderSizeInStream;
	maxSize -= kSpecialSlot * inSlotSize;
	size_t maxClusterIndex = maxSize / ((inSlotSize * kSlotsPerCluster) + kClusterMapWidth);
	return clusterIndex <= maxClusterIndex;
	}

// These functions are defined here to allow them to be inlined below.

size_t ZSlotStore::ClusterIndexToPhysicalOffset(size_t inIndex)
	{
	size_t offset = sHeaderSizeInStream + fUserHeaderSize;
	if (inIndex < kSpecialSlot)
		offset += inIndex * fSlotSize;
	else
		offset += (kSpecialSlot * fSlotSize) + (inIndex - kSpecialSlot) * (kClusterMapWidth + (kSlotsPerCluster * fSlotSize));
	return offset;
	}

size_t ZSlotStore::SlotHandleToPhysicalOffset(SlotHandle inHandle)
	{
	size_t index = ::sSlotHandleToClusterMapIndex(inHandle);
	size_t offset = this->ClusterIndexToPhysicalOffset(index);
	if (index >= kSpecialSlot)
		offset += fSlotSize * ::sSlotHandleToSlotNumber(inHandle) + kClusterMapWidth;
	return offset;
	}

// =================================================================================================

ZSlotStore::ZSlotStore(const ZStreamRWPos& inStream, size_t inSlotSize, size_t inUserHeaderSize)
:	fStream(inStream),
	fBitSet_Full(nil),
	fBitSet_NonEmpty(nil),
	fAllocatedSlotCount(0),
	fHeaderClean(true),
	fBitSet_CurrentCluster(nil),
	fCurrentClusterIndex(size_t(-1)),
	fCurrentClusterClean(true),
	fSlotSize(inSlotSize),
	fUserHeaderSize(inUserHeaderSize)
	{
	try
		{
		fBitSet_Full = new BitSet(1 << kRootWidth);
		fBitSet_NonEmpty = new BitSet(1 << kRootWidth);
		fBitSet_CurrentCluster = new BitSet(kSlotsPerCluster);
// Truncate the stream to the start of our header area
		fStream.SetSize(fUserHeaderSize);
		fStream.SetPosition(fUserHeaderSize);
		this->WriteHeaderToStream(fStream);
		}
	catch (...)
		{
		delete[] fBitSet_Full;
		delete[] fBitSet_NonEmpty;
		delete[] fBitSet_CurrentCluster;
		}
	}

ZSlotStore::ZSlotStore(const ZStreamRWPos& inStream, size_t inUserHeaderSize)
:	fStream(inStream),
	fBitSet_Full(nil),
	fBitSet_NonEmpty(nil),
	fAllocatedSlotCount(0),
	fHeaderClean(true),
	fBitSet_CurrentCluster(nil),
	fCurrentClusterIndex(size_t(-1)),
	fCurrentClusterClean(true),
	fSlotSize(0),
	fUserHeaderSize(inUserHeaderSize)
	{
	try
		{
		fBitSet_Full = new BitSet(1 << kRootWidth);
		fBitSet_NonEmpty = new BitSet(1 << kRootWidth);
		fBitSet_CurrentCluster = new BitSet(kSlotsPerCluster);

		fStream.SetPosition(fUserHeaderSize);
		if (!this->ReadHeaderFromStream(fStream))
			throw runtime_error("ZSlotStore::ZSlotStore, Bad header format");
		}
	catch (...)
		{
		delete[] fBitSet_Full;
		delete[] fBitSet_NonEmpty;
		delete[] fBitSet_CurrentCluster;
		}
	}

ZSlotStore::~ZSlotStore()
	{
	this->Flush();
	}

// ==================================================

void ZSlotStore::Flush()
	{
	if (!fCurrentClusterClean && fCurrentClusterIndex != size_t(-1))
		{
		size_t offset = this->ClusterIndexToPhysicalOffset(fCurrentClusterIndex);
		fStream.SetPosition(offset);
		fBitSet_CurrentCluster->ToStream(fStream);
		}

	fCurrentClusterClean = true;

	if (!fHeaderClean)
		{
		size_t theRealFileSize = 0;
// Which cluster is the last one actually in use?
		uint32 cluserIndex = fBitSet_NonEmpty->LargestMember();
// If it's one of our special slots, then the file ends at the end of that slot
		if (cluserIndex < kSpecialSlot)
			theRealFileSize = this->ClusterIndexToPhysicalOffset(cluserIndex) + fSlotSize;
		else
			{
// Get the allocation map for that cluster
			this->LoadClusterMap(cluserIndex);
			size_t lastSlot = fBitSet_CurrentCluster->LargestMember();
			theRealFileSize = this->ClusterIndexToPhysicalOffset(cluserIndex) // the start of the cluster map
							+ kClusterMapWidth// plus its width
							+ fSlotSize * lastSlot // plus the slotSize times the slotnumber
							+ fSlotSize; // plus the width of the slot itself
			}
		fStream.SetSize(theRealFileSize);
		fStream.SetPosition(fUserHeaderSize);
		this->WriteHeaderToStream(fStream);
		}
	fHeaderClean = true;
	fStream.Flush();
	}

bool ZSlotStore::SlotHandleIsValid(SlotHandle inHandle)
	{ return ::sSlotHandleIsValid(inHandle, fSlotSize); } 

bool ZSlotStore::SlotHandleIsAllocated(SlotHandle inHandle)
	{
	if (!::sSlotHandleIsValid(inHandle, fSlotSize))
		return false;

	long slotNumber = ::sSlotHandleToSlotNumber(inHandle);
	long clusterIndex = ::sSlotHandleToClusterMapIndex(inHandle);
	if (clusterIndex < kSpecialSlot)
		return fBitSet_Full->Includes(slotNumber);

	this->LoadClusterMap(clusterIndex);
	return fBitSet_CurrentCluster->Includes(slotNumber);
	}

// This is only called by ZBlockStoreInSlotStore::ValidateAndFix().
// This forces potentially legitimate blocks to marked as allocated. This
// helps prevent overzealous truncation of database files during repair. 
// Though perhaps not necessary, this returns true slot is allocated -
// either before this method executed or as a result of this method
// executing. A return value of false indicates that the slot did not
// need to be added to the "bit set". -ec 00.12.06
bool ZSlotStore::MarkSlotHandleAsAllocated(SlotHandle inHandle)
	{
	if (!::sSlotHandleIsValid(inHandle, fSlotSize))
		throw runtime_error("Invalid slot handle");

	long slotNumber = ::sSlotHandleToSlotNumber(inHandle);
	long clusterIndex = ::sSlotHandleToClusterMapIndex(inHandle);
	if (clusterIndex < kSpecialSlot)
		{
		if (!fBitSet_Full->Includes(slotNumber))
			{
			//ZDebugLogf(1, ("/* ZSlotStore::MarkSlotHandleAsAllocated() : inHandle = x%X ; slotNumber = %d */", inHandle, slotNumber));
			fBitSet_Full->Add(slotNumber);
			return true;
			}
		return false;
		}

	this->LoadClusterMap(clusterIndex);
	if (!fBitSet_CurrentCluster->Includes(slotNumber))
		{
		//ZDebugLogf(1, ("/* ZSlotStore::MarkSlotHandleAsAllocated() : inHandle = x%X ; slotNumber = %d */", inHandle, slotNumber));
		fBitSet_CurrentCluster->Add(slotNumber);
		return true;
		}
	return false;
	}

const ZStreamR& ZSlotStore::GetStreamReadyToReadSlot(SlotHandle inSlot)
	{
	if (!::sSlotHandleIsValid(inSlot, fSlotSize))
		throw runtime_error("Invalid slot handle");

	long offset = this->SlotHandleToPhysicalOffset(inSlot);
	if (offset > fStream.GetSize())
		throw runtime_error("Slot handle out of bounds");

	if (!this->SlotHandleIsAllocated(inSlot))
		throw runtime_error("SlotHandle is not allocated");

	ZAssertStop(2, offset >= sHeaderSizeInStream + fUserHeaderSize);

	fStream.SetPosition(offset);
	return fStream;
	}

const ZStreamW& ZSlotStore::GetStreamReadyToWriteSlot(SlotHandle inSlot)
	{
	if (!::sSlotHandleIsValid(inSlot, fSlotSize))
		throw runtime_error("Invalid slot handle");

	long offset = this->SlotHandleToPhysicalOffset(inSlot);
	if (offset > fStream.GetSize())
		throw runtime_error("Slot handle out of bounds");

	if (!this->SlotHandleIsAllocated(inSlot))
		throw runtime_error("SlotHandle is not allocated");

	ZAssertStop(2, offset >= sHeaderSizeInStream + fUserHeaderSize);

	fStream.SetPosition(offset);
	return fStream;
	}

ZSlotStore::SlotHandle ZSlotStore::AllocateSlot()
	{
// Have we got *any* slots available?
	if (fBitSet_Full->IsUniversal())
		return 0;

	uint32 clusterIndex = fBitSet_Full->SmallestNonMember();
	uint32 slotNumber;
	if (clusterIndex < kSpecialSlot)
		{
// We're allocating in the low end of the file
		slotNumber = clusterIndex;
		fBitSet_Full->Add(clusterIndex);
		fBitSet_NonEmpty->Add(clusterIndex);
		}
	else
		{
// We're allocating in the high end
		size_t offset = this->ClusterIndexToPhysicalOffset(clusterIndex);
		if (offset >= fStream.GetSize())
			{
// File too small, create the level-1 map
// Note that we don't actually put this map into our cache -- just
// cause I don't feel like *completely* reworking the original YACL scheme
			BitSet newClusterMap(kSlotsPerCluster);
			newClusterMap.Add(0);
			fStream.SetPosition(offset);
			newClusterMap.ToStream(fStream);
			slotNumber = 0;
			fBitSet_NonEmpty->Add(clusterIndex);
			fCurrentClusterClean = false;
			}
		else
			{
// File large enough, level-1 map exists
			this->LoadClusterMap(clusterIndex);
// Check that the cluster map is not already full
			ZAssertStop(2, !fBitSet_CurrentCluster->IsUniversal());
			slotNumber = fBitSet_CurrentCluster->SmallestNonMember();
			fBitSet_CurrentCluster->Add(slotNumber);
			fCurrentClusterClean = false;
			fBitSet_NonEmpty->Add(clusterIndex);
			if (fBitSet_CurrentCluster->IsUniversal())
				fBitSet_Full->Add(clusterIndex);
			}
		}

	fHeaderClean = false;
	++fAllocatedSlotCount;

	SlotHandle theHandle = ::sMakeSlotHandle(slotNumber, clusterIndex);
	ZAssertStop(2, ::sSlotHandleIsValid(theHandle, fSlotSize));

// AG 97-12-30. Also esnure the stream is large enough to hold the slot we're allocating
	size_t slotOffset = this->SlotHandleToPhysicalOffset(theHandle);
	if (slotOffset >= fStream.GetSize())
		fStream.SetSize(slotOffset + fSlotSize);

	return theHandle;
	}

void ZSlotStore::FreeSlot(SlotHandle inHandle)
	{
	if (!::sSlotHandleIsValid(inHandle, fSlotSize))
		throw runtime_error("Invalid slot handle");
	if (!this->SlotHandleIsAllocated(inHandle))
		throw runtime_error("SlotHandle is not allocated");

	uint32 slotNumber = ::sSlotHandleToSlotNumber(inHandle);
	uint32 clusterIndex = ::sSlotHandleToClusterMapIndex(inHandle);
	size_t newStreamSize = size_t(-1); // Will be changed appropriately if the stream needs to be truncated

	if (clusterIndex < kSpecialSlot)
		{
// The index specifies a slot, not a bitmap
		fBitSet_Full->Remove(clusterIndex);
		fBitSet_NonEmpty->Remove(clusterIndex);
// Was this the last slot?
		if (fBitSet_NonEmpty->IsEmpty())
			newStreamSize = sHeaderSizeInStream;
		else
			{
// Where's the last slot in the file?
			uint32 lastSlotIndex = fBitSet_NonEmpty->LargestMember();
// If the last slot comes before the one we're deleting, then truncate the file
			if (lastSlotIndex < clusterIndex)
				newStreamSize = this->ClusterIndexToPhysicalOffset(lastSlotIndex) + fSlotSize;
			}
		}
	else
		{
// The index specifies a bitmap
		this->LoadClusterMap(clusterIndex);
		size_t streamSize = fStream.GetSize();
// Remove this slot from its cluster map
		fBitSet_CurrentCluster->Remove(slotNumber);
		fCurrentClusterClean = false;
// And make sure our fullMap has the appropriate bit reset
		fBitSet_Full->Remove(clusterIndex);
		size_t endOfSlot = this->SlotHandleToPhysicalOffset(inHandle) + fSlotSize;
// Have we just deleted the last slot from this cluster?
		if (fBitSet_CurrentCluster->IsEmpty())
			{
// So, make sure our nonEmptyMap is updated appropriately
			fBitSet_NonEmpty->Remove(clusterIndex);
// If this was the last cluster in the stream, then shrink the stream
			if (streamSize <= endOfSlot)
				newStreamSize = this->ClusterIndexToPhysicalOffset(clusterIndex);
			}
		else
			{
// If this was the last slot in the stream, then shrink the stream
			if (streamSize <= endOfSlot)
				newStreamSize = endOfSlot - fSlotSize;
			}
		}
// Update our allocation count
	--fAllocatedSlotCount;
	fHeaderClean = false;
// If we came up with a new size, then update the stream's size
	if (newStreamSize != size_t(-1))
		fStream.SetSize(newStreamSize);
	}

void ZSlotStore::ReadSlot(SlotHandle inHandle, void* inBuffer)
	{
	if (!::sSlotHandleIsValid(inHandle, fSlotSize))
		throw runtime_error("Invalid slot handle");
	if (!this->SlotHandleIsAllocated(inHandle))
		throw runtime_error("SlotHandle is not allocated");

	fStream.SetPosition(this->SlotHandleToPhysicalOffset(inHandle));
	fStream.Read(inBuffer, fSlotSize);
	}

void ZSlotStore::WriteSlot(SlotHandle inHandle, const void* inBuffer)
	{
	if (!::sSlotHandleIsValid(inHandle, fSlotSize))
		throw runtime_error("Invalid slot handle");
	if (!this->SlotHandleIsAllocated(inHandle))
		throw runtime_error("SlotHandle is not allocated");

	fStream.SetPosition(this->SlotHandleToPhysicalOffset(inHandle));
	fStream.Write(inBuffer, fSlotSize);
	}

ZSlotStore::SlotHandle ZSlotStore::GetFirstSlotHandle()
	{
	if (fAllocatedSlotCount == 0)
		return kInvalidHandle;

	uint32 clusterIndex = fBitSet_NonEmpty->SmallestMember();
	if (clusterIndex < kSpecialSlot)
		return ::sMakeSlotHandle(clusterIndex, clusterIndex);
	this->LoadClusterMap(clusterIndex);
	return ::sMakeSlotHandle(fBitSet_CurrentCluster->SmallestMember(), clusterIndex);
	}

ZSlotStore::SlotHandle ZSlotStore::GetNextSlotHandle(SlotHandle inHandle)
	{
// Get the slot number *within* the cluster
	uint32 slotNumber = ::sSlotHandleToSlotNumber(inHandle);
// And the index number of the cluster itself (which for real clusters starts at 4096)
	uint32 clusterIndex = ::sSlotHandleToClusterMapIndex(inHandle);
// Note that if the slot handle is numbered between 0 and 4095, both slotNumber and index will have the same value

	size_t offset = this->ClusterIndexToPhysicalOffset(clusterIndex);
// Sanity check
	if (offset >= fStream.GetSize())
		return kInvalidHandle;

	if (clusterIndex >= kSpecialSlot)
		{
// The given handle is that of a slot in some cluster; read its map
		this->LoadClusterMap(clusterIndex);
		uint32 slotNumber2 = fBitSet_CurrentCluster->SuccessorMember(slotNumber);
		if (slotNumber2 < kSlotsPerCluster)
			return ::sMakeSlotHandle(slotNumber2, clusterIndex);
// No more allocated slots in the cluster; fall through...
		}
	uint32 nextAllocatedIndex = fBitSet_NonEmpty->SuccessorMember(clusterIndex);
	if (nextAllocatedIndex >= (1 << kRootWidth))
		return kInvalidHandle;
	if (nextAllocatedIndex < kSpecialSlot)
		return ::sMakeSlotHandle(nextAllocatedIndex, nextAllocatedIndex);
	this->LoadClusterMap(nextAllocatedIndex);
	return ::sMakeSlotHandle(fBitSet_CurrentCluster->SmallestMember(), nextAllocatedIndex);
	}

// ==================================================

void ZSlotStore::LoadClusterMap(uint32 inIndex)
	{
	if (inIndex == fCurrentClusterIndex)
		return;

// Write back the cached cluster map, if it's for real
	if (fCurrentClusterIndex != uint32(-1))
		{
		if (!fCurrentClusterClean)
			{
			fStream.SetPosition(this->ClusterIndexToPhysicalOffset(fCurrentClusterIndex));
			fBitSet_CurrentCluster->ToStream(fStream);
			}
		}

	fStream.SetPosition(this->ClusterIndexToPhysicalOffset(inIndex));
	fBitSet_CurrentCluster->FromStream(fStream);
	fCurrentClusterIndex = inIndex;
	fCurrentClusterClean = true;
	}

static const unsigned long sSlotMagicNumber = 0x534C5421; // 'SLT!'

void ZSlotStore::WriteHeaderToStream(const ZStreamW& inStream)
	{
	inStream.WriteUInt32(sSlotMagicNumber);
	inStream.WriteUInt32(fAllocatedSlotCount);
	inStream.WriteUInt32(fSlotSize);
	inStream.WriteUInt32(0);
	fBitSet_Full->ToStream(inStream);
	fBitSet_NonEmpty->ToStream(inStream);
	}

bool ZSlotStore::ReadHeaderFromStream(const ZStreamR& inStream)
	{
// Check for our flag word at the beginning
	uint32 theFlagLong = inStream.ReadUInt32();
	if (theFlagLong != sSlotMagicNumber)
		return false;
	fAllocatedSlotCount = inStream.ReadUInt32();
	fSlotSize = inStream.ReadUInt32();
// Read and ignore the spare reserved word, that may allow us to expand later without breaking things
	long dummy = inStream.ReadUInt32();
	fBitSet_Full->FromStream(inStream);
	fBitSet_NonEmpty->FromStream(inStream);
	return true;
	}
