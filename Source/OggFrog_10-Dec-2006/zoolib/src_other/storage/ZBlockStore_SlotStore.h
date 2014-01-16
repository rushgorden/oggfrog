/*  @(#) $Id: ZBlockStore_SlotStore.h,v 1.5 2004/05/05 15:54:38 agreen Exp $ */

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

#ifndef __ZBlockStore_SlotStore__
#define __ZBlockStore_SlotStore__ 1
#include "zconfig.h"

#include "ZBlockStore.h"
#include "ZSlotStore.h"

class ZProgressWatcher;

class ZBlockStore_SlotStore : public ZBlockStore
	{
public:
// Open constructor
	ZBlockStore_SlotStore(ZSlotStore* inSlotStore, ZSlotStore::SlotHandle inSlotHandleOfHeader);
// Create constructor
	ZBlockStore_SlotStore(ZSlotStore* inSlotStore);
	virtual ~ZBlockStore_SlotStore();

// From ZBlockStore
	virtual void Flush();

	virtual ZRef<ZStreamerRWPos> Create(BlockID& oBlockID);
	virtual ZRef<ZStreamerRWPos> OpenRWPos(BlockID iBlockID);
	virtual bool Delete(BlockID iBlockID);
	virtual size_t EfficientSize();

// Our protocol
	size_t GetBlockCount();

	ZBlockStore::BlockID GetFirstBlockID();
	ZBlockStore::BlockID GetLastBlockID();

	ZBlockStore::BlockID GetNextBlockID(BlockID inBlockID);
	ZBlockStore::BlockID GetPreviousBlockID(BlockID inBlockID);

	static std::string sBlockIDAsString(ZBlockStore::BlockID inBlockID);

	ZSlotStore* GetSlotStore() const { return fSlotStore; }

	int32 GetRefCon();
	void SetRefCon(int32 inRefCon);

	ZSlotStore::SlotHandle GetSlotHandleOfHeader();

// Fixing the store
	void ValidateAndFix(ZProgressWatcher* inProgressWatcher);

private:
// Implementation specific stuff, used by our streamwriter
	ZSlotStore::SlotHandle AppendNewSecondaryToLoadedPrimary();
	void DeleteTailSecondaryOfLoadedPrimary();
	void CreatePrimarySlot(ZSlotStore::SlotHandle inNewPrimarySlotHandle);
	void CreateSecondarySlot(ZSlotStore::SlotHandle inNewSecondarySlotHandle);

// These have a return value just so we can do a condition in an inline
	ZSlotStore::SlotHandle Internal_LoadPrimarySlot(ZSlotStore::SlotHandle inSlotHandle);
	ZSlotStore::SlotHandle Internal_LoadSecondarySlot(ZSlotStore::SlotHandle inSlotHandle);

	ZSlotStore::SlotHandle LoadPrimarySlot(ZSlotStore::SlotHandle inSlotHandle);
	ZSlotStore::SlotHandle LoadSecondarySlot(ZSlotStore::SlotHandle inSlotHandle);

	ZMutex& GetMutex() { return fMutex; }

	ZMutex fMutex;

	ZSlotStore* fSlotStore;

// Private nested classes used in our implementation
	class Slot;
	class PrimarySlot;
	class SecondarySlot;
	class Header;
	class Stream;
	friend class Stream; // The stream will be messing with our internals a bit
	class Streamer;

	ZSlotStore::SlotHandle fSlotHandleOfHeader;
	Header* fHeader;

	PrimarySlot* fPrimarySlot;
	SecondarySlot* fSecondarySlot;
	};

// ==================================================

#endif // __ZBlockStore_SlotStore__
