static const char rcsid[] = "@(#) $Id: ZBlockStore_SlotStore.cpp,v 1.14 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZBlockStore_SlotStore.h"
#include "ZByteSwap.h"
#include "ZMemory.h"
#include "ZProgressWatcher.h"
#include "ZStream.h"
#include "ZString.h"

#include "ZCompat_algorithm.h"
#include <set>

using std::min;
using std::set;
using std::runtime_error;
using std::string;
using std::vector;

// ==================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_BlockStore 2

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::Header

class ZBlockStore_SlotStore::Header
	{
public:
	Header();

	static size_t sSizeInStream();

	bool ReadFrom(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandle);
	void WriteTo(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandle);

	void SetHeadPrimary(ZBlockStore::BlockID inNewHeadPrimary);
	ZBlockStore::BlockID GetHeadPrimary() const;

	void SetTailPrimary(ZBlockStore::BlockID inNewTailPrimary);
	ZBlockStore::BlockID GetTailPrimary() const;

	void SetAllocatedBlockCount(size_t inCount);
	size_t GetAllocatedBlockCount() const;

	void SetRefCon(int32 inRefCon);
	int32 GetRefCon() const;

	bool IsClean() const { return fIsClean; }
	void MarkDirty() { fIsClean = false; }
	void MarkClean() { fIsClean = true; }

private:
	ZSlotStore::SlotHandle fHeadPrimary;
	ZSlotStore::SlotHandle fTailPrimary;
	size_t fAllocatedBlockCount;
	int32 fRefCon;
	bool fIsClean;
	};

ZBlockStore_SlotStore::Header::Header()
	{
	fHeadPrimary = 0;
	fTailPrimary = 0;
	fAllocatedBlockCount = 0;
	fRefCon = 0;
	fIsClean = true;
	}

size_t ZBlockStore_SlotStore::Header::sSizeInStream()
	{
	return sizeof(uint32) + // The flag long
		sizeof(ZSlotStore::SlotHandle) + // fHeadPrimary
		sizeof(ZSlotStore::SlotHandle) + // fTailPrimary
		sizeof(size_t) + // fAllocatedBlockCount
		sizeof(size_t) + // fRefCon
		sizeof(size_t); // "Reserved" long
	}

static const uint32 sBlockHeaderFlag = 0x424C4B21; // 'BLK!'

bool ZBlockStore_SlotStore::Header::ReadFrom(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandle)
	{
	const ZStreamR& theStream= inSlotStore->GetStreamReadyToReadSlot(inSlotHandle);

	uint32 theFlagLong = theStream.ReadUInt32();
	if (theFlagLong != sBlockHeaderFlag)
		return false;

	fHeadPrimary = theStream.ReadUInt32();
	fTailPrimary = theStream.ReadUInt32();
	fAllocatedBlockCount = theStream.ReadUInt32();
	fRefCon = theStream.ReadInt32();
	int32 dummy = theStream.ReadInt32(); // read and ignore the "reserved" long
	return true;
	}

void ZBlockStore_SlotStore::Header::WriteTo(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandle)
	{
	const ZStreamW& theStream = inSlotStore->GetStreamReadyToWriteSlot(inSlotHandle);
	theStream.WriteUInt32(sBlockHeaderFlag); // Write the sanity checking flag long

	theStream.WriteUInt32(fHeadPrimary);
	theStream.WriteUInt32(fTailPrimary);
	theStream.WriteUInt32(fAllocatedBlockCount);
	theStream.WriteInt32(fRefCon);
	theStream.WriteInt32(0); // Write the "Reserved" long (should be zero for now)

// Write out dummy info to fill up the slot we occupy, so the stream can not end up
// being too short for a later read or write.
	char dummy[1024];
	size_t countRemaining = inSlotStore->GetSlotSize() - sSizeInStream();
	while (countRemaining > 0)
		{
		size_t countWritten;
		theStream.Write(dummy, min(countRemaining, size_t(1024)), &countWritten);
		countRemaining -= countWritten;
		}
	this->MarkClean();
	}

void ZBlockStore_SlotStore::Header::SetHeadPrimary(ZBlockStore::BlockID inNewHeadPrimary)
	{
	if (fHeadPrimary != inNewHeadPrimary)
		{
		fHeadPrimary = inNewHeadPrimary;
		this->MarkDirty();
		}
	}

ZBlockStore::BlockID ZBlockStore_SlotStore::Header::GetHeadPrimary() const
	{ return fHeadPrimary; }

void ZBlockStore_SlotStore::Header::SetTailPrimary(ZBlockStore::BlockID inNewTailPrimary)
	{
	if (fTailPrimary != inNewTailPrimary)
		{
		fTailPrimary = inNewTailPrimary;
		this->MarkDirty();
		}
	}

ZBlockStore::BlockID ZBlockStore_SlotStore::Header::GetTailPrimary() const
	{ return fTailPrimary; }

void ZBlockStore_SlotStore::Header::SetAllocatedBlockCount(size_t inCount)
	{
	if (fAllocatedBlockCount != inCount)
		{
		fAllocatedBlockCount = inCount;
		this->MarkDirty();
		}
	}

size_t ZBlockStore_SlotStore::Header::GetAllocatedBlockCount() const
	{ return fAllocatedBlockCount; }

void ZBlockStore_SlotStore::Header::SetRefCon(int32 theRefCon)
	{
	if (fRefCon != theRefCon)
		{
		fRefCon = theRefCon;
		this->MarkDirty();
		}
	}

int32 ZBlockStore_SlotStore::Header::GetRefCon() const
	{ return fRefCon; }

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::Slot

class ZBlockStore_SlotStore::Slot
	{
protected:
	Slot() : fIsClean(true), fSlotHandle(0) {}
	~Slot() { ZAssertStop(2, fIsClean); }

	void MarkClean() { fIsClean = true; }
	void MarkDirty() { fIsClean = false; }

public:
	void SetSlotHandle(ZSlotStore::SlotHandle inNewSlotHandle);

	bool IsClean() const { return fIsClean; }

protected:
	bool fIsClean;
	ZSlotStore::SlotHandle fSlotHandle;
	};

