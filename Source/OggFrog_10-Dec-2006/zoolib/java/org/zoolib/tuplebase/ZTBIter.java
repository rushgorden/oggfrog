// "@(#) $Id: ZTBIter.java,v 1.8 2005/04/25 17:55:49 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
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

import java.util.*;
import org.zoolib.ZID;
import org.zoolib.ZTuple;
import org.zoolib.ZTxn;

public class ZTBIter implements java.util.Iterator, Cloneable
	{
	public ZTBIter(ZTBIter iOther)
		{
		fIDs = iOther.fIDs;
		fIndex = iOther.fIndex;
		fTBTransRep = iOther.fTBTransRep;
		}

	public ZTBIter(ZTxn iTxn, ZTB iTB, ZTBSpec iSpec)
		{
		fTBTransRep = iTB.getTransRep(iTxn);
		fIDs = fTBTransRep.search(new ZTBQuery(iSpec));
		fIndex = 0;
		while (fIndex < fIDs.length && fIDs[fIndex] == 0)
			++fIndex;
		}

	public ZTBIter(ZTxn iTxn, ZTB iTB, ZTBQuery iQuery)
		{
		fTBTransRep = iTB.getTransRep(iTxn);
		fIDs = fTBTransRep.search(iQuery);
		fIndex = 0;
		while (fIndex < fIDs.length && fIDs[fIndex] == 0)
			++fIndex;
		}

	public ZTBIter(ZTBTxn iTBTxn, ZTBSpec iSpec)
		{
		fTBTransRep = iTBTxn.getTransRep();
		fIDs = fTBTransRep.search(new ZTBQuery(iSpec));
		fIndex = 0;
		while (fIndex < fIDs.length && fIDs[fIndex] == 0)
			++fIndex;
		}

	public ZTBIter(ZTBTxn iTBTxn, ZTBQuery iQuery)
		{
		fTBTransRep = iTBTxn.getTransRep();
		fIDs = fTBTransRep.search(iQuery);
		fIndex = 0;
		while (fIndex < fIDs.length && fIDs[fIndex] == 0)
			++fIndex;
		}

	public Object clone()
		{
		return new ZTBIter(this);
		}

// Our protocol, akin to the C++ API.
	public final boolean hasValue()
		{
		return fIndex < fIDs.length;
		}

	public final void advance()
		{
		if (fIndex < fIDs.length)
			++fIndex;
		while (fIndex < fIDs.length && fIDs[fIndex] == 0)
			++fIndex;
		}

	public final int advanceAll()
		{
		int count = 0;
		while (fIndex < fIDs.length)
			{
			if (fIDs[fIndex] != 0)
				++count;
			++fIndex;
			}
		return count;
		}

	public final ZTuple get()
		{
		if (fIndex < fIDs.length)
			return fTBTransRep.get(fIDs[fIndex]);
		return null;
		}

	public final ZTuple getTuple()
		{
		if (fIndex < fIDs.length)
			return fTBTransRep.get(fIDs[fIndex]);
		return null;
		}

	public final long getID()
		{
		if (fIndex < fIDs.length)
			return fIDs[fIndex];
		return 0;
		}

	public final ZID getZID()
		{
		if (fIndex < fIDs.length)
			return new ZID(fIDs[fIndex]);
		return new ZID();
		}

	public final void set(Map iMap)
		{
		if (fIndex < fIDs.length && fIDs[fIndex] != 0)
			fTBTransRep.setTuple(fIDs[fIndex], iMap);
		}

	public final void erase()
		{
		this.set(new ZTuple());
		}

	public final int eraseAll()
		{
		int count = 0;
		ZTuple emptyTuple = new ZTuple();
		while (fIndex < fIDs.length)
			{
			if (fIDs[fIndex] != 0)
				{
				fTBTransRep.setTuple(fIDs[fIndex], emptyTuple);
				++count;
				}
			++fIndex;
			}
		return count;
		}

// From Iterator
	// It's not highly recommended that you use the Iterator interface. It's
	// just here in case it's needed. The ZTBIter API provides accessors for
	// both the current tuple and the current ID.

	public boolean hasNext()
		{
		return this.hasValue();
		}

	public Object next()
		{
		// Perhaps should return a pair of some sort.
		Map result = this.get();
		this.advance();
		return result;
		}

	public void remove()
		{
		throw new UnsupportedOperationException();
		}

	private final ZTBTransRep fTBTransRep;
	private final long fIDs[];
	private int fIndex;
	};
