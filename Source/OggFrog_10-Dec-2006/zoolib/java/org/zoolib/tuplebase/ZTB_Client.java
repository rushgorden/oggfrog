// "@(#) $Id: ZTB_Client.java,v 1.10 2006/01/24 23:09:04 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2004 Andrew Green and Learning in Motion, Inc.
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

import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

//import org.zoolib.ZDebug;
import org.zoolib.ZID;
import org.zoolib.ZTuple;
import org.zoolib.ZTxn;
import org.zoolib.ZTxnTarget;
import org.zoolib.thread.*;

// ====================================================================================================

public class ZTB_Client extends ZTB implements ZTxnTarget
	{	
	// From ZTxnTarget
	public void TxnAbortPreValidate(long iTxnID)
		{
		fMutex_Structure.acquire();
		try
			{
			Long theID = new Long(iTxnID);
			TransRep theTrans = (TransRep)fTransactionsAll.remove(theID);
			fTransactions_Aborted_Unsent.add(theTrans);
			fCondition_Sender.broadcast();
			}
		finally
			{
			fMutex_Structure.release();
			}
		
		}

	public boolean TxnValidate(long iTxnID)
		{
		fMutex_Structure.acquire();
		try
			{
			TransRep theTrans = (TransRep)fTransactionsAll.get(new Long(iTxnID));
			fTransactions_Validate_Unsent.add(theTrans);
			fCondition_Sender.broadcast();
			while (!theTrans.fValidateCompleted)
				fCondition_Transaction.wait(fMutex_Structure);
			return theTrans.fValidateSucceeded;
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	public void TxnAbortPostValidate(long iTxnID)
		{
		fMutex_Structure.acquire();
		try
			{
			Long theID = new Long(iTxnID);
			TransRep theTrans = (TransRep)fTransactionsAll.remove(theID);
			fTransactions_Aborted_Unsent.add(theTrans);
			fCondition_Sender.broadcast();
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	public void TxnCommit(long iTxnID)
		{
		fMutex_Structure.acquire();
		try
			{
			TransRep theTrans = (TransRep)fTransactionsAll.get(new Long(iTxnID));
			fTransactions_Commit_Unsent.add(theTrans);
			fCondition_Sender.broadcast();
			while (!theTrans.fCommitCompleted)
				fCondition_Transaction.wait(fMutex_Structure);
			}
		finally
			{
			fMutex_Structure.release();
			}
		}


	static long sID = 1;
	synchronized static long sNextID()
		{ return ++sID; }

	ZTB_Client()
		{
		fPingRequested = false;
		fPingSent = false;
		}

	public long allocateID()
		{
		fMutex_Structure.acquire();
		try
			{
			for (;;)
				{
				while (fIDsHead != null)
					{
					if (fIDsHead.fCount > 0)
						{
						long result = fIDsHead.fBase;
						--fIDsHead.fCount;
						++fIDsHead.fBase;
						return result;
						}
					fIDsHead = fIDsHead.fNext;
					}
				++fIDsNeeded;
				fCondition_Sender.broadcast();
				fCondition_ID.wait(fMutex_Structure);
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	public ZTBTransRep getTransRep(ZTxn iTxn)
		{
		long theTxnID = iTxn.getID();
		Long theLong = new Long(theTxnID);
		fMutex_Structure.acquire();
		try
			{
			Object theOb = fTransactionsAll.get(theLong);
			if (theOb != null)
				return (ZTBTransRep) theOb;
			TransRep theTransRep = new TransRep(this, theTxnID);
			fTransactionsAll.put(theLong, theTransRep);
			iTxn.registerTarget(this);
			fTransactions_Create_Unsent.add(theTransRep);
			fCondition_Sender.broadcast();
			while (theTransRep.fServerID == 0)
				fCondition_Transaction.wait(fMutex_Structure);
			return theTransRep;
			}
		finally
			{
			fMutex_Structure.release();
			}
		}
	
	void runReader(InputStream iStream) throws IOException
		{
		for (;;)
			{
			this.reader(iStream);
			}
		}

	private void reader(InputStream iStream) throws IOException
		{
		// Must be called with fMutex_Structure not held
		ZTuple theResp = new ZTuple(iStream);
		fMutex_Structure.acquire();

		try
			{
			fTime_LastRead = System.currentTimeMillis();

			String theWhat = theResp.getString("What");
			if (theWhat.equals("Ping"))
				{
				fPingRequested = true;
				fCondition_Sender.broadcast();
				}
			else if (theWhat.equals("Pong"))
				{
				fPingSent = false;
				}
			else if (theWhat.equals("AllocateIDs_Ack"))
				{
				AllocatedID theAllocatedID = new AllocatedID(theResp.getInt64("BaseID"), theResp.getInt32("Count"));
				theAllocatedID.fNext = fIDsHead;
				fIDsHead = theAllocatedID;
				fCondition_ID.broadcast();
				}
			else if (theWhat.equals("Create_Ack"))
				{
				boolean broadcastTransaction = false;
				for (Iterator i = theResp.getListNoClone("IDs").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof Map))
						continue;

					ZTuple theTuple = new ZTuple((Map)theOb);
					Long clientID = new Long(theTuple.getInt64("ClientID"));
					TransRep theTrans = (TransRep)fTransactions_Create_Unacked.remove(clientID);
					if (theTrans == null)
						continue;

					theTrans.fServerID = theTuple.getInt64("ServerID");

					broadcastTransaction = true;
					}
				if (broadcastTransaction)
					fCondition_Transaction.broadcast();
				}
			else if (theWhat.equals("Validate_Succeeded"))
				{
				boolean broadcastTransaction = false;
				for (Iterator i = theResp.getListNoClone("ClientIDs").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof Long))
						continue;
					
					Long clientID = (Long)theOb;
					TransRep theTrans = (TransRep)fTransactions_Validate_Unacked.remove(clientID);
					theTrans.fValidateCompleted = true;
					theTrans.fValidateSucceeded = true;
					broadcastTransaction = true;
					}
				if (broadcastTransaction)
					fCondition_Transaction.broadcast();
				}
			else if (theWhat.equals("Validate_Failed"))
				{
				boolean broadcastTransaction = false;
				for (Iterator i = theResp.getListNoClone("ClientIDs").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof Long))
						continue;
					
					Long clientID = (Long)theOb;
					TransRep theTrans = (TransRep)fTransactions_Validate_Unacked.remove(clientID);
					fTransactionsAll.remove(clientID);
					theTrans.fValidateCompleted = true;
					theTrans.fValidateSucceeded = false;
					broadcastTransaction = true;
					}
				if (broadcastTransaction)
					fCondition_Transaction.broadcast();
				}
			else if (theWhat.equals("Commit_Ack"))
				{
				boolean broadcastTransaction = false;
				for (Iterator i = theResp.getListNoClone("ClientIDs").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof Long))
						continue;

					Long clientID = (Long)theOb;
					TransRep theTrans = (TransRep)fTransactions_Commit_Unacked.remove(clientID);
					fTransactionsAll.remove(clientID);
					theTrans.fCommitCompleted = true;
					broadcastTransaction = true;
					}
				if (broadcastTransaction)
					fCondition_Transaction.broadcast();
				}
			else if (theWhat.equals("GetTuples_Ack"))
				{
				boolean broadcastTuples = false;
				for (Iterator iterTrans = theResp.getListNoClone("Transactions").iterator(); iterTrans.hasNext();)
					{
					Object theOb = iterTrans.next();
					if (!(theOb instanceof ZTuple))
						continue;

					ZTuple theTransactionTuple = (ZTuple)theOb;
					TransRep theTrans = (TransRep)fTransactionsAll.get(new Long(theTransactionTuple.getInt64("ClientID")));

					for (Iterator iterIDValues = theTransactionTuple.getListNoClone("IDValues").iterator(); iterIDValues.hasNext();)
						{
						theOb = iterIDValues.next();
						if (!(theOb instanceof ZTuple))
							continue;

						ZTuple theTuple = (ZTuple)theOb;
						long theID = theTuple.getID("ID");
						TransTuple theTransTuple = this.internal_GetTransTuple(theTrans, theID);
						if (!theTransTuple.fWritten)
							{
							theTransTuple.fValueReturned = true;
							theTransTuple.fValue = theTuple.getTuple("Value");
							broadcastTuples = true;
							}
						}
					}
				if (broadcastTuples)
					fCondition_Tuples.broadcast();
				}
			else if (theWhat.equals("Search_Ack"))
				{
				boolean broadcastSearch = false;
				for (Iterator i = theResp.getListNoClone("Searches").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof ZTuple))
						continue;
					ZTuple theSearchTuple = (ZTuple)theOb;
					Long theSearchID = new Long(theSearchTuple.getInt64("SearchID"));
					TransSearch theSearch = (TransSearch)fSearches_Pending.remove(theSearchID);

					TransRep theTrans = theSearch.fTrans;
					List results = theSearchTuple.getListNoClone("Results");
					theSearch.fResults = new long[results.size()];
					for (int x = 0; x < results.size(); ++x)
						theSearch.fResults[x] = ((ZID)results.get(x)).longValue();
					broadcastSearch = true;
					}
				if (broadcastSearch)
					fCondition_Search.broadcast();
				}
			else if (theWhat.equals("Count_Ack"))
				{
				boolean broadcastCount = false;
				for (Iterator i = theResp.getListNoClone("Counts").iterator(); i.hasNext();)
					{
					Object theOb = i.next();
					if (!(theOb instanceof ZTuple))
						continue;
					ZTuple theCountTuple = (ZTuple)theOb;
					Long theCountID = new Long(theCountTuple.getInt64("CountID"));
					TransCount theCount = (TransCount)fCounts_Pending.remove(theCountID);

					TransRep theTrans = theCount.fTrans;
					List results = theCountTuple.getListNoClone("Results");
					theCount.fResult = theCountTuple.getInt64("Result");
					theCount.fWaiting = false;
					broadcastCount = true;
					}
				if (broadcastCount)
					fCondition_Count.broadcast();
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	static void sSend(ZMutex iMutex, OutputStream iStream, ZTuple iTuple) throws IOException
		{
		iMutex.release();
		try
			{
			iTuple.toStream(iStream);
			}
		finally
			{
			iMutex.acquire();
			}
		}

	private void writer(OutputStream iStream) throws IOException
		{
		// Must be called with fMutex_Structure held
		boolean didAnything = false;

		if (fPingRequested)
			{
			fPingRequested = false;
			ZTuple response = new ZTuple();
			response.setString("What", "Pong");
			sSend(fMutex_Structure, iStream, response);
			didAnything = true;
			}

		if (!fPingSent && fTime_LastRead + 10000 < System.currentTimeMillis())
			{
			fPingSent = true;
			ZTuple req = new ZTuple();
			req.setString("What", "Ping");
			sSend(fMutex_Structure, iStream, req);
			didAnything = true;
			}

		if (fIDsNeeded != 0)
			{
			ZTuple req = new ZTuple();
			req.setString("What", "AllocateIDs");
			req.setInt32("Count", fIDsNeeded);
			fIDsNeeded = 0;
			sSend(fMutex_Structure, iStream, req);
			didAnything = true;			
			}


		if (!fTransactions_Create_Unsent.isEmpty())
			{
			List vecClientIDs = new ArrayList();
			for (Iterator iterTrans = fTransactions_Create_Unsent.iterator(); iterTrans.hasNext();)
				{
				TransRep theTrans = (TransRep)iterTrans.next();
				Long theLong = new Long(theTrans.fClientID);
				vecClientIDs.add(theLong);
				fTransactions_Create_Unacked.put(theLong, theTrans);
				}
			fTransactions_Create_Unsent.clear();

			ZTuple reqTuple = new ZTuple();
			reqTuple.setString("What", "Create");
			reqTuple.setList("ClientIDs", vecClientIDs);
			sSend(fMutex_Structure, iStream, reqTuple);
			didAnything = true;
			}
		
		if (!fTransactions_Aborted_Unsent.isEmpty())
			{
			List vecServerIDs = new ArrayList();
			for (Iterator iterTrans = fTransactions_Aborted_Unsent.iterator(); iterTrans.hasNext();)
				{
				TransRep theTrans = (TransRep)iterTrans.next();
				vecServerIDs.add(new Long(theTrans.fServerID));
				// This is the point at which theTrans should no longer be referenced.
				}
			fTransactions_Aborted_Unsent.clear();

			ZTuple reqTuple = new ZTuple();
			reqTuple.setString("What", "Abort");
			reqTuple.setList("ServerIDs", vecServerIDs);
			sSend(fMutex_Structure, iStream, reqTuple);
			didAnything = true;
			}

		if (!fTransactions_Validate_Unsent.isEmpty())
			{
			if (fTransactions_NeedWork.isEmpty())
				{
				// We don't send the validate till we've disposed of pending work.
				List vecServerIDs = new ArrayList();

				for (Iterator iterTrans = fTransactions_Validate_Unsent.iterator(); iterTrans.hasNext();)
					{
					TransRep theTrans = (TransRep)iterTrans.next();
					vecServerIDs.add(new Long(theTrans.fServerID));
					fTransactions_Validate_Unacked.put(new Long(theTrans.fClientID), theTrans);
					}
				fTransactions_Validate_Unsent.clear();

				ZTuple reqTuple = new ZTuple();
				reqTuple.setString("What", "Validate");
				reqTuple.setList("ServerIDs", vecServerIDs);
				sSend(fMutex_Structure, iStream, reqTuple);
				}
			didAnything = true;
			}

		if (!fTransactions_Commit_Unsent.isEmpty())
			{
			List vecServerIDs = new ArrayList();
			for (Iterator iterTrans = fTransactions_Commit_Unsent.iterator(); iterTrans.hasNext();)
				{
				TransRep theTrans = (TransRep)iterTrans.next();
				vecServerIDs.add(new Long(theTrans.fServerID));
				fTransactions_Commit_Unacked.put(new Long(theTrans.fClientID), theTrans);
				}
			fTransactions_Commit_Unsent.clear();

			ZTuple reqTuple = new ZTuple();
			reqTuple.setString("What", "Commit");
			reqTuple.setList("ServerIDs", vecServerIDs);
			sSend(fMutex_Structure, iStream, reqTuple);
			didAnything = true;
			}

		while (!fTransactions_NeedWork.isEmpty())
			{
			TransRep theTrans = (TransRep)fTransactions_NeedWork.iterator().next();
			fTransactions_NeedWork.remove(theTrans);
			if (!theTrans.fTransTuples_NeedWork.isEmpty())
				{
				List vecGets = new ArrayList();
				List vecWrites = new ArrayList();

				for (Iterator iterTT = theTrans.fTransTuples_NeedWork.iterator(); iterTT.hasNext();)
					{
					TransTuple theTT = (TransTuple)iterTT.next();
					if (theTT.fWritten)
						{
						if (!theTT.fWriteSent)
							{
							theTT.fWriteSent = true;
							ZTuple theWriteTuple = new ZTuple();
							theWriteTuple.setID("ID", theTT.fID);
							theWriteTuple.setTuple("Value", theTT.fValue);
							vecWrites.add(theWriteTuple);
							}
						}
					else
						{
						if (!theTT.fRequestSent)
							{
							theTT.fRequestSent = true;
							// Ask the server to send the tuple.
							vecGets.add(new ZID(theTT.fID));
							}
						}
					}

				theTrans.fTransTuples_NeedWork.clear();

				if (!vecGets.isEmpty() || !vecWrites.isEmpty())
					{
					ZTuple reqTuple = new ZTuple();
					reqTuple.setString("What", "Actions");
					reqTuple.setInt64("ServerID", theTrans.fServerID);
					if (!vecGets.isEmpty())
						reqTuple.setList("Gets", vecGets);
					if (!vecWrites.isEmpty())
						reqTuple.setList("Writes", vecWrites);
					sSend(fMutex_Structure, iStream, reqTuple);
					didAnything = true;
					}
				}

			if (!theTrans.fSearches_Unsent.isEmpty())
				{
				ZTuple reqTuple = new ZTuple();
				reqTuple.setString("What", "Search");
				reqTuple.setInt64("ServerID", theTrans.fServerID);

				List vecSearches = new ArrayList();
				
				for (Iterator iterSearch = theTrans.fSearches_Unsent.iterator(); iterSearch.hasNext();)
					{
					TransSearch theSearch = (TransSearch)iterSearch.next();

					ZTuple theTuple = new ZTuple();
					theTuple.setInt64("SearchID", theSearch.fSearchID);
					theTuple.setTuple("QueryAsTuple", theSearch.fQueryAsTuple);
					vecSearches.add(theTuple);
					fSearches_Pending.put(new Long(theSearch.fSearchID), theSearch);
					}
				reqTuple.setList("Searches", vecSearches);
				theTrans.fSearches_Unsent.clear();
				sSend(fMutex_Structure, iStream, reqTuple);
				didAnything = true;
				}

			if (!theTrans.fCounts_Unsent.isEmpty())
				{
				ZTuple reqTuple = new ZTuple();
				reqTuple.setString("What", "Count");
				reqTuple.setInt64("ServerID", theTrans.fServerID);

				List vecCounts = new ArrayList();
				
				for (Iterator iterCount = theTrans.fCounts_Unsent.iterator(); iterCount.hasNext();)
					{
					TransCount theCount = (TransCount)iterCount.next();

					ZTuple theTuple = new ZTuple();
					theTuple.setInt64("CountID", theCount.fCountID);
					theTuple.setTuple("QueryAsTuple", theCount.fQueryAsTuple);
					vecCounts.add(theTuple);
					fCounts_Pending.put(new Long(theCount.fCountID), theCount);
					}
				reqTuple.setList("Counts", vecCounts);
				theTrans.fCounts_Unsent.clear();
				sSend(fMutex_Structure, iStream, reqTuple);
				didAnything = true;
				}
			}

		if (!didAnything)
			{
			iStream.flush();			
			fCondition_Sender.wait(fMutex_Structure, 1000);
			}
		}

	void runWriter(OutputStream iStream) throws IOException
		{
		fMutex_Structure.acquire();
		try
			{
			for (;;)
				{
				this.writer(iStream);
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	public final long[] transSearch(TransRep iTrans, ZTBQuery iQuery)
		{
		fMutex_Structure.acquire();

		TransSearch theSearch = new TransSearch(iTrans, fNextSearchID++, iQuery);

		iTrans.fSearches_Unsent.add(theSearch);

		try
			{
			fTransactions_NeedWork.add(iTrans);
			fCondition_Sender.broadcast();
			while (theSearch.fResults == null)
				{
				try
					{ fCondition_Search.wait(fMutex_Structure); }
				catch (Exception ex)
					{}
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		return theSearch.fResults;
		}

	public final long transCount(TransRep iTrans, ZTBQuery iQuery)
		{
		fMutex_Structure.acquire();

		TransCount theCount = new TransCount(iTrans, fNextCountID++, iQuery);

		iTrans.fCounts_Unsent.add(theCount);

		try
			{
			fTransactions_NeedWork.add(iTrans);
			fCondition_Sender.broadcast();
			while (theCount.fWaiting)
				{
				try
					{ fCondition_Count.wait(fMutex_Structure); }
				catch (Exception ex)
					{}
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		return theCount.fResult;
		}

	final void transSet(TransRep iTrans, long iID, Map iMap)
		{
		if (iID == 0)
			return;
		fMutex_Structure.acquire();
		try
			{
			TransTuple theTT = this.internal_GetTransTuple(iTrans, iID);
			if (iMap == null)
				theTT.fValue = new ZTuple();
			else if (iMap instanceof ZTuple)
				theTT.fValue = (ZTuple)iMap;
			else
				theTT.fValue = new ZTuple(iMap);
			theTT.fWritten = true;
			theTT.fWriteSent = false;
			iTrans.fTransTuples_NeedWork.add(theTT);
			fTransactions_NeedWork.add(iTrans);
			fCondition_Sender.broadcast();
			}
		finally
			{
			fMutex_Structure.release();
			}
		}

	final ZTuple[] transGet(TransRep iTrans, long iIDs[])
		{
		ZTuple result[] = new ZTuple[iIDs.length];
		List neededIDs = new ArrayList();
		fMutex_Structure.acquire();
		try
			{
			for (int x = 0; x < iIDs.length; ++x)
				{
				long theID = iIDs[x];
				if (theID == 0)
					{
					result[x] = new ZTuple();
					}
				else
					{
					TransTuple theTT = this.internal_GetTransTuple(iTrans, theID);
					while (!theTT.fValueReturned)
						{
						if (!theTT.fRequestSent)
							{
							iTrans.fTransTuples_NeedWork.add(theTT);
							fTransactions_NeedWork.add(iTrans);
							fCondition_Sender.broadcast();
							}
						fCondition_Tuples.wait(fMutex_Structure);
						}
					result[x] = theTT.fValue;
					}
				}
			}
		finally
			{
			fMutex_Structure.release();
			}
		return result;
		}

	private TransTuple internal_GetTransTuple(TransRep iTrans, long iID)
		{
		Long idAsKey = new Long(iID);
		Object theOb = iTrans.fTransTuples.get(idAsKey);
		if (theOb != null)
			return (TransTuple)theOb;

		TransTuple theTT = new TransTuple(iID);
		iTrans.fTransTuples.put(idAsKey, theTT);
		return theTT;
		}

	private long fTime_LastRead = 0;

	private final ZMutex fMutex_Structure = new ZMutex();

	private final ZCondition fCondition_Sender = new ZCondition();
	private final ZCondition fCondition_Search = new ZCondition();
	private final ZCondition fCondition_Count = new ZCondition();
	private final ZCondition fCondition_ID = new ZCondition();
	private final ZCondition fCondition_Transaction = new ZCondition();
	private final ZCondition fCondition_Tuples = new ZCondition();

 	private boolean fPingRequested = false;
	private boolean fPingSent = false;

	private int fIDsNeeded = 0;
	private AllocatedID fIDsHead = null;

	private final Map fTransactionsAll = new TreeMap();

	private final ArrayList fTransactions_Create_Unsent = new ArrayList();
	private final Map fTransactions_Create_Unacked = new TreeMap();

	private final ArrayList fTransactions_Validate_Unsent = new ArrayList();
	private final Map fTransactions_Validate_Unacked = new TreeMap();

	private final ArrayList fTransactions_Aborted_Unsent = new ArrayList();

	private final ArrayList fTransactions_Commit_Unsent = new ArrayList();
	private final Map fTransactions_Commit_Unacked = new TreeMap();

	private final ArrayList fTransactions_NeedWork = new ArrayList();

	private int fNextSearchID = 1;
	private final Map fSearches_Pending = new java.util.TreeMap();

	private int fNextCountID = 1;
	private final Map fCounts_Pending = new java.util.TreeMap();

	private static final class TransTuple
		{
		TransTuple(long iID)
			{
			fID = iID;
			}

		final long fID;
		ZTuple fValue;
		boolean fRequestSent = false;
		boolean fValueReturned = false;
		boolean fWritten = false;
		boolean fWriteSent = false;
		};

	private static final class TransSearch
		{
		TransSearch(TransRep iTrans, long iSearchID, ZTBQuery iQuery)
			{
			fTrans = iTrans;
			fSearchID = iSearchID;
			fQueryAsTuple = iQuery.asTuple();
			}

		final TransRep fTrans;
		final long fSearchID;
		final Map fQueryAsTuple;

		long fResults[];
		};

	private static final class TransCount
		{
		TransCount(TransRep iTrans, long iCountID, ZTBQuery iQuery)
			{
			fTrans = iTrans;
			fCountID = iCountID;
			fQueryAsTuple = iQuery.asTuple();
			fWaiting = true;
			}

		final TransRep fTrans;
		final long fCountID;
		final Map fQueryAsTuple;

		boolean fWaiting;
		long fResult;
		};

	private static final class AllocatedID
		{
		AllocatedID(long iBase, int iCount)
			{
			fBase = iBase;
			fCount = iCount;
			}

		long fBase;
		int fCount;
		AllocatedID fNext;
		};

	private static final class TransRep extends ZTBTransRep
		{
		TransRep(ZTB_Client iTB, long iClientID)
			{
			fTB_Client = iTB;
			fClientID = iClientID;
			}

		public long[] search(ZTBQuery iQuery)
			{ return fTB_Client.transSearch(this, iQuery); }
		
		public long count(ZTBQuery iQuery)
			{ return fTB_Client.transCount(this, iQuery); }

		public void setTuple(long iID, Map iMap)
			{ fTB_Client.transSet(this, iID, iMap); }

		public ZTuple[] getTuples(long[] iIDs)
			{ return fTB_Client.transGet(this, iIDs); }

		final ZTB_Client fTB_Client;

		final long fClientID;

		long fServerID = 0;

		boolean fValidateCompleted = false;
		boolean fValidateSucceeded = false;
		boolean fCommitCompleted = false;

		final Map fTransTuples = new TreeMap();
		final Set fTransTuples_NeedWork = new HashSet();

		final ArrayList fSearches_Unsent = new ArrayList();

		final ArrayList fCounts_Unsent = new ArrayList();
		};
	}

// ====================================================================================================