inline void ZBlockStore_SlotStore::Slot::SetSlotHandle(ZSlotStore::SlotHandle inNewSlotHandle)
	{
	fSlotHandle = inNewSlotHandle;
	fIsClean = false;
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::PrimarySlot

class ZBlockStore_SlotStore::PrimarySlot : public ZBlockStore_SlotStore::Slot
	{
public:
	static size_t sGetHeaderSize();
	static size_t sGetContentSize(size_t inSlotSize);
	void* operator new(size_t inSize, size_t inSlotSize);

	PrimarySlot();

	ZSlotStore::SlotHandle GetSlotHandle() const { return fSlotHandle; }

	void ReadFrom(ZSlotStore* inSlotStore);
	void WriteTo(ZSlotStore* inSlotStore);

	void SetPreviousPrimary(ZSlotStore::SlotHandle inewPreviousPrimary);
	ZSlotStore::SlotHandle GetPreviousPrimary() const;

	void SetNextPrimary(ZSlotStore::SlotHandle inNewNextPrimary);
	ZSlotStore::SlotHandle GetNextPrimary() const;

	void SetHeadSecondary(ZSlotStore::SlotHandle inNewHeadSecondary);
	ZSlotStore::SlotHandle GetHeadSecondary() const;

	void SetTailSecondary(ZSlotStore::SlotHandle inNewTailSecondary);
	ZSlotStore::SlotHandle GetTailSecondary() const;

	void SetBlockSize(size_t inNewBlockSize);
	size_t GetBlockSize() const;

	void CopyContentsTo(void* inDest, size_t inSourceOffset, size_t inAmountToMove) const;
	void CopyContentsFrom(const void* inSource, size_t inSourceOffset, size_t inAmountToMove);

	void ResetAttributes();
	void ZeroContents(size_t inSlotSize);

	void DeleteYourSlot(ZSlotStore* inSlotStore);

private:
	ZSlotStore::SlotHandle fPreviousPrimary;
	ZSlotStore::SlotHandle fNextPrimary;
	ZSlotStore::SlotHandle fHeadSecondary;
	ZSlotStore::SlotHandle fTailSecondary;
	uint32 fBlockSize;
	uint8 fData[1];
	};

inline size_t ZBlockStore_SlotStore::PrimarySlot::sGetHeaderSize()
	{ return 4 * sizeof(ZSlotStore::SlotHandle) + sizeof(uint32); }

inline size_t ZBlockStore_SlotStore::PrimarySlot::sGetContentSize(size_t inSlotSize)
	{ return inSlotSize - sGetHeaderSize(); }

void* ZBlockStore_SlotStore::PrimarySlot::operator new(size_t inSize, size_t inSlotSize)
	{
	return ::operator new(inSize + inSlotSize - sGetHeaderSize());
	}

ZBlockStore_SlotStore::PrimarySlot::PrimarySlot()
	{ this->ResetAttributes(); }

/* AG 97-03-26. A note on byte swapping issues.
We don't know whether our underlying stream does buffered I/O or not, so
it may be *extremely* inefficient to use the ZStream read/write routines
to do our byte swapping field by field. One of the goals of ZBlockStore_SlotStore
is to do accesses in fixed sized chunks, hopefully corresponding to the size of
the underlying storage medium's blocks. So, we do our swaps on the
local copies of our data. Writing out is a bit of a pain, because we have to
swap to the canonical format, write out all our data in one hit, then
swap the local copies back again. Still, it reduces the number of calls
to the stream code (and hence disk-hits) from 6 to 1 in the case of
a primary slot.
Another thing to remember is that the data that is stored in the contents
of one of our slots has *already* been swapped to the canonical format (big-endian,
the only form for right-thinking people).
*/

void ZBlockStore_SlotStore::PrimarySlot::ReadFrom(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	const ZStreamR& theStream = inSlotStore->GetStreamReadyToReadSlot(fSlotHandle);
	theStream.Read(&fPreviousPrimary, inSlotStore->GetSlotSize());

// We've pulled in the data, now swap it in place
	::ZByteSwap_HostToBig32(&fPreviousPrimary);
	::ZByteSwap_BigToHost32(&fNextPrimary);
	::ZByteSwap_BigToHost32(&fHeadSecondary);
	::ZByteSwap_BigToHost32(&fTailSecondary);
	::ZByteSwap_BigToHost32(&fBlockSize);

	this->MarkClean();
	}

void ZBlockStore_SlotStore::PrimarySlot::WriteTo(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	const ZStreamW& theStream = inSlotStore->GetStreamReadyToWriteSlot(fSlotHandle);

// We're about to write out our data, so swap it in place
	::ZByteSwap_HostToBig32(&fPreviousPrimary);
	::ZByteSwap_HostToBig32(&fNextPrimary);
	::ZByteSwap_HostToBig32(&fHeadSecondary);
	::ZByteSwap_HostToBig32(&fTailSecondary);
	::ZByteSwap_HostToBig32(&fBlockSize);

	theStream.Write(&fPreviousPrimary, inSlotStore->GetSlotSize());

// We've written it out, so swap it back
	::ZByteSwap_BigToHost32(&fPreviousPrimary);
	::ZByteSwap_BigToHost32(&fNextPrimary);
	::ZByteSwap_BigToHost32(&fHeadSecondary);
	::ZByteSwap_BigToHost32(&fTailSecondary);
	::ZByteSwap_BigToHost32(&fBlockSize);

	this->MarkClean();
	}

void ZBlockStore_SlotStore::PrimarySlot::SetPreviousPrimary(ZSlotStore::SlotHandle inNewPreviousPrimary)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fPreviousPrimary != inNewPreviousPrimary)
		{
		fPreviousPrimary = inNewPreviousPrimary;
		this->MarkDirty();
		}
	}

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::PrimarySlot::GetPreviousPrimary() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fPreviousPrimary;
	}

void ZBlockStore_SlotStore::PrimarySlot::SetNextPrimary(ZSlotStore::SlotHandle inNewNextPrimary)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fNextPrimary != inNewNextPrimary)
		{
		fNextPrimary = inNewNextPrimary;
		this->MarkDirty();
		}
	}

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::PrimarySlot::GetNextPrimary() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fNextPrimary;
	}

void ZBlockStore_SlotStore::PrimarySlot::SetHeadSecondary(ZSlotStore::SlotHandle inNewHeadSecondary)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fHeadSecondary != inNewHeadSecondary)
		{
		fHeadSecondary = inNewHeadSecondary;
		this->MarkDirty();
		}
	}

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::PrimarySlot::GetHeadSecondary() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fHeadSecondary;
	}

inline void ZBlockStore_SlotStore::PrimarySlot::SetTailSecondary(ZSlotStore::SlotHandle inNewTailSecondary)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fTailSecondary != inNewTailSecondary)
		{
		fTailSecondary = inNewTailSecondary;
		this->MarkDirty();
		}
	}
inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::PrimarySlot::GetTailSecondary() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fTailSecondary;
	}

inline void ZBlockStore_SlotStore::PrimarySlot::SetBlockSize(size_t inNewBlockSize)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fBlockSize != inNewBlockSize)
		{
		fBlockSize = inNewBlockSize;
		this->MarkDirty();
		}
	}
inline size_t ZBlockStore_SlotStore::PrimarySlot::GetBlockSize() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fBlockSize;
	}

inline void ZBlockStore_SlotStore::PrimarySlot::CopyContentsTo(void* inDest, size_t inSourceOffset, size_t inAmountToMove) const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	ZBlockMove(fData + inSourceOffset, inDest, inAmountToMove);
	}

inline void ZBlockStore_SlotStore::PrimarySlot::CopyContentsFrom(const void* inSource, size_t inSourceOffset, size_t inAmountToMove)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	ZBlockMove(inSource, fData + inSourceOffset, inAmountToMove);
	this->MarkDirty();
	}

void ZBlockStore_SlotStore::PrimarySlot::ResetAttributes()
	{
	fSlotHandle = 0;
	this->MarkClean();
	fPreviousPrimary = 0;
	fNextPrimary = 0;
	fHeadSecondary = 0;
	fTailSecondary = 0;
	fBlockSize = 0;
	}

void ZBlockStore_SlotStore::PrimarySlot::ZeroContents(size_t inSlotSize)
	{
	ZBlockSet(fData, inSlotSize - sGetHeaderSize(), 0);
	}

