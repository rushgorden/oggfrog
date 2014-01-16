// "@(#) $Id: ZTSieve.java,v 1.2 2006/10/23 21:24:50 agreen Exp $";

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

import java.util.List;
import java.util.Set;

// ====================================================================================================

public class ZTSieve
	{
	// Our protocol
	public final void register(ZTSoup iTSoup, ZTBQuery iTBQuery)
		{ iTSoup.pRegisterTSieve(this, iTBQuery); }

	public final void register(ZTSoup iTSoup, ZTBSpec iTBSpec)
		{ iTSoup.pRegisterTSieve(this, new ZTBQuery(iTBSpec)); }

	public void close()
		{ fPSieve.fTSoup.pCloseTSieve(this); }

	public void changed()
		{}

	public final boolean hasCurrent()
		{ return fPSieve.fHasResults_Current; }

	public final boolean hasPrior()
		{ return fPSieve.fHasResults_Prior; }

	public final List getCurrent()
		{ return fPSieve.fResults_Local_Current; }

	public final List getPrior()
		{ return fPSieve.fResults_Local_Prior; }

	public final Set getAdded()
		{ return fPSieve.getAdded(); }

	public final Set getRemoved()
		{ return fPSieve.getRemoved(); }

	public final ZTBQuery getTBQuery()
		{ return fPSieve.fTBQuery; }

	ZTSoup.PSieve fPSieve;

	final DList.Link fLink_PSieve = new DList.Link(this);
	};
