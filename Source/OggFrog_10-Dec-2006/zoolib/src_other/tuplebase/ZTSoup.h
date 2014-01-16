/*  @(#) $Id: ZTSoup.h,v 1.14 2006/11/02 18:13:17 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2005 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZTSoup__
#define __ZTSoup__ 1

#include "ZTSWatcher.h"
#include "ZDList.h"

class ZTCrouton;
class ZTSieve;

// =================================================================================================
#pragma mark -
#pragma mark * ZTSoup

class ZTSoup : public ZRefCounted
	{
public:
	enum { kDebug = 1 };

	typedef void (*Callback_UpdateNeeded_t)(void* iRefcon);
	typedef void (*Callback_SyncNeeded_t)(void* iRefcon);

	ZTSoup(ZRef<ZTSWatcher> iTSWatcher);
	virtual ~ZTSoup();

	void SetCallback_Update(
		Callback_UpdateNeeded_t iCallback_UpdateNeeded, void* iRefcon_UpdateNeeded);

	void SetCallback_Sync(
		Callback_SyncNeeded_t iCallback_SyncNeeded, void* iRefcon_SyncNeeded);

	void AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCountIssued);

	uint64 AllocateID();

	void Set(uint64 iID, const ZTuple& iTuple);

	uint64 Add(const ZTuple& iTuple);

	void Register(ZRef<ZTSieve> iTSieve, const ZTBQuery& iTBQuery);
	void Register(ZRef<ZTSieve> iTSieve, const ZTBQuery& iTBQuery, bool iPrefetch);
	void Register(ZRef<ZTCrouton> iTCrouton, uint64 iID);
	void Register(ZRef<ZTCrouton> iTCrouton);

	void Sync();
	void Update();

private:
	static void pCallback_TSWatcher(void* iRefcon);

	friend class ZTSieve;
	friend class ZTCrouton;

	class PSieve;
	class PSieveUpdate;
	class PSieveSync;
	class PSieveChanged;

	class PCrouton;
	class PCroutonUpdate;
	class PCroutonSync;
	class PCroutonChanged;
	class PCroutonSyncing;
	class PCroutonPending;

	void pDisposingTSieve(ZTSieve* iTSieve);
	void pDisposingTCrouton(ZTCrouton* iTCrouton);

	bool pHasCurrent(PSieve* iPSieve);
	bool pHasPrior(PSieve* iPSieve);

	const std::vector<uint64>& pSieveGetCurrent(PSieve* iPSieve);
	bool pSieveGetCurrent(PSieve* iPSieve, std::vector<uint64>& oCurrent);
	bool pSieveGetCurrent(PSieve* iPSieve, std::set<uint64>& oCurrent);

	const std::vector<uint64>& pSieveGetPrior(PSieve* iPSieve);
	bool pSieveGetPrior(PSieve* iPSieve, std::vector<uint64>& oPrior);
	bool pSieveGetPrior(PSieve* iPSieve, std::set<uint64>& oPrior);

	void pSieveGetAdded(PSieve* iPSieve, std::set<uint64>& oAdded);
	void pSieveGetRemoved(PSieve* iPSieve, std::set<uint64>& oRemoved);

	void pCheckSieveDiffs(PSieve* iPSieve);

	bool pHasCurrent(PCrouton* iPCrouton);
	bool pHasPrior(PCrouton* iPCrouton);

	bool pCroutonGetCurrent(PCrouton* iPCrouton, ZTuple& oTuple);
	bool pCroutonGetPrior(PCrouton* iPCrouton, ZTuple& oTuple);

	PCrouton* pGetPCrouton(uint64 iID);

	void pSetCroutonFromTCrouton(PCrouton* iPCrouton, const ZTuple& iTuple);
	void pSet(ZMutexLocker& iLocker_Structure, PCrouton* iPCrouton, const ZTuple& iTuple);	

	void pTriggerUpdate();
	void pTriggerSync();

	ZRef<ZTSWatcher> fTSWatcher;

	ZMutex fMutex_CallSync;
	ZMutex fMutex_CallUpdate;
	ZMutex fMutex_Structure;

	bool fCalled_UpdateNeeded;
	Callback_UpdateNeeded_t fCallback_UpdateNeeded;
	void* fRefcon_UpdateNeeded;
	
	bool fCalled_SyncNeeded;
	Callback_SyncNeeded_t fCallback_SyncNeeded;
	void* fRefcon_SyncNeeded;

	std::map<ZTBQuery, PSieve> fTBQuery_To_PSieve;

	ZooLib::DListHead<PSieveUpdate, kDebug> fPSieves_Update;
	ZooLib::DListHead<PSieveSync, kDebug> fPSieves_Sync;
	ZooLib::DListHead<PSieveChanged, kDebug> fPSieves_Changed;

	std::map<uint64, PCrouton> fID_To_PCrouton;

	ZooLib::DListHead<PCroutonUpdate, kDebug> fPCroutons_Update;
	ZooLib::DListHead<PCroutonSync, kDebug> fPCroutons_Sync;
	ZooLib::DListHead<PCroutonChanged, kDebug> fPCroutons_Changed;
	ZooLib::DListHead<PCroutonSyncing, kDebug> fPCroutons_Syncing;
	ZooLib::DListHead<PCroutonPending, kDebug> fPCroutons_Pending;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTSieve

class ZTSieveLink : public ZooLib::DListLink<ZTSieve, ZTSieveLink>
	{};

class ZTSieve : public ZRefCounted, public ZTSieveLink
	{
public:
	ZTSieve();
	virtual ~ZTSieve();

	virtual void Changed();

	bool HasCurrent();
	bool HasPrior();

	const std::vector<uint64>& GetCurrent();
	bool GetCurrent(std::vector<uint64>& oCurrent);
	bool GetCurrent(std::set<uint64>& oCurrent);

	const std::vector<uint64>& GetPrior();
	bool GetPrior(std::vector<uint64>& oPrior);
	bool GetPrior(std::set<uint64>& oPrior);

	void GetAdded(std::set<uint64>& oAdded);
	void GetRemoved(std::set<uint64>& oRemoved);

	ZRef<ZTSoup> GetTSoup();

	ZTBQuery GetTBQuery();

private:
	friend class ZTSoup;
	friend class ZTSoup::PSieve;

	ZTSoup::PSieve* fPSieve;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTCrouton

class ZTCroutonLink : public ZooLib::DListLink<ZTCrouton, ZTCroutonLink>
	{};

class ZTCrouton : public ZRefCounted, public ZTCroutonLink
	{
public:
	enum EChange { eChange_Local, eChange_Remote };

	ZTCrouton();
	virtual ~ZTCrouton();

	virtual void Changed(EChange iChange);

	uint64 GetID();

	bool HasCurrent();
	bool HasPrior();
	
	bool GetCurrent(ZTuple& oTuple);
	bool GetPrior(ZTuple& oTuple);

	ZTuple GetCurrent();
	ZTuple GetPrior();

	void Set(const ZTuple& iTuple);

	ZRef<ZTSoup> GetTSoup();

private:
	friend class ZTSoup;
	friend class ZTSoup::PCrouton;

	ZTSoup::PCrouton* fPCrouton;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTCrouton_Bowl

class ZTBowl;

class ZTCrouton_Bowl : public ZTCrouton
	{
public:
	ZTCrouton_Bowl();
	virtual void Changed(EChange iChange);

	ZRef<ZTBowl> GetBowl() const;

private:
	friend class ZTBowl;
	ZTBowl* fTBowl;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTBowl

class ZTBowl : public ZTSieve
	{
public:
	ZTBowl();
	virtual ~ZTBowl();

// From ZTSieve
	virtual void Changed();

// Our protocol
	virtual ZRef<ZTCrouton_Bowl> MakeCrouton();
	virtual void CroutonChanged(ZRef<ZTCrouton> iCrouton, ZTCrouton::EChange iChange);

	const std::vector<ZRef<ZTCrouton> >& GetCroutons();

private:
	std::vector<ZRef<ZTCrouton> > fTCroutons;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTBowl_T

struct ZTBowlBogusInitializerType;

template <class Crouton_t, typename Initalizer_t = ZTBowlBogusInitializerType>
class ZTBowl_T : public ZTBowl
	{
public:
	ZTBowl_T(Initalizer_t iInitializer)
	:	fInitializer(iInitializer)
		{}

// From ZTBowl
	virtual ZRef<ZTCrouton_Bowl> MakeCrouton()
		{ return new Crouton_t(fInitializer); }

private:
	Initalizer_t fInitializer;
	};

// For use with croutons that do not take an initializer.
template <class Crouton_t>
class ZTBowl_T<Crouton_t, ZTBowlBogusInitializerType> : public ZTBowl
	{
public:
	virtual ZRef<ZTCrouton_Bowl> MakeCrouton()
		{ return new Crouton_t; }
	};

#endif // __ZSoup__