void ZBlockStore_SlotStore::PrimarySlot::DeleteYourSlot(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	inSlotStore->FreeSlot(fSlotHandle);
	this->ResetAttributes();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::SecondarySlot

class ZBlockStore_SlotStore::SecondarySlot : public ZBlockStore_SlotStore::Slot
	{
public:
	static size_t sGetHeaderSize();
	static size_t sGetContentSize(size_t inSlotSize);

	void* operator new(size_t inSize, size_t inSlotSize);

	SecondarySlot();

	ZSlotStore::SlotHandle GetSlotHandle() const { return fSlotHandle; }

	void ReadFrom(ZSlotStore* inSlotStore);
	void WriteTo(ZSlotStore* inSlotStore);

	void SetPreviousSlot(ZSlotStore::SlotHandle newPreviousSlot);
	ZSlotStore::SlotHandle GetPreviousSlot() const;

	void SetNextSlot(ZSlotStore::SlotHandle newNextSlot);
	ZSlotStore::SlotHandle GetNextSlot() const;

	void CopyContentsTo(void* inDest, size_t inSourceOffset, size_t inAmountToMove) const;
	void CopyContentsFrom(const void* inSource, size_t inSourceOffset, size_t inAmountToMove);

	void ResetAttributes();
	void ZeroContents(size_t slotSize);

	void DeleteYourSlot(ZSlotStore* inSlotStore);

private:
	ZSlotStore::SlotHandle fPreviousSlot;
	ZSlotStore::SlotHandle fNextSlot;
	unsigned char fData[1];
	};

inline size_t ZBlockStore_SlotStore::SecondarySlot::sGetHeaderSize()
	{ return 2 * sizeof(ZSlotStore::SlotHandle);}

inline size_t ZBlockStore_SlotStore::SecondarySlot::sGetContentSize(size_t slotSize)
	{ return slotSize - sGetHeaderSize(); }

void* ZBlockStore_SlotStore::SecondarySlot::operator new(size_t inSize, size_t inSlotSize)
	{ return ::operator new(inSize + inSlotSize - sGetHeaderSize());}

ZBlockStore_SlotStore::SecondarySlot::SecondarySlot()
	{ this->ResetAttributes(); }

void ZBlockStore_SlotStore::SecondarySlot::ReadFrom(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	const ZStreamR& theStream = inSlotStore->GetStreamReadyToReadSlot(fSlotHandle);

	theStream.Read(&fPreviousSlot, inSlotStore->GetSlotSize());

// We've pulled in the data, now swap it in place
	::ZByteSwap_BigToHost32(&fPreviousSlot);
	::ZByteSwap_BigToHost32(&fNextSlot);

	this->MarkClean();
	}

void ZBlockStore_SlotStore::SecondarySlot::WriteTo(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	const ZStreamW& theStream = inSlotStore->GetStreamReadyToWriteSlot(fSlotHandle);

// We're about to write out our data, so swap it in place
	::ZByteSwap_HostToBig32(&fPreviousSlot);
	::ZByteSwap_HostToBig32(&fNextSlot);

	theStream.Write(&fPreviousSlot, inSlotStore->GetSlotSize());

// We've written it out, so swap it back
	::ZByteSwap_BigToHost32(&fPreviousSlot);
	::ZByteSwap_BigToHost32(&fNextSlot);

	this->MarkClean();
	}

inline void ZBlockStore_SlotStore::SecondarySlot::SetPreviousSlot(ZSlotStore::SlotHandle inNewPreviousSlot)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fPreviousSlot != inNewPreviousSlot)
		{
		fPreviousSlot = inNewPreviousSlot;
		this->MarkDirty();
		}
	}

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::SecondarySlot::GetPreviousSlot() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fPreviousSlot;
	}

inline void ZBlockStore_SlotStore::SecondarySlot::SetNextSlot(ZSlotStore::SlotHandle inNewNextSlot)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	if (fNextSlot != inNewNextSlot)
		{
		fNextSlot = inNewNextSlot;
		this->MarkDirty();
		}
	}
inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::SecondarySlot::GetNextSlot() const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	return fNextSlot;
	}

inline void ZBlockStore_SlotStore::SecondarySlot::CopyContentsTo(void* inDest, size_t inSourceOffset, size_t inAmountToMove) const
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	ZBlockMove(fData + inSourceOffset, inDest, inAmountToMove);
	}

inline void ZBlockStore_SlotStore::SecondarySlot::CopyContentsFrom(const void* inSource, size_t inSourceOffset, size_t inAmountToMove)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	ZBlockMove(inSource, fData + inSourceOffset, inAmountToMove);
	this->MarkDirty();
	}

inline void ZBlockStore_SlotStore::SecondarySlot::ResetAttributes()
	{
	this->MarkClean();
	fPreviousSlot = 0;
	fNextSlot = 0;
	fSlotHandle = 0;
	}

void ZBlockStore_SlotStore::SecondarySlot::ZeroContents(size_t inSlotSize)
	{
	ZBlockSet(fData, inSlotSize - sGetHeaderSize(), 0);
	}

