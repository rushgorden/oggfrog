// "@(#) $Id: ZTSoup.java,v 1.22 2006/11/23 02:18:46 eric_cooper Exp $";

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

package org.zoolib.tuplebase;

import org.zoolib.DList;
import org.zoolib.DList;
import org.zoolib.ZDebug;
import org.zoolib.ZID;
import org.zoolib.ZTuple;
import org.zoolib.thread.*;

import java.lang.Long;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

// ====================================================================================================

public final class ZTSoup
	{
	public interface SyncListener
		{
		public void syncNeeded(ZTSoup iTSoup);
		}

	public interface UpdateListener
		{
		public void updateNeeded(ZTSoup iTSoup);
		}

	public ZTSoup(ZTSWatcher iTSWatcher, UpdateListener iUpdateListener, SyncListener iSyncListener)
		{
		fTSWatcher = iTSWatcher;
		fUpdateListener = iUpdateListener;
		fSyncListener = iSyncListener;
		}

	/** Detach from fTSWatcher */
	public final void close()
		{
		fTSWatcher.close();
		fTSWatcher = null;
		fUpdateListener = null;
		fSyncListener = null;
		}

	public final long allocateID()
		{ return fTSWatcher.allocateID(); }

	public final long add(Map iTuple)
		{
		long theID = fTSWatcher.allocateID();
		this.set(theID, iTuple);
		return theID;
		}

	public final void set(long iID, Map iTuple)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();
		PCrouton thePCrouton = this.pGetPCrouton(new ZID(iID));
		this.pSet(thePCrouton, iTuple);
		// fMutex_Structure will have been released by pSet;
		ZDebug.sAssert(!fMutex_Structure.isLocked());
		fMutex_CallUpdate.release();
		}

	public final void set(ZID iZID, Map iTuple)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();
		PCrouton thePCrouton = this.pGetPCrouton(iZID);
		this.pSet(thePCrouton, iTuple);
		// fMutex_Structure will have been released by pSet;
		ZDebug.sAssert(!fMutex_Structure.isLocked());
		fMutex_CallUpdate.release();
		}

	public final void sync()
		{
		fMutex_CallSync.acquire();
		fMutex_Structure.acquire();

		fCalled_SyncNeeded = false;

		Set removedIDs = new TreeSet();
		Set addedIDs = new TreeSet();
		Map writtenTuples = new TreeMap();

		for (Iterator iter = fPCroutons_Sync.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PCrouton thePCrouton = (PCrouton)iter.next();

			if (thePCrouton.fHasValue_ForServer)
				{
				thePCrouton.fHasValue_ForServer = false;
				writtenTuples.put(thePCrouton.fZID, thePCrouton.fValue_ForServer);

				if (!thePCrouton.fServerKnown)
					{
					thePCrouton.fServerKnown = true;
					addedIDs.add(thePCrouton.fZID);
					}

				// Put it on fPCroutons_Syncing, which will be transcribed
				// to fCroutons_Pending after we've called our watcher.
				fPCroutons_Syncing.insertIfNotContains(thePCrouton.fLink_Syncing);
				}
			else if (!thePCrouton.fUsingTCroutons.empty())
				{
				// It's in use.
				if (!thePCrouton.fServerKnown)
					{
					// But not known to the server.
					thePCrouton.fServerKnown = true;
					addedIDs.add(thePCrouton.fZID);
					}
				}
			else if (thePCrouton.fServerKnown)
				{
				// It's not in use, and is known to the server.
				thePCrouton.fServerKnown = false;
				removedIDs.add(thePCrouton.fZID);
				thePCrouton.fHasValue_Current = false;
				thePCrouton.fHasValue_Prior = false;
				fPCroutons_Update.insertIfNotContains(thePCrouton.fLink_Update);
				}
			else
				{
				// It's not in use, and not known to the server, so it should have
				// been pulled from the sync list by update and deleted.
				}
			}

		Set removedQueries = new TreeSet();
		Map addedQueries = new TreeMap();

		for (Iterator iter = fPSieves_Sync.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PSieve thePSieve = (PSieve)iter.next();

			if (!thePSieve.fUsingTSieves.empty())
				{
				// It's in use.
				if (!thePSieve.fServerKnown)
					{
					// But not known to the server.
					thePSieve.fServerKnown = true;
					addedQueries.put(thePSieve.fIdentity, thePSieve.fTBQuery);
					}
				}
			else if (thePSieve.fServerKnown)
				{
				// It's not in use, and is known to the server.
				thePSieve.fServerKnown = false;
				thePSieve.fHasResults_Current = false;
				thePSieve.fHasResults_Prior = false;
				removedQueries.add(thePSieve.fIdentity);
				fPSieves_Update.insertIfNotContains(thePSieve.fLink_Update);
				}
			else
				{
				// Shouldn't still be on the sync list if it's not in use and not known to the server
				}
			}

		fMutex_Structure.release();

		Set serverAddedIDs = new TreeSet();
		Map changedTuples = new TreeMap();
		Map changedQueries = new TreeMap();

		fTSWatcher.doIt(removedIDs, addedIDs,
					removedQueries, addedQueries,
					serverAddedIDs,
					changedTuples,
					writtenTuples, changedQueries);

		fMutex_Structure.acquire();

		for (Iterator i = serverAddedIDs.iterator(); i.hasNext(); /*no increment*/)
			{
			ZID theZID = (ZID)i.next();
			PCrouton thePCrouton = this.pGetPCrouton(theZID);
			ZDebug.sAssert(!thePCrouton.fServerKnown);
			thePCrouton.fServerKnown = true;
			fPCroutons_Pending.insertIfNotContains(thePCrouton.fLink_Pending);			
			}

		for (Iterator i = changedTuples.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			ZID theZID = (ZID)theEntry.getKey();
			PCrouton thePCrouton = (PCrouton)fMap_ID_To_PCrouton.get(theZID);
			if (thePCrouton != null)
				{
				if (!thePCrouton.fWrittenLocally)
					{
					thePCrouton.fValue_FromServer = (ZTuple)theEntry.getValue();
					fPCroutons_Changed.insertIfNotContains(thePCrouton.fLink_Changed);
					}
				}
			}

		for (Iterator i = changedQueries.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			Long theIdentity = (Long)theEntry.getKey();
			PSieve thePSieve = (PSieve)fMap_Identity_To_PSieve.get(theIdentity);
			ZDebug.sAssert(thePSieve != null);
			thePSieve.fResults_Remote = (List)theEntry.getValue();
			fPSieves_Changed.insertIfNotContains(thePSieve.fLink_Changed);
			}



		for (Iterator iter = fPCroutons_Syncing.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			fPCroutons_Pending.insertIfNotContains(((PCrouton)iter.next()).fLink_Pending);

		fMutex_CallSync.release();
		if (!fPSieves_Update.empty() || !fPSieves_Changed.empty()
			|| !fPCroutons_Update.empty() || !fPCroutons_Changed.empty()
			|| !fPCroutons_Pending.empty())
			{
			this.pTriggerUpdate();
			}

		fMutex_Structure.release();
		}

	public final void update()
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		fCalled_UpdateNeeded = false;

		if (fPSieves_Update.empty() && fPSieves_Changed.empty()
			&& fPCroutons_Update.empty() && fPCroutons_Changed.empty())
			{
			fMutex_Structure.release();
			fMutex_CallUpdate.release();
			return;
			}

		for (Iterator iter = fPCroutons_Update.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PCrouton thePCrouton = (PCrouton)iter.next();

			if (thePCrouton.fWrittenLocally)
				{
				// A local change has been made, mark it so that Sync knows to
				// send the value to the server.
				thePCrouton.fWrittenLocally = false;
				thePCrouton.fHasValue_ForServer = true;
				thePCrouton.fValue_ForServer = thePCrouton.fValue_Current;
				fPCroutons_Sync.insertIfNotContains(thePCrouton.fLink_Sync);
				}

			if (!thePCrouton.fHasValue_ForServer)
				{
				// It's not waiting to be sent to the server.
				if (!thePCrouton.fUsingTCroutons.empty())
					{
					// It's in use.
					if (!thePCrouton.fServerKnown)
						{
						// But not known to the server.
						fPCroutons_Sync.insertIfNotContains(thePCrouton.fLink_Sync);
						}
					}
				else if (!fPCroutons_Syncing.contains(thePCrouton.fLink_Syncing)
					&& !fPCroutons_Pending.contains(thePCrouton.fLink_Pending))
					{
					// It's not in use, and it's not syncing or pending.
					if (thePCrouton.fServerKnown)
						{
						// It's not in use, not syncing or pending, and is known to the server.
						fPCroutons_Sync.insertIfNotContains(thePCrouton.fLink_Sync);
						}
					else
						{
						// It's not in use, not syncing or pending, is not known
						// to the server so we can toss it.
						fPCroutons_Sync.removeIfContains(thePCrouton.fLink_Sync);
						fPCroutons_Changed.removeIfContains(thePCrouton.fLink_Changed);
						sEraseMustContain(fMap_ID_To_PCrouton, thePCrouton.fZID);
						}
					}
				}
			}

		for (Iterator iter = fPSieves_Update.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PSieve thePSieve = (PSieve)iter.next();

			if (!thePSieve.fUsingTSieves.empty())
				{
				// It's in use.
				if (!thePSieve.fServerKnown)
					{
					// But not known to the server.
					fPSieves_Sync.insertIfNotContains(thePSieve.fLink_Sync);
					}
				}
			else if (thePSieve.fServerKnown)
				{
				// It's not in use and is known to the server.
				fPSieves_Sync.insertIfNotContains(thePSieve.fLink_Sync);
				}
			else
				{
				// It's not in use, is not known to the server, so we can toss it.
				fPSieves_Sync.removeIfContains(thePSieve.fLink_Sync);
				fPSieves_Changed.removeIfContains(thePSieve.fLink_Changed);
				sEraseMustContain(fMap_TBQuery_To_PSieve, thePSieve.fTBQuery);
				sEraseMustContain(fMap_Identity_To_PSieve, thePSieve.fIdentity);
				}
			}

		// Pick up remotely changed croutons.
		List localTCroutons = new ArrayList();
		for (Iterator iter = fPCroutons_Changed.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PCrouton thePCrouton = (PCrouton)iter.next();

			thePCrouton.fHasValue_Prior = thePCrouton.fHasValue_Current;
			thePCrouton.fHasValue_Current = true;
			thePCrouton.fValue_Prior = thePCrouton.fValue_Current;
			thePCrouton.fValue_Current = thePCrouton.fValue_FromServer;

			for (Iterator i = thePCrouton.fUsingTCroutons.iterator(); i.hasNext(); /*no increment*/)
				localTCroutons.add(i.next());
			}

		// Pick up remotely changed sieves
		List localTSieves = new ArrayList();
		for (Iterator iter = fPSieves_Changed.iteratorEraseAll(); iter.hasNext(); /*no increment*/)
			{
			PSieve thePSieve = (PSieve)iter.next();

			thePSieve.fHasResults_Prior = thePSieve.fHasResults_Current;
			thePSieve.fHasResults_Current = true;

			thePSieve.fResults_Local_Prior = thePSieve.fResults_Local_Current;
			thePSieve.fResults_Local_Current = thePSieve.fResults_Remote;
			thePSieve.fHasDiffs = false;
			thePSieve.fAdded.clear();
			thePSieve.fRemoved.clear();

			for (Iterator i = thePSieve.fUsingTSieves.iterator(); i.hasNext(); /*no increment*/)
				localTSieves.add(i.next());
			}


		if (!fPCroutons_Pending.empty())
			{
			for (Iterator i = fPCroutons_Pending.iteratorEraseAll(); i.hasNext(); /*no increment*/)
				fPCroutons_Update.insertIfNotContains(((PCrouton)i.next()).fLink_Update);

			this.pTriggerUpdate();
			}

		if (!fPCroutons_Sync.empty() || !fPSieves_Sync.empty())
			this.pTriggerSync();

		fMutex_Structure.release();

		for (Iterator i = localTSieves.iterator(); i.hasNext(); /*no increment*/)
			{
			try { ((ZTSieve)i.next()).changed(); }
			catch (Exception ex) {}
			}

		for (Iterator i = localTCroutons.iterator(); i.hasNext(); /*no increment*/)
			{
			try { ((ZTCrouton)i.next()).changed(ZTCrouton.eChange_Remote); }
			catch (Exception ex) {}
			}
		fMutex_CallUpdate.release();
		}
	// =====
	final void pRegisterTSieve(ZTSieve iTSieve, ZTBQuery iTBQuery)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		Object existing = fMap_TBQuery_To_PSieve.get(iTBQuery);
		PSieve thePSieve;
		if (existing != null)
			{
			thePSieve = (PSieve)existing;
			}
		else
			{
			thePSieve = new PSieve(this, iTBQuery);
			thePSieve.fServerKnown = false;
			thePSieve.fHasResults_Current = false;
			thePSieve.fHasResults_Prior = false;
			thePSieve.fHasDiffs = false;
			
			fMap_TBQuery_To_PSieve.put(iTBQuery, thePSieve);
			fMap_Identity_To_PSieve.put(thePSieve.fIdentity, thePSieve);

			fPSieves_Update.insert(thePSieve.fLink_Update);

			this.pTriggerUpdate();
			}

		iTSieve.fPSieve = thePSieve;
		thePSieve.fUsingTSieves.insert(iTSieve.fLink_PSieve);

		if (thePSieve.fHasResults_Current)
			{
			fMutex_Structure.release();
			try { iTSieve.changed(); }
			catch (Exception ex) {}
			}
		else
			{
			fMutex_Structure.release();
			}
		fMutex_CallUpdate.release();
		}

	final void pCloseTSieve(ZTSieve iTSieve)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		PSieve thePSieve = iTSieve.fPSieve;
		iTSieve.fPSieve = null;

		thePSieve.fUsingTSieves.remove(iTSieve.fLink_PSieve);

		if (thePSieve.fUsingTSieves.empty())
			{
			fPSieves_Update.insertIfNotContains(thePSieve.fLink_Update);
			this.pTriggerUpdate();
			}

		fMutex_Structure.release();
		fMutex_CallUpdate.release();
		}

	final void pRegisterTCrouton(ZTCrouton iTCrouton)
		{ this.pRegisterTCrouton(iTCrouton, fTSWatcher.allocateID()); }

	final void pRegisterTCrouton(ZTCrouton iTCrouton, long iID)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		PCrouton thePCrouton = this.pGetPCrouton(new ZID(iID));

		iTCrouton.fPCrouton = thePCrouton;
		thePCrouton.fUsingTCroutons.insert(iTCrouton.fLink_PCrouton);
		if (thePCrouton.fHasValue_Current)
			{
 			fMutex_Structure.release();
			// AG 2006-10-23. Having creation and registration of
			// a crouton be separate steps lets the crouton's owner
			// know what the croutons identity is before there's
			// any opportunity for its changed method to be called.
			// Nevertheless there's some code in KF that breaks
			// when we do the callback here, so it's disabled for now.
			try { iTCrouton.changed(ZTCrouton.eChange_Local); }
			catch (Exception ex) {}
			}
		else
			{
			fMutex_Structure.release();
			}
		fMutex_CallUpdate.release();		
		}

	final void pCloseTCrouton(ZTCrouton iTCrouton)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		PCrouton thePCrouton = iTCrouton.fPCrouton;
		iTCrouton.fPCrouton = null;

		thePCrouton.fUsingTCroutons.remove(iTCrouton.fLink_PCrouton);			
		if (thePCrouton.fUsingTCroutons.empty())
			{
			fPCroutons_Update.insertIfNotContains(thePCrouton.fLink_Update);
			this.pTriggerUpdate();
			}

		fMutex_Structure.release();
		fMutex_CallUpdate.release();
		}

	// =====

	// =====
	void pCheckDiffs(PSieve iPSieve)
		{
		ZDebug.sAssert(!fMutex_CallUpdate.isLocked());
		ZDebug.sAssert(!fMutex_Structure.isLocked());

		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();

		if (!iPSieve.fHasDiffs)
			{
			iPSieve.fHasDiffs = true;			
			iPSieve.fAdded.addAll(iPSieve.fResults_Local_Current);
			iPSieve.fAdded.removeAll(iPSieve.fResults_Local_Prior);

			iPSieve.fRemoved.addAll(iPSieve.fResults_Local_Prior);
			iPSieve.fRemoved.removeAll(iPSieve.fResults_Local_Current);
			}
		fMutex_Structure.release();
		fMutex_CallUpdate.release();
		}

	private PCrouton pGetPCrouton(ZID iZID)
		{
		ZDebug.sAssert(fMutex_Structure.isLocked());
		Object existing = fMap_ID_To_PCrouton.get(iZID);
		if (existing != null)
			{
			return (PCrouton)existing;
			}
		else
			{
			PCrouton thePCrouton = new PCrouton(this, iZID);
			sInsertMustNotContain(fMap_ID_To_PCrouton, iZID, thePCrouton);
			thePCrouton.fServerKnown = false;
			thePCrouton.fHasValue_Current = false;
			thePCrouton.fHasValue_Prior = false;
			thePCrouton.fWrittenLocally = false;
			thePCrouton.fHasValue_ForServer = false;
			fPCroutons_Update.insert(thePCrouton.fLink_Update);
			this.pTriggerUpdate();
			return thePCrouton;
			}
		}

	final void pSetFromTCrouton(PCrouton iPCrouton, Map iMap)
		{
		fMutex_CallUpdate.acquire();
		fMutex_Structure.acquire();
		this.pSet(iPCrouton, iMap);
		// fMutex_Structure will have been released by pSet;
		ZDebug.sAssert(!fMutex_Structure.isLocked());
		fMutex_CallUpdate.release();		
		}

	final void pSet(PCrouton iPCrouton, Map iMap)
		{
		ZDebug.sAssert(fMutex_CallUpdate.isLocked());
		ZDebug.sAssert(fMutex_Structure.isLocked());

		iPCrouton.fHasValue_Prior = iPCrouton.fHasValue_Current;
		iPCrouton.fHasValue_Current = true;
		iPCrouton.fValue_Prior = iPCrouton.fValue_Current;
		iPCrouton.fValue_Current = new ZTuple(iMap);
		iPCrouton.fWrittenLocally = true;

		fPCroutons_Update.insertIfNotContains(iPCrouton.fLink_Update);

		this.pTriggerUpdate();

		List localTCroutons = new ArrayList();
		for (Iterator i = iPCrouton.fUsingTCroutons.iterator(); i.hasNext(); /*no increment*/)
			localTCroutons.add(i.next());

		fMutex_Structure.release();

		for (Iterator i = localTCroutons.iterator(); i.hasNext(); /*no increment*/)
			{
			try { ((ZTCrouton)i.next()).changed(ZTCrouton.eChange_Local); }
			catch (Exception ex) {}
			}
		}

	private final void pTriggerUpdate()
		{
		ZDebug.sAssert(fMutex_Structure.isLocked());
		if (!fCalled_UpdateNeeded)
			{
			fCalled_UpdateNeeded = true;
			fUpdateListener.updateNeeded(this);
			}
		}

	private final void pTriggerSync()
		{
		ZDebug.sAssert(fMutex_Structure.isLocked());
		if (!fCalled_SyncNeeded)
			{
			fCalled_SyncNeeded = true;
			fSyncListener.syncNeeded(this);
			}
		}

	// =====
	static long sNextIdentity = 1;
	public synchronized static long sNewIdentity()
		{ return sNextIdentity++; }
	// =====
	private ZTSWatcher fTSWatcher;

	private final ZMutex fMutex_CallSync = new ZMutex();
	private final ZMutex fMutex_CallUpdate = new ZMutex();
	private final ZMutex fMutex_Structure = new ZMutex();

	private UpdateListener fUpdateListener;
	private SyncListener fSyncListener;

	private boolean fCalled_UpdateNeeded = false;
	private boolean fCalled_SyncNeeded;


	private final Map fMap_Identity_To_PSieve = new TreeMap();
	private final Map fMap_TBQuery_To_PSieve = new TreeMap(new Comparator_TBQuery());

	private final Map fMap_ID_To_PCrouton = new TreeMap();


	private final DList fPSieves_Update = new DList();
	private final DList fPSieves_Sync = new DList();
	private final DList fPSieves_Changed = new DList();

	private final DList fPCroutons_Update = new DList();
	private final DList fPCroutons_Sync = new DList();
	private final DList fPCroutons_Changed = new DList();
	private final DList fPCroutons_Syncing = new DList();
	private final DList fPCroutons_Pending = new DList();
	// =====
	private static final class Comparator_TBQuery implements java.util.Comparator
		{
		public int compare(Object o1, Object o2)
			{ return ((ZTBQuery)o1).compare((ZTBQuery)o2); }
		}
	// =====
	static final class PSieve
		{
		public PSieve(ZTSoup iTSoup, ZTBQuery iTBQuery)
			{
			fTSoup = iTSoup;
			fTBQuery = iTBQuery;
			}

		public final Set getAdded()
			{
			fTSoup.pCheckDiffs(this);
			return fAdded;
			}

		public final Set getRemoved()
			{
			fTSoup.pCheckDiffs(this);
			return fRemoved;
			}

		public final ZTBQuery getTBQuery()
			{ return fTBQuery; }

		final ZTSoup fTSoup;
		final ZTBQuery fTBQuery;

		final DList fUsingTSieves = new DList();

		boolean fServerKnown;

		boolean fHasResults_Current;
		boolean fHasResults_Prior;

		List fResults_Local_Prior = new ArrayList();
		List fResults_Local_Current = new ArrayList();
		List fResults_Remote = new ArrayList();

		boolean fHasDiffs;
		Set fAdded = new TreeSet();
		Set fRemoved = new TreeSet();

		private final Long fIdentity = new Long(ZTSoup.sNewIdentity());

		final DList.Link fLink_Update = new DList.Link(this);
		final DList.Link fLink_Sync = new DList.Link(this);
		final DList.Link fLink_Changed = new DList.Link(this);
		}
	// =====
	static final class PCrouton
		{
		PCrouton(ZTSoup iTSoup, ZID iZID)
			{
			fTSoup = iTSoup;
			fID = iZID.longValue();
			fZID = iZID;
			}

		final ZTSoup fTSoup;
		final long fID;
		final ZID fZID;	

		final DList fUsingTCroutons = new DList();

		boolean fServerKnown;

		boolean fHasValue_Current;
		boolean fHasValue_Prior;

		ZTuple fValue_Current = new ZTuple();
		ZTuple fValue_Prior = new ZTuple();
		
		boolean fWrittenLocally;
		boolean fHasValue_ForServer;

		ZTuple fValue_ForServer = new ZTuple();
		ZTuple fValue_FromServer = new ZTuple();

		final DList.Link fLink_Update = new DList.Link(this);
		final DList.Link fLink_Sync = new DList.Link(this);
		final DList.Link fLink_Changed = new DList.Link(this);
		final DList.Link fLink_Syncing = new DList.Link(this);
		final DList.Link fLink_Pending = new DList.Link(this);
		}
	// =====
	private static final void sInsertMustNotContain(Map ioMap, Object iKey, Object iValue)
		{
		ZDebug.sAssert(!ioMap.containsKey(iKey));
		ioMap.put(iKey, iValue);
		}
	private static final void sEraseMustContain(Map ioMap, Object iKey)
		{
		ZDebug.sAssert(ioMap.containsKey(iKey));
		ioMap.remove(iKey);
		}
	}
