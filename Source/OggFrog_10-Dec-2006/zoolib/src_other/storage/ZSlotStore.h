/*  @(#) $Id: ZSlotStore.h,v 1.5 2006/07/17 18:33:16 agreen Exp $ */

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

#ifndef __ZSlotStore__
#define __ZSlotStore__ 1 
#include "zconfig.h"

#include "ZStreamer.h"

// This code is derived from SlottedFile in M.A.Sridhar's YACL library <http://www.cs.sc.edu/~sridhar/yacl.html>
// as documented in "Building Portable C++ Application with YACL", ISBN 0-201-83276-3, pp128-138.

// I've reworked it considerably to use a ZStreamRWPos to access the underlying storage medium, do some
// useful caching of allocation bit maps and to provide the GetStreamReadyToReadSlot/GetStreamReadyToWriteSlot
// methods which are helpful when building higher level entities (ie ZBlockStoreInSlotStore).

// =================================================================================================

#pragma mark -
#pragma mark * ZSlotStore

class ZSlotStore
	{
	ZSlotStore(); // Not implemented
	ZSlotStore(const ZSlotStore& other); // Not implemented
	ZSlotStore& operator=(const ZSlotStore& other); // Not implemented

public:
	enum { kInvalidHandle = 0 };
	typedef uint32 SlotHandle;

// Open constructor
	ZSlotStore(const ZStreamRWPos& inStream, size_t inUserHeaderSize);

// Create constructor
	ZSlotStore(const ZStreamRWPos& inStream, size_t inSlotSize, size_t inUserHeaderSize);

	~ZSlotStore();

// Ensure the underlying stream is up to date
	void Flush();

// Structural info
	size_t GetSlotSize() { return fSlotSize; }
	size_t GetSlotCount() { return fAllocatedSlotCount; }

// Checking status of slots
	bool SlotHandleIsValid(SlotHandle inHandle);
	bool SlotHandleIsAllocated(SlotHandle inHandle);

// Marking slot handles as allocated during validate and fix process. -ec 00.12.06
	bool MarkSlotHandleAsAllocated(SlotHandle inHandle);

// This can be used by higher level structures to directly access the underlying storage
// Note that these upper layers *must* respect the boundaries of a slot
	const ZStreamR& GetStreamReadyToReadSlot(SlotHandle inSlot);
	const ZStreamW& GetStreamReadyToWriteSlot(SlotHandle inSlot);

// The real stuff, allocating, freeing and accessing slots
	ZSlotStore::SlotHandle AllocateSlot();
	void FreeSlot(SlotHandle handle);

	void ReadSlot(SlotHandle inHandle, void* inBuffer);
	void WriteSlot(SlotHandle inHandle, const void* inBuffer);

// Accessors to walk allocated slots
	ZSlotStore::SlotHandle GetFirstSlotHandle();
	ZSlotStore::SlotHandle GetNextSlotHandle(SlotHandle inHandle);

	class BitSet; // formerly private -ct 2001.06.19
private:
// Internal implementation stuff
	void LoadClusterMap(uint32 inIndex);
	size_t ClusterIndexToPhysicalOffset(size_t inIndex);
	size_t SlotHandleToPhysicalOffset(SlotHandle inHandle);

	void WriteHeaderToStream(const ZStreamW& inStream);
	bool ReadHeaderFromStream(const ZStreamR& inStream);

	const ZStreamRWPos& fStream;

	//class BitSet; // now public --ct 2001.06.19

	BitSet* fBitSet_Full;
	BitSet* fBitSet_NonEmpty;
	size_t fAllocatedSlotCount; // #allocated slots in the file
	bool fHeaderClean;

	BitSet* fBitSet_CurrentCluster;
	uint32 fCurrentClusterIndex;
	bool fCurrentClusterClean;

	size_t fSlotSize; // Size of each slot, in bytes
	size_t fUserHeaderSize; // Our starting position in our stream
	};

#endif // __ZSlotStore__