void ZBlockStore_SlotStore::SecondarySlot::DeleteYourSlot(ZSlotStore* inSlotStore)
	{
	ZAssertStop(kDebug_BlockStore, fSlotHandle != 0);
	inSlotStore->FreeSlot(fSlotHandle);
	this->ResetAttributes();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::Streamer Declaration

class ZBlockStore_SlotStore::Stream : public ZStreamRWPos
	{
public:
	Stream(ZBlockStore_SlotStore* inBlockStore, ZBlockStore::BlockID inBlockID);
	virtual ~Stream();

// From ZStreamR via ZStreamRWPos
	virtual void Imp_Read(void* inDest, size_t inCount, size_t* outCountRead);

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten);
	virtual void Imp_Flush();

// From ZStreamRPos/ZStreamWPos via ZStreamRWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 inPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 inSize);

private:
	ZBlockStore_SlotStore* fBlockStore;
	ZBlockStore::BlockID fBlockID;
	ZSlotStore::SlotHandle fCurrentSecondary;
	size_t fPosition;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::Streamer

class ZBlockStore_SlotStore::Streamer : public ZStreamerRWPos
	{
public:
	Streamer(ZBlockStore_SlotStore* inBlockStore, ZBlockStore::BlockID inBlockID);
	virtual ~Streamer();

// From ZStreamerR via ZStreamerRWPos
	virtual const ZStreamR& GetStreamR();

// From ZStreamerW via ZStreamerRWPos
	virtual const ZStreamW& GetStreamW();

// From ZStreamerRWPos
	virtual const ZStreamRWPos& GetStreamRWPos();

protected:
	ZBlockStore_SlotStore::Stream fStream;
	};

ZBlockStore_SlotStore::Streamer::Streamer(ZBlockStore_SlotStore* inBlockStore, ZBlockStore::BlockID inBlockID)
:	fStream(inBlockStore, inBlockID)
	{}

ZBlockStore_SlotStore::Streamer::~Streamer()
	{}

const ZStreamR& ZBlockStore_SlotStore::Streamer::GetStreamR()
	{ return fStream; }

const ZStreamW& ZBlockStore_SlotStore::Streamer::GetStreamW()
	{ return fStream; }

const ZStreamRWPos& ZBlockStore_SlotStore::Streamer::GetStreamRWPos()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore

// These routine are called *a lot*, so they're broken into an inline check	with a
// call to the real routine if the cache is not current

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::LoadPrimarySlot(ZSlotStore::SlotHandle inSlotHandle)
	{
	ZAssertStop(kDebug_BlockStore, inSlotHandle != 0);
	return (inSlotHandle == fPrimarySlot->GetSlotHandle()) ? (inSlotHandle) : this->Internal_LoadPrimarySlot(inSlotHandle);
	}

inline ZSlotStore::SlotHandle ZBlockStore_SlotStore::LoadSecondarySlot(ZSlotStore::SlotHandle inSlotHandle)
	{
	ZAssertStop(kDebug_BlockStore, inSlotHandle != 0);
	return (inSlotHandle == fSecondarySlot->GetSlotHandle()) ? (inSlotHandle) : this->Internal_LoadSecondarySlot(inSlotHandle);
	}


// ==================================================
// ==================================================
// Open an existing block file, stored in theSlotStore, and with its header stored in
// the slot with the handle slotHandleOfHeader

ZBlockStore_SlotStore::ZBlockStore_SlotStore(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandleOfHeader)
:	fMutex("ZBlockStore_SlotStore::fMutex"),
	fSlotStore(inSlotStore)
	{
	fHeader = new Header;

	fPrimarySlot = new(fSlotStore->GetSlotSize()) PrimarySlot;
	fSecondarySlot = new(fSlotStore->GetSlotSize()) SecondarySlot;

	fSlotHandleOfHeader = inSlotHandleOfHeader;
	if (!fHeader->ReadFrom(fSlotStore, fSlotHandleOfHeader))
		throw runtime_error("ZBlockStore_SlotStore, bad header");
	}

// Create a new block file in theSlotStore. Create a new header and put it in a slot in
// the slotfile, which can later be retrieved by someone calling our
// GetSlotHandleOfHeader method
ZBlockStore_SlotStore::ZBlockStore_SlotStore(ZSlotStore* inSlotStore)
:	fMutex("ZBlockStore_SlotStore::fMutex"),
	fSlotStore(inSlotStore)
	{
	fHeader = new Header;
	fPrimarySlot = new(fSlotStore->GetSlotSize()) PrimarySlot;
	fSecondarySlot = new(fSlotStore->GetSlotSize()) SecondarySlot;

// Ensure that the slot size is enough for our header info. If it isn't, we've got a slot file with
// a bizarrely small slot size (ie < 24 bytes currently).
	ZAssertStop(kDebug_BlockStore, fSlotStore->GetSlotSize() >= Header::sSizeInStream());

	fSlotHandleOfHeader = fSlotStore->AllocateSlot();
	fHeader->WriteTo(fSlotStore, fSlotHandleOfHeader);
	}

ZBlockStore_SlotStore::~ZBlockStore_SlotStore()
	{
	this->Flush();
	delete fHeader;
	delete fPrimarySlot;
	delete fSecondarySlot;
	}

void ZBlockStore_SlotStore::Flush()
	{
	ZMutexLocker locker(fMutex);
// Write out our cached primary and secondary blocks
	if (!fPrimarySlot->IsClean())
		fPrimarySlot->WriteTo(fSlotStore);
	if (!fSecondarySlot->IsClean())
		fSecondarySlot->WriteTo(fSlotStore);
	if (!fHeader->IsClean())
		fHeader->WriteTo(fSlotStore, fSlotHandleOfHeader);
	fSlotStore->Flush();
	}

ZRef<ZStreamerRWPos> ZBlockStore_SlotStore::Create(BlockID& oBlockID)
	{
	ZMutexLocker locker(fMutex);
// We create blocks at the tail of the list of all primary slots
	ZSlotStore::SlotHandle newPrimaryHandle = fSlotStore->AllocateSlot();
	if (fHeader->GetTailPrimary() != 0)
		{
		this->LoadPrimarySlot(fHeader->GetTailPrimary());
		fPrimarySlot->SetNextPrimary(newPrimaryHandle);
		}
	this->CreatePrimarySlot(newPrimaryHandle);
	fPrimarySlot->SetPreviousPrimary(fHeader->GetTailPrimary());
	fHeader->SetTailPrimary(newPrimaryHandle);
	if (fHeader->GetHeadPrimary() == 0)
		fHeader->SetHeadPrimary(newPrimaryHandle);

	fHeader->SetAllocatedBlockCount(fHeader->GetAllocatedBlockCount() + 1);
	oBlockID = newPrimaryHandle;
	return new Streamer(this, newPrimaryHandle);
	}

ZRef<ZStreamerRWPos> ZBlockStore_SlotStore::OpenRWPos(BlockID iBlockID)
	{
	if (!fSlotStore->SlotHandleIsAllocated(iBlockID))
		return ZRef<ZStreamerRWPos>();

	return new Streamer(this, iBlockID);
	}

bool ZBlockStore_SlotStore::Delete(BlockID iBlockID)
	{
	if (iBlockID == 0)
		return false;

	ZMutexLocker locker(fMutex);
	if (!fSlotStore->SlotHandleIsAllocated(iBlockID))
		return false;

	this->LoadPrimarySlot(iBlockID);
// Walk the list of secondary slots, from tail to beginning, deleting as we go
	ZSlotStore::SlotHandle theSecondaryHandle = fPrimarySlot->GetTailSecondary();
	while (theSecondaryHandle != 0)
		{
		this->LoadSecondarySlot(theSecondaryHandle);
		theSecondaryHandle = fSecondarySlot->GetPreviousSlot();
		fSecondarySlot->DeleteYourSlot(fSlotStore);
		}
// Find the handles of the primary slots before and after this one
	ZSlotStore::SlotHandle previousPrimary = fPrimarySlot->GetPreviousPrimary();
	ZSlotStore::SlotHandle nextPrimary = fPrimarySlot->GetNextPrimary();
	fPrimarySlot->DeleteYourSlot(fSlotStore);
// And update their next and previous pointers to reference each other
	if (previousPrimary != 0)
		{
		this->LoadPrimarySlot(previousPrimary);
		fPrimarySlot->SetNextPrimary(nextPrimary);
		}
	if (nextPrimary != 0)
		{
		this->LoadPrimarySlot(nextPrimary);
		fPrimarySlot->SetPreviousPrimary(previousPrimary);
		}
// Fix up the header, in case this primary was at the head and/or tail of the list of primaries
	if (fHeader->GetHeadPrimary() == iBlockID)
		fHeader->SetHeadPrimary(nextPrimary);
	if (fHeader->GetTailPrimary() == iBlockID)
		fHeader->SetTailPrimary(previousPrimary);
	fHeader->SetAllocatedBlockCount(fHeader->GetAllocatedBlockCount() - 1);
	return true;
	}

size_t ZBlockStore_SlotStore::EfficientSize()
	{ return PrimarySlot::sGetContentSize(fSlotStore->GetSlotSize()); }

size_t ZBlockStore_SlotStore::GetBlockCount()
	{ return fHeader->GetAllocatedBlockCount(); }

ZBlockStore::BlockID ZBlockStore_SlotStore::GetFirstBlockID()
	{ return fHeader->GetHeadPrimary(); }

ZBlockStore::BlockID ZBlockStore_SlotStore::GetLastBlockID()
	{ return fHeader->GetTailPrimary(); }

ZBlockStore::BlockID ZBlockStore_SlotStore::GetNextBlockID(BlockID inBlockID)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_BlockStore, inBlockID != 0);
// Get the primary slot for this BlockID
	this->LoadPrimarySlot(inBlockID);
	return fPrimarySlot->GetNextPrimary();
	}

