// "@(#) $Id: ZTSWatcher_Client.java,v 1.7 2006/10/30 06:17:01 agreen Exp $";

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

import org.zoolib.ZID;
import org.zoolib.ZTuple;
import org.zoolib.ZUtil_IO;
import org.zoolib.thread.ZMutex;

import java.io.ByteArrayOutputStream;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;

import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ZTSWatcher_Client extends ZTSWatcher
	{
	public ZTSWatcher_Client(boolean iSupports2, boolean iSupports3, boolean iSupports4, InputStream iInputStream, OutputStream iOutputStream)
		{
		fSupports2 = iSupports2;
		fSupports3 = iSupports3;
		fSupports4 = iSupports4;
		fInputStream = iInputStream;
		fOutputStream = iOutputStream;
		}

	public void close()
		{
		fMutex.acquire();

		ZTuple req = new ZTuple();
		req.setString("What", "Close");
		this.pSend(req);

		try
			{
			fOutputStream.flush();
			fOutputStream.close();
			fInputStream.close();
			}
		catch (Exception ex)
			{}

		fOutputStream = null;
		fInputStream = null;

		fMutex.release();
		}

	public long allocateID()
		{
		// Note that fMutex is released from inside the infinite loop.
		fMutex.acquire();
		for (;;)
			{
			while (fIDsHead != null)
				{
				if (fIDsHead.fCount > 0)
					{
					long result = fIDsHead.fBase;
					--fIDsHead.fCount;
					++fIDsHead.fBase;
					fMutex.release(); // <---
					return result;
					}
				fIDsHead = fIDsHead.fNext;
				}
			ZTuple req = new ZTuple();
			req.setString("What", "AllocateIDs");
			req.setInt32("Count", 10);
			pSend(req);

			ZTuple theResp = pReceive();
			AllocatedID theAllocatedID = new AllocatedID(theResp.getInt64("BaseID"), theResp.getInt32("Count"));
			theAllocatedID.fNext = fIDsHead;
			fIDsHead = theAllocatedID;
			}
		}

	public void doIt(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Set oAddedIDs,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries)
		{
		try
			{
			if (fSupports4)
				{
				this.pDoIt4(iRemovedIDs, iAddedIDs,
						iRemovedQueries, iAddedQueries,
						oAddedIDs,
						oChangedTuples,
						iWrittenTuples,
						oChangedQueries);
				}
			else if (fSupports3)
				{
				this.pDoIt3(iRemovedIDs, iAddedIDs,
						iRemovedQueries, iAddedQueries,
						oChangedTuples,
						iWrittenTuples,
						oChangedQueries);
				}
			else if (fSupports2)
				{
				this.pDoIt2(iRemovedIDs, iAddedIDs,
						iRemovedQueries, iAddedQueries,
						oChangedTuples,
						iWrittenTuples,
						oChangedQueries);
				}
			else
				{
				this.pDoIt1(iRemovedIDs, iAddedIDs,
						iRemovedQueries, iAddedQueries,
						oAddedIDs,
						oChangedTuples,
						iWrittenTuples,
						oChangedQueries);
				}
			}
		catch (IOException ex)
			{}
		}

	private final void pDoIt1(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Set oAddedIDs,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries)
		{
		fMutex.acquire();

		ZTuple req = new ZTuple();
		req.setString("What", "DoIt");

		if (iRemovedIDs != null && !iRemovedIDs.isEmpty())
			req.setList("removedIDs", sSetToList(iRemovedIDs));

		if (iAddedIDs != null && !iAddedIDs.isEmpty())
			req.setList("addedIDs", sSetToList(iAddedIDs));

		if (iRemovedQueries != null && !iRemovedQueries.isEmpty())
			req.setList("removedQueries", sSetToList(iRemovedQueries));

		if (iAddedQueries != null && !iAddedQueries.isEmpty())
			{
			List theList = new ArrayList();

			for (Iterator i = iAddedQueries.entrySet().iterator(); i.hasNext(); /*no increment*/)
				{
				Map.Entry theEntry = (Map.Entry)i.next();
				ZTuple aTuple = new ZTuple();
				aTuple.setValue("refcon", theEntry.getKey());
				aTuple.setValue("query", ((ZTBQuery)theEntry.getValue()).asTuple());
				aTuple.setBool("prefetch", true);
				theList.add(aTuple);
				}

			req.setList("addedQueries", theList);
			}

		if (iWrittenTuples != null && !iWrittenTuples.isEmpty())
			req.setList("writtenTuples", sMapToList("ID", "tuple", iWrittenTuples));

		pSend(req);
		ZTuple theResp = pReceive();

		for (Iterator i = theResp.getListNoClone("addedTuples").iterator(); i.hasNext(); /*no increment*/)
			oAddedIDs.add(i.next());

		List changedTuplesV = theResp.getListNoClone("changedTuples");
		for (Iterator i = changedTuplesV.iterator(); i.hasNext(); /*no increment*/)
			{
			ZTuple entry = (ZTuple)i.next();
			ZID theZID = entry.getZID("ID");
			ZTuple theValue = entry.getTuple("tuple");
			oChangedTuples.put(theZID, theValue);
			}

		List changedQueriesV = theResp.getListNoClone("changedQueries");
		for (Iterator i = changedQueriesV.iterator(); i.hasNext(); /*no increment*/)
			{
			ZTuple entry = (ZTuple)i.next();
			Long theRefcon = entry.getLong("refcon");
			List theList = entry.getList("IDs");
			oChangedQueries.put(theRefcon, theList);
			}

		fMutex.release();
		}

	// pDoIt2 and pDoIt3 are very similar -- version 3 puts the length of
	// a queries data before the data itself, so the receiver does not
	// have to parse the data in order to read it.
	
	private final void pDoIt2(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries) throws IOException
		{
		fMutex.acquire();
		ZTuple req = new ZTuple();
		req.setString("What", "NewDoIt");
		pSend(req);

		DataOutput theDO = new DataOutputStream(fOutputStream);

		ZUtil_IO.sWriteCount(theDO, iRemovedIDs.size());
		for (Iterator i = iRemovedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedIDs.size());
		for (Iterator i = iAddedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iRemovedQueries.size());
		for (Iterator i = iRemovedQueries.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((Long)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedQueries.size());
		for (Iterator i = iAddedQueries.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((Long)theEntry.getKey()).longValue());
			((ZTBQuery)theEntry.getValue()).toDataOutput(theDO);
			}

		ZUtil_IO.sWriteCount(theDO, iWrittenTuples.size());
		for (Iterator i = iWrittenTuples.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((ZID)theEntry.getKey()).longValue());
			((ZTuple)theEntry.getValue()).toDataOutput(theDO);
			}

		fOutputStream.flush();
		DataInput theDI = new DataInputStream(fInputStream);

		int countChangedTuples = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedTuples; ++x)
			{
			long theID = theDI.readLong();
			ZTuple theTuple = new ZTuple(theDI);
			oChangedTuples.put(new ZID(theID), theTuple);
			}

		int countChangedQueries = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedQueries; ++x)
			{
			long theRefcon = theDI.readLong();
			int countIDs = ZUtil_IO.sReadCount(theDI);
			List theList = new ArrayList();
			for (int y = 0; y < countIDs; ++y)
				theList.add(new ZID(theDI.readLong()));
			oChangedQueries.put(new Long(theRefcon), theList);
			}

		fMutex.release();
		}

	private final void pDoIt3(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries) throws IOException
		{
		fMutex.acquire();
		ZTuple req = new ZTuple();
		req.setString("What", "DoIt3");
		pSend(req);

		DataOutput theDO = new DataOutputStream(fOutputStream);

		ZUtil_IO.sWriteCount(theDO, iRemovedIDs.size());
		for (Iterator i = iRemovedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedIDs.size());
		for (Iterator i = iAddedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iRemovedQueries.size());
		for (Iterator i = iRemovedQueries.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((Long)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedQueries.size());
		for (Iterator i = iAddedQueries.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((Long)theEntry.getKey()).longValue());
			
			// Have the query write itself into a byte array.
			ByteArrayOutputStream theBAOS = new ByteArrayOutputStream();
			DataOutput tempDO = new DataOutputStream(theBAOS);

			((ZTBQuery)theEntry.getValue()).toDataOutput(new DataOutputStream(theBAOS));

			byte[] theData = theBAOS.toByteArray();

			// Whose length we can send ahead of the data itself.
			ZUtil_IO.sWriteCount(theDO, theData.length);
			theDO.write(theData);
			}

		ZUtil_IO.sWriteCount(theDO, iWrittenTuples.size());
		for (Iterator i = iWrittenTuples.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((ZID)theEntry.getKey()).longValue());
			((ZTuple)theEntry.getValue()).toDataOutput(theDO);
			}

		fOutputStream.flush();
		DataInput theDI = new DataInputStream(fInputStream);

		int countChangedTuples = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedTuples; ++x)
			{
			long theID = theDI.readLong();
			ZTuple theTuple = new ZTuple(theDI);
			oChangedTuples.put(new ZID(theID), theTuple);
			}

		int countChangedQueries = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedQueries; ++x)
			{
			long theRefcon = theDI.readLong();
			int countIDs = ZUtil_IO.sReadCount(theDI);
			List theList = new ArrayList();
			for (int y = 0; y < countIDs; ++y)
				theList.add(new ZID(theDI.readLong()));
			oChangedQueries.put(new Long(theRefcon), theList);
			}

		fMutex.release();
		}

	private final void pDoIt4(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Set oAddedIDs,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries) throws IOException
		{
		fMutex.acquire();
		ZTuple req = new ZTuple();
		req.setString("What", "DoIt4");
		pSend(req);

		DataOutput theDO = new DataOutputStream(fOutputStream);

		ZUtil_IO.sWriteCount(theDO, iRemovedIDs.size());
		for (Iterator i = iRemovedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedIDs.size());
		for (Iterator i = iAddedIDs.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((ZID)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iRemovedQueries.size());
		for (Iterator i = iRemovedQueries.iterator(); i.hasNext(); /*no increment*/)
			theDO.writeLong(((Long)i.next()).longValue());

		ZUtil_IO.sWriteCount(theDO, iAddedQueries.size());
		for (Iterator i = iAddedQueries.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((Long)theEntry.getKey()).longValue());
			// Prefetch queries, for we're assuming all queries are to be prefetched.
			theDO.writeByte(1);
			
			// Have the query write itself into a byte array.
			ByteArrayOutputStream theBAOS = new ByteArrayOutputStream();
			DataOutput tempDO = new DataOutputStream(theBAOS);

			((ZTBQuery)theEntry.getValue()).toDataOutput(new DataOutputStream(theBAOS));

			byte[] theData = theBAOS.toByteArray();

			// Whose length we can send ahead of the data itself.
			ZUtil_IO.sWriteCount(theDO, theData.length);
			theDO.write(theData);
			}

		ZUtil_IO.sWriteCount(theDO, iWrittenTuples.size());
		for (Iterator i = iWrittenTuples.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			theDO.writeLong(((ZID)theEntry.getKey()).longValue());
			((ZTuple)theEntry.getValue()).toDataOutput(theDO);
			}

		fOutputStream.flush();
		DataInput theDI = new DataInputStream(fInputStream);

		int countAddedIDs = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countAddedIDs; ++x)
			oAddedIDs.add(new ZID(theDI.readLong()));

		int countChangedTuples = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedTuples; ++x)
			{
			long theID = theDI.readLong();
			ZTuple theTuple = new ZTuple(theDI);
			oChangedTuples.put(new ZID(theID), theTuple);
			}

		int countChangedQueries = ZUtil_IO.sReadCount(theDI);
		for (int x = 0; x < countChangedQueries; ++x)
			{
			long theRefcon = theDI.readLong();
			int countIDs = ZUtil_IO.sReadCount(theDI);
			List theList = new ArrayList();
			for (int y = 0; y < countIDs; ++y)
				theList.add(new ZID(theDI.readLong()));
			oChangedQueries.put(new Long(theRefcon), theList);
			}

		fMutex.release();
		}

	private final void pSend(ZTuple iTuple)
		{
		try
			{
			iTuple.toStream(fOutputStream);
			fOutputStream.flush();
			}
		catch (Exception ex)
			{}
		}

	private final ZTuple pReceive()
		{
		try
			{
			ZTuple result = new ZTuple(fInputStream);
			return result;
			}
		catch (Exception ex)
			{}
		return null;
		}

	private static final List sSetToList(Set iSet)
		{
		List theList = new ArrayList();
		for (Iterator i = iSet.iterator(); i.hasNext(); /*no increment*/)
			theList.add(i.next());
		return theList;
		}

	private static final List sMapToList(String iKeyName, String iValueName, Map iMap)
		{
		List theList = new ArrayList();

		for (Iterator i = iMap.entrySet().iterator(); i.hasNext(); /*no increment*/)
			{
			Map.Entry theEntry = (Map.Entry)i.next();
			ZTuple aTuple = new ZTuple();
			aTuple.setValue(iKeyName, theEntry.getKey());
			aTuple.setValue(iValueName, theEntry.getValue());
			theList.add(aTuple);
			}
		return theList;
		}

	private final ZMutex fMutex = new ZMutex();
	private final boolean fSupports2;
	private final boolean fSupports3;
	private final boolean fSupports4;
	private InputStream fInputStream;
	private OutputStream fOutputStream;
	private AllocatedID fIDsHead;

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
	}