ZBlockStore::BlockID ZBlockStore_SlotStore::GetPreviousBlockID(BlockID inBlockID)
	{
	ZMutexLocker locker(fMutex);
	ZAssertStop(kDebug_BlockStore, inBlockID != 0);

// Get the primary slot for this BlockID
	this->LoadPrimarySlot(inBlockID);
	return fPrimarySlot->GetPreviousPrimary();
	}

string ZBlockStore_SlotStore::sBlockIDAsString(ZBlockStore::BlockID inHandle)
	{
	uint32 temp = inHandle & 0x3FFFFFFF;
	uint32 hiPart = temp >> 16;
	uint32 loPart = temp & 0xFFFF;
	uint32 realSlotNumber = loPart;
	if (hiPart >= 4096)
		realSlotNumber += (hiPart - 4095) << 16;

	return ZString::sFromInt(realSlotNumber);
	}

int32 ZBlockStore_SlotStore::GetRefCon()
	{ return fHeader->GetRefCon(); }

void ZBlockStore_SlotStore::SetRefCon(int32 theNewRefCon)
	{ fHeader->SetRefCon(theNewRefCon); }

ZSlotStore::SlotHandle ZBlockStore_SlotStore::GetSlotHandleOfHeader()
	{ return fSlotHandleOfHeader; }

#define ZCONFIG_TrackLostHandles 1 //## -ec 00.12.08

void ZBlockStore_SlotStore::ValidateAndFix(ZProgressWatcher* iProgressWatcher)
	{
	ZMutexLocker locker(fMutex);

	ZProgressPusher pusher(iProgressWatcher, "Checking low level format", 5);

	long countOfBlocks = fHeader->GetAllocatedBlockCount();
	BlockID theFirstHandle = fHeader->GetHeadPrimary();
	BlockID theLastHandle = fHeader->GetTailPrimary();

	// First we walk from head to tail building a list of all blocks, then we walk
	// from tail to head. In both cases we make sure the linked list is correct.

	set<BlockID> forwardSet;
	bool forwardHasProblems = false;
	BlockID currentHandle = theFirstHandle;
	{ // Code block to scope previousHandle and innerPusher
	ZProgressPusher innerPusher(iProgressWatcher, "forward pass", countOfBlocks);
	BlockID previousHandle = 0;
	while (currentHandle != 0)
		{
		innerPusher.Process(1);
		try
			{
			if (ZCONFIG_TrackLostHandles && currentHandle != fPrimarySlot->GetSlotHandle())
				fSlotStore->MarkSlotHandleAsAllocated(currentHandle);

			this->LoadPrimarySlot(currentHandle);

			// Check that this blocks previous points to the last block we looked at
			if (fPrimarySlot->GetPreviousPrimary() != previousHandle)
				forwardHasProblems = true;

			// Check that this block's next is a valid handle
			BlockID nextHandle = fPrimarySlot->GetNextPrimary();
			if (nextHandle != 0 && !fSlotStore->SlotHandleIsValid(nextHandle))
				forwardHasProblems = true;

			// Check to see if we already have this block in the set, in which case we have
			// a circularly linked list
			if (forwardSet.find(currentHandle) != forwardSet.end())
				{
				forwardHasProblems = true;
				break;
				}
			forwardSet.insert(currentHandle);
			previousHandle = currentHandle;
			currentHandle = nextHandle;
			}
		catch (...)
			{
			// We failed to load currentHandle, so bail from the loop
			break;
			}
		}
	// forwardSet now holds handles of all the blocks we were able
	// to find walking forwards through the list.
	}


	// Do the same thing, but backwards

	set<BlockID> backwardSet;
	bool backwardHasProblems = false;
	currentHandle = theLastHandle;
	{
	ZProgressPusher innerPusher(iProgressWatcher, "backward pass", countOfBlocks);
	BlockID nextHandle = 0;
	while (currentHandle != 0)
		{
		innerPusher.Process(1);
		try
			{
			if (ZCONFIG_TrackLostHandles && currentHandle != fPrimarySlot->GetSlotHandle())
				fSlotStore->MarkSlotHandleAsAllocated(currentHandle);

			this->LoadPrimarySlot(currentHandle);
			// Check that this block's next handle points to the last block we looked at
			if (fPrimarySlot->GetNextPrimary() != nextHandle)
				backwardHasProblems = true;
			// Check that this block's previous is a valid handle
			BlockID previousHandle = fPrimarySlot->GetPreviousPrimary();
			if (previousHandle != 0 && !fSlotStore->SlotHandleIsValid(previousHandle))
				backwardHasProblems = true;

			// Check to see if we already have this block in the set, in which case we have
			// a circularly linked list
			if (backwardSet.find(currentHandle) != backwardSet.end())
				{
				backwardHasProblems = true;
				break;
				}
			backwardSet.insert(currentHandle);
			nextHandle = currentHandle;
			currentHandle = previousHandle;
			}
		catch (...)
			{
			// We failed to load currentHandle, so bail from the loop
			break;
			}
		}
	}

	// If there were no problems, and the sets are the same size, then check that the
	// sets have the same contents
	bool allOkay = !forwardHasProblems && !backwardHasProblems && forwardSet.size() == backwardSet.size();
	if (allOkay)
		{
		set<BlockID>::iterator forwardIter = forwardSet.begin();
		set<BlockID>::iterator backwardIter = backwardSet.begin();

		while (forwardIter != forwardSet.end())
			{
			if (*forwardIter != *backwardIter)
				{
				allOkay = false;
				break;
				}
			++forwardIter;
			++backwardIter;
			}
		}

	set<BlockID> allBlocks;
	if (allOkay)
		{
		allBlocks = forwardSet;
		pusher.Process(1);
		}
	else
		{
		// There's a problem of some kind. Build a master list of blocks
		ZProgressPusher innerPusher(iProgressWatcher, "relink pass", allBlocks.size());

		std::set_union(forwardSet.begin(), forwardSet.end(), backwardSet.begin(), backwardSet.end(), inserter(allBlocks, allBlocks.begin()));

		// Walk through all the blocks in the set, pulling in each primary and linking it to the next.
		BlockID previousHandle = 0;
		for (set<BlockID>::iterator i = allBlocks.begin(); i != allBlocks.end();)
			{
			innerPusher.Process(1);
			BlockID currentHandle = *i;
			++i;
			BlockID nextHandle = 0;
			if (i != allBlocks.end())
				nextHandle = *i;

			this->LoadPrimarySlot(currentHandle);
			fPrimarySlot->SetPreviousPrimary(previousHandle);
			fPrimarySlot->SetNextPrimary(nextHandle);
			previousHandle = currentHandle;
			}
		if (allBlocks.size() != 0)
			fHeader->SetHeadPrimary(*allBlocks.begin());
		else
			fHeader->SetHeadPrimary(0);
		fHeader->SetTailPrimary(previousHandle);
		fHeader->SetAllocatedBlockCount(allBlocks.size());

		}
	// Commit our changes to disk
	this->Flush();

	// We now have a reliable chain of blocks. Walk through them making sure
	// we can get to all of their secondaries, and that they don't share
	// secondaries (ie they're cross-linked)
	set<ZSlotStore::SlotHandle> allocatedSlots(allBlocks);
	{
	ZProgressPusher innerPusher(iProgressWatcher, "block check pass", fHeader->GetAllocatedBlockCount());

	// Remember, we're also using a slot for our header info
	allocatedSlots.insert(fSlotHandleOfHeader);
	vector<BlockID> blocksToDelete;
	BlockID currentPrimary = this->GetFirstBlockID();
	while (currentPrimary != 0)
		{
		innerPusher.Process(1);
		// Attempt to pull in all the secondaries for this block, and make sure that
		// its size corresponds to the amount of space occupied
		vector<ZSlotStore::SlotHandle> currentSecondaries;
		this->LoadPrimarySlot(currentPrimary);
		ZSlotStore::SlotHandle currentSecondary = fPrimarySlot->GetHeadSecondary();
		size_t allocatedSpace = PrimarySlot::sGetContentSize(fSlotStore->GetSlotSize());
		bool allOkay = true;
		ZSlotStore::SlotHandle previousSecondary = 0;
		while (currentSecondary != 0)
			{
			try
				{
				// Has this secondary already been claimed for a primary or another secondary?
				if (allocatedSlots.find(currentSecondary) != allocatedSlots.end())
					{
					allOkay = false;
					break;
					}

				if (ZCONFIG_TrackLostHandles && currentSecondary != fSecondarySlot->GetSlotHandle())
					fSlotStore->MarkSlotHandleAsAllocated(currentSecondary);

				this->LoadSecondarySlot(currentSecondary);
				allocatedSlots.insert(currentSecondary);
				currentSecondaries.push_back(currentSecondary);
				// Make sure the forward and back pointers concur
				if (fSecondarySlot->GetPreviousSlot() != previousSecondary)
					{
					allOkay = false;
					break;
					}
				// Update the amount of space available for this block
				allocatedSpace += SecondarySlot::sGetContentSize(fSlotStore->GetSlotSize());
				}
			catch (...)
				{
				allOkay = false;
				break;
				}
			previousSecondary = currentSecondary;
			currentSecondary = fSecondarySlot->GetNextSlot();
			}

		// If the secondaries don't have enough space for the block
		if (allocatedSpace < fPrimarySlot->GetBlockSize())
			allOkay = false;

		ZSlotStore::SlotHandle nextPrimary = fPrimarySlot->GetNextPrimary();
		if (!allOkay)
			{
			for (vector<ZSlotStore::SlotHandle>::iterator i = currentSecondaries.begin(); i != currentSecondaries.end(); ++i)
				fSlotStore->FreeSlot(*i);

			ZSlotStore::SlotHandle previousPrimary = fPrimarySlot->GetPreviousPrimary();
			fPrimarySlot->DeleteYourSlot(fSlotStore);

			// And update their next and previous pointers to reference each other
			if (previousPrimary != 0)
				{
				this->LoadPrimarySlot(previousPrimary);
				fPrimarySlot->SetNextPrimary(nextPrimary);
				}
			if (nextPrimary != 0)
				{
				this->LoadPrimarySlot(nextPrimary);
				fPrimarySlot->SetPreviousPrimary(previousPrimary);
				}

			// Fix up the header, in case this primary was at the head and/or tail of the list of primaries
			if (fHeader->GetHeadPrimary() == currentPrimary)
				fHeader->SetHeadPrimary(nextPrimary);
			if (fHeader->GetTailPrimary() == currentPrimary)
				fHeader->SetTailPrimary(previousPrimary);
			fHeader->SetAllocatedBlockCount(fHeader->GetAllocatedBlockCount()-1);
			}
		currentPrimary = nextPrimary;
		}
	}

	{
	ZProgressPusher innerPusher(iProgressWatcher, "allocation pass", fSlotStore->GetSlotCount());

	// Walk through all the slots the slotstore thinks it has allocated unmark any slots
	ZSlotStore::SlotHandle currentSlotHandle = fSlotStore->GetFirstSlotHandle();
	while (currentSlotHandle != 0)
		{
		ZSlotStore::SlotHandle nextSlotHandle = fSlotStore->GetNextSlotHandle(currentSlotHandle);
		if (allocatedSlots.find(currentSlotHandle) == allocatedSlots.end())
			fSlotStore->FreeSlot(currentSlotHandle);
		innerPusher.Process(1);
		ZAssertStop(kDebug_BlockStore, nextSlotHandle != currentSlotHandle);
		currentSlotHandle = nextSlotHandle;
		}
	}
	}

ZSlotStore::SlotHandle ZBlockStore_SlotStore::AppendNewSecondaryToLoadedPrimary()
	{
	ZAssertStop(kDebug_BlockStore, fPrimarySlot->GetSlotHandle() != 0);

	ZSlotStore::SlotHandle newTailSecondaryHandle = fSlotStore->AllocateSlot();
	ZSlotStore::SlotHandle oldTailSecondaryHandle = fPrimarySlot->GetTailSecondary();
	if (oldTailSecondaryHandle)
		{
		this->LoadSecondarySlot(oldTailSecondaryHandle);
		fSecondarySlot->SetNextSlot(newTailSecondaryHandle);
		fPrimarySlot->SetTailSecondary(newTailSecondaryHandle);
		}
	else
		{
		fPrimarySlot->SetTailSecondary(newTailSecondaryHandle);
		fPrimarySlot->SetHeadSecondary(newTailSecondaryHandle);
		}
	this->CreateSecondarySlot(newTailSecondaryHandle);
	fSecondarySlot->SetPreviousSlot(oldTailSecondaryHandle);
	return newTailSecondaryHandle;
	}

void ZBlockStore_SlotStore::DeleteTailSecondaryOfLoadedPrimary()
	{
	ZSlotStore::SlotHandle oldTailSecondaryHandle = fPrimarySlot->GetTailSecondary();
	this->LoadSecondarySlot(oldTailSecondaryHandle);
	ZSlotStore::SlotHandle previousSecondary = fSecondarySlot->GetPreviousSlot();
	fSecondarySlot->DeleteYourSlot(fSlotStore);
	if (previousSecondary != 0)
		{
		this->LoadSecondarySlot(previousSecondary);
		fSecondarySlot->SetNextSlot(0);
		fPrimarySlot->SetTailSecondary(previousSecondary);
		}
	else
		{
		fPrimarySlot->SetTailSecondary(0);
		fPrimarySlot->SetHeadSecondary(0);
		}
	}

void ZBlockStore_SlotStore::CreatePrimarySlot(ZSlotStore::SlotHandle inNewPrimarySlotHandle)
	{
	if (!fPrimarySlot->IsClean())
		fPrimarySlot->WriteTo(fSlotStore);
	fPrimarySlot->ResetAttributes();
	fPrimarySlot->ZeroContents(fSlotStore->GetSlotSize());
	fPrimarySlot->SetSlotHandle(inNewPrimarySlotHandle);
	}

void ZBlockStore_SlotStore::CreateSecondarySlot(ZSlotStore::SlotHandle inNewSecondarySlotHandle)
	{
	if (!fSecondarySlot->IsClean())
		fSecondarySlot->WriteTo(fSlotStore);
	fSecondarySlot->ResetAttributes();
	fSecondarySlot->ZeroContents(fSlotStore->GetSlotSize());
	fSecondarySlot->SetSlotHandle(inNewSecondarySlotHandle);
	}

ZSlotStore::SlotHandle ZBlockStore_SlotStore::Internal_LoadPrimarySlot(ZSlotStore::SlotHandle inSlotHandle)
	{
	if (!fPrimarySlot->IsClean())
		fPrimarySlot->WriteTo(fSlotStore);
	fPrimarySlot->ResetAttributes();
	fPrimarySlot->SetSlotHandle(inSlotHandle);
	try
		{
		fPrimarySlot->ReadFrom(fSlotStore);
		}
	catch (...)
		{
		fPrimarySlot->ResetAttributes();
		throw;
		}
	return inSlotHandle;
	}

ZSlotStore::SlotHandle ZBlockStore_SlotStore::Internal_LoadSecondarySlot(ZSlotStore::SlotHandle inSlotHandle)
	{
	if (!fSecondarySlot->IsClean())
		fSecondarySlot->WriteTo(fSlotStore);
	fSecondarySlot->ResetAttributes();
	fSecondarySlot->SetSlotHandle(inSlotHandle);
	try
		{
		fSecondarySlot->ReadFrom(fSlotStore);
		}
	catch (...)
		{
		fSecondarySlot->ResetAttributes();
		throw;
		}
	return inSlotHandle;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBlockStore_SlotStore::Stream definition

ZBlockStore_SlotStore::Stream::Stream(ZBlockStore_SlotStore* inBlockStore, ZBlockStore::BlockID inBlockID)
	{
	fPosition = 0;
	fCurrentSecondary = 0;
	fBlockStore = inBlockStore;
	fBlockID = inBlockID;
	}

ZBlockStore_SlotStore::Stream::~Stream()
	{}

void ZBlockStore_SlotStore::Stream::Imp_Read(void* inDest, size_t inCount, size_t* outCountRead)
	{
	if (outCountRead)
		*outCountRead = 0;

	ZMutexLocker locker(fBlockStore->GetMutex());

	// Make sure our primary slot is loaded
	fBlockStore->LoadPrimarySlot(fBlockID);

	// Generate some useful values
	const size_t blockSize = fBlockStore->fPrimarySlot->GetBlockSize();
	const size_t slotSize(fBlockStore->fSlotStore->GetSlotSize());
	const size_t primaryContentSize(ZBlockStore_SlotStore::PrimarySlot::sGetContentSize(slotSize));
	const size_t secondaryContentSize(ZBlockStore_SlotStore::SecondarySlot::sGetContentSize(slotSize));

	char* localDest = reinterpret_cast<char*>(inDest);
	size_t countRemaining = inCount;

	// Keep going till the size goes to zero
	while (countRemaining > 0)
		{
		if (fPosition < primaryContentSize)
			{
			// We're inside the primary
			size_t amountToMove = min(primaryContentSize - fPosition, blockSize - fPosition);
			amountToMove = min(countRemaining, amountToMove);
			if (amountToMove == 0)
				break;
			fBlockStore->fPrimarySlot->CopyContentsTo(localDest, fPosition, amountToMove);
			fPosition += amountToMove;
			localDest += amountToMove;
			countRemaining -= amountToMove;
			if (outCountRead)
				*outCountRead += amountToMove;
			}
		else
			{
			size_t offsetIntoSecondaries = fPosition - primaryContentSize;
			int indexOfCurrentSecondary = offsetIntoSecondaries / secondaryContentSize;
			size_t offsetInCurrentSecondary = offsetIntoSecondaries % secondaryContentSize;

			if (offsetInCurrentSecondary == 0)
				{
				// If we're at the start of a secondary then get it loaded
				if (fPosition == primaryContentSize)
					{
					ZSlotStore::SlotHandle theNext = fBlockStore->fPrimarySlot->GetHeadSecondary();
					fCurrentSecondary = theNext;
					}
				else
					{
					fBlockStore->LoadSecondarySlot(fCurrentSecondary);
					ZSlotStore::SlotHandle theNext = fBlockStore->fSecondarySlot->GetNextSlot();
					fCurrentSecondary = theNext;
					}
				}
			fBlockStore->LoadSecondarySlot(fCurrentSecondary);

			size_t amountToMove = min(secondaryContentSize - offsetInCurrentSecondary, blockSize - fPosition);
			amountToMove = min(countRemaining, amountToMove);
			if (amountToMove == 0)
				break;
			fBlockStore->fSecondarySlot->CopyContentsTo(localDest, offsetInCurrentSecondary, amountToMove);
			fPosition += amountToMove;
			localDest += amountToMove;
			countRemaining -= amountToMove;
			if (outCountRead)
				*outCountRead += amountToMove;
			}
		}
	}

void ZBlockStore_SlotStore::Stream::Imp_Write(const void* inSource, size_t inCount, size_t* outCountWritten)
	{
	if (outCountWritten)
		*outCountWritten = 0;

	ZMutexLocker locker(fBlockStore->GetMutex());

	// Make sure our primary slot is loaded
	fBlockStore->LoadPrimarySlot(fBlockID);

	// And generate some useful values
	const size_t slotSize = fBlockStore->fSlotStore->GetSlotSize();
	const size_t primaryContentSize = ZBlockStore_SlotStore::PrimarySlot::sGetContentSize(slotSize);
	const size_t secondaryContentSize = ZBlockStore_SlotStore::SecondarySlot::sGetContentSize(slotSize);

	const char* localSource = reinterpret_cast<const char*>(inSource);
	size_t countRemaining = inCount;

	// Keep going till the size goes to zero
	while (countRemaining > 0)
		{
		// Are we inside the primary?
		if (fPosition < primaryContentSize)
			{
			size_t amountToMove = min(countRemaining, primaryContentSize - fPosition);
			fBlockStore->fPrimarySlot->CopyContentsFrom(localSource, fPosition, amountToMove);
			fPosition += amountToMove;
			localSource += amountToMove;
			countRemaining -= amountToMove;
			if (outCountWritten)
				*outCountWritten += amountToMove;
			}
		else
			{
			size_t offsetIntoSecondaries = fPosition - primaryContentSize ;
			int indexOfCurrentSecondary = offsetIntoSecondaries / secondaryContentSize;
			size_t offsetInCurrentSecondary = offsetIntoSecondaries % secondaryContentSize;
			// Are we at the start of a secondary?
			if (offsetInCurrentSecondary == 0)
				{
				// Then load/create the secondary
				if (fPosition == primaryContentSize)
					{
					fCurrentSecondary = fBlockStore->fPrimarySlot->GetHeadSecondary();
					}
				else
					{
					fBlockStore->LoadSecondarySlot(fCurrentSecondary);
					fCurrentSecondary = fBlockStore->fSecondarySlot->GetNextSlot();
					}
				}
			if (fCurrentSecondary == 0)
				fCurrentSecondary = fBlockStore->AppendNewSecondaryToLoadedPrimary();
			fBlockStore->LoadSecondarySlot(fCurrentSecondary);

			size_t amountToMove = min(countRemaining, secondaryContentSize - offsetInCurrentSecondary);
			fBlockStore->fSecondarySlot->CopyContentsFrom(localSource, offsetInCurrentSecondary, amountToMove);
			fPosition += amountToMove;
			localSource += amountToMove;
			countRemaining -= amountToMove;
			if (outCountWritten)
				*outCountWritten += amountToMove;
			}
		if (fPosition > fBlockStore->fPrimarySlot->GetBlockSize())
			fBlockStore->fPrimarySlot->SetBlockSize(fPosition);
		}
	}

void ZBlockStore_SlotStore::Stream::Imp_Flush()
	{
	ZMutexLocker locker(fBlockStore->GetMutex());
	fBlockStore->Flush();
	}

uint64 ZBlockStore_SlotStore::Stream::Imp_GetPosition()
	{ return fPosition; }

static size_t sCalcEndOfSlot(size_t inOffset, size_t inSlotSize)
	{ return ((inOffset + inSlotSize - 1) / inSlotSize) * inSlotSize; }

void ZBlockStore_SlotStore::Stream::Imp_SetPosition(uint64 inPosition)
	{
	// We don't respect the (new) semantics for streams. If necessary I'll
	// revamp this code at some point, but for most uses the difference
	// between old and new semantics is not going to be significant.

	// Remember, if we are positioned at the beginning of a secondary slot (at offset zero within
	// that slot) then fCurrentSecondary will record the handle of the *prior* slot. This is so
	// we don't allocate a secondary slot until we actually store a byte into it.

	ZMutexLocker locker(fBlockStore->GetMutex());

	// Bail if we're already where we need to be
	if (fPosition == inPosition)
		return;

	// Generate some useful values
	const size_t slotSize = fBlockStore->fSlotStore->GetSlotSize();
	const size_t primaryContentSize = ZBlockStore_SlotStore::PrimarySlot::sGetContentSize(slotSize);
	const size_t secondaryContentSize = ZBlockStore_SlotStore::SecondarySlot::sGetContentSize(slotSize);

	// Make sure our primary slot is loaded
	fBlockStore->LoadPrimarySlot(fBlockID);

	// Handle the simple case, positioning inside the primary slot
	if (inPosition <= primaryContentSize)
		{
		fPosition = inPosition;
		fCurrentSecondary = 0;
		}
	else
		{
		size_t currentPositionInTail = fPosition - primaryContentSize;
		size_t newPositionInTail = inPosition - primaryContentSize;
		size_t endOfCurrentSlot = ::sCalcEndOfSlot(currentPositionInTail, secondaryContentSize);
		size_t endOfNewSlot = ::sCalcEndOfSlot(newPositionInTail, secondaryContentSize);
		// Two different tasks if we're moving backwards or forwards
		if (inPosition > fPosition)
			{
			// Moving forwards
			while (endOfNewSlot > endOfCurrentSlot)
				{
				if (fCurrentSecondary == 0)
					{
					fCurrentSecondary = fBlockStore->fPrimarySlot->GetHeadSecondary();
					}
				else
					{
					fBlockStore->LoadSecondarySlot(fCurrentSecondary);
					fCurrentSecondary = fBlockStore->fSecondarySlot->GetNextSlot();
					}
				if (fCurrentSecondary == 0)
					fCurrentSecondary = fBlockStore->AppendNewSecondaryToLoadedPrimary();
				endOfCurrentSlot += secondaryContentSize;
				}
			fPosition = inPosition;
			// And update the recorded block size, if we extended it
			if (fPosition > fBlockStore->fPrimarySlot->GetBlockSize())
				fBlockStore->fPrimarySlot->SetBlockSize(fPosition);
			}
		else
			{
			while (endOfNewSlot < endOfCurrentSlot)
				{
				ZAssertStop(kDebug_BlockStore, fCurrentSecondary != 0);
				fBlockStore->LoadSecondarySlot(fCurrentSecondary);
				fCurrentSecondary = fBlockStore->fSecondarySlot->GetPreviousSlot();
				endOfCurrentSlot -= secondaryContentSize;
				}
			fPosition = inPosition;
			}
		}
	}

uint64 ZBlockStore_SlotStore::Stream::Imp_GetSize()
	{
	ZMutexLocker locker(fBlockStore->GetMutex());
	fBlockStore->LoadPrimarySlot(fBlockID);
	return fBlockStore->fPrimarySlot->GetBlockSize();
	}

void ZBlockStore_SlotStore::Stream::Imp_SetSize(uint64 inSize)
	{
	if (inSize > 0xFFFFFFFFU)
		sThrowBadSize();

	ZMutexLocker locker(fBlockStore->GetMutex());

	// Make sure our primary slot is loaded
	fBlockStore->LoadPrimarySlot(fBlockID);

	size_t currentSize = fBlockStore->fPrimarySlot->GetBlockSize();

	if (inSize == currentSize)
		return;

	// Generate some useful values
	const size_t slotSize = fBlockStore->fSlotStore->GetSlotSize();
	const size_t primaryContentSize = ZBlockStore_SlotStore::PrimarySlot::sGetContentSize(slotSize);
	const size_t secondaryContentSize = ZBlockStore_SlotStore::SecondarySlot::sGetContentSize(slotSize);

	// The simplest case, we currently only have a primary slot, and the new size will not
	// require us to delete or create secondary slots
	if (inSize <= primaryContentSize && currentSize <= primaryContentSize)
		{
		fBlockStore->fPrimarySlot->SetBlockSize(inSize);
		// Make sure our position gets limited to our current size
		if (fPosition > inSize)
			fPosition = inSize;
		}
	else
		{
		size_t currentTailSize = currentSize - primaryContentSize;
		size_t newTailSize = inSize - primaryContentSize;
		size_t endOfCurrentSlot = ::sCalcEndOfSlot(currentTailSize, secondaryContentSize);
		size_t endOfNewSlot = ::sCalcEndOfSlot(newTailSize, secondaryContentSize);
		if (newTailSize > currentTailSize)
			{
			while (endOfNewSlot > endOfCurrentSlot)
				{
				fBlockStore->AppendNewSecondaryToLoadedPrimary();
				endOfCurrentSlot += secondaryContentSize;
				}
			fBlockStore->fPrimarySlot->SetBlockSize(inSize);
			}
		else
			{
			while (endOfNewSlot < endOfCurrentSlot)
				{
				fBlockStore->DeleteTailSecondaryOfLoadedPrimary();
				endOfCurrentSlot -= secondaryContentSize;
				}
			fBlockStore->fPrimarySlot->SetBlockSize(inSize);
			if (fPosition > inSize)
				{
				// Note that we keep a reference to the secondary that contains the byte
				// before our fPosition. This gives us immediate access to that slot when we're
				// reading or writing
				fPosition = inSize;
				fCurrentSecondary = fBlockStore->fPrimarySlot->GetTailSecondary();
				}
			}
		}
	}
