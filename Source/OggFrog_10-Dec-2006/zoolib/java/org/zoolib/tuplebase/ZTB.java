// "@(#) $Id: ZTB.java,v 1.8 2005/07/29 15:17:21 agreen Exp $";

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

abstract public class ZTB
	{
	public abstract long allocateID();

	public ZID allocateZID()
		{ return new ZID(this.allocateID()); }

	public final ZID add(ZTxn iTxn, Map iMap)
		{
		ZID theID = this.allocateZID();
		this.getTransRep(iTxn).setTuple(theID.longValue(), iMap);
		return theID;
		}

	public final void set(ZTxn iTxn, ZID iID, Map iMap)
		{
		if (iID == null)
			return;

		long theLong = iID.longValue();
		if (theLong == 0)
			return;

		this.getTransRep(iTxn).setTuple(theLong, iMap);
		}

	public final void set(ZTxn iTxn, long iID, Map iMap)
		{
		if (iID == 0)
			return;

		this.getTransRep(iTxn).setTuple(iID, iMap);
		}

	public final ZTuple get(ZTxn iTxn, ZID iID)
		{
		if (iID == null)
			return null;

		long theLong = iID.longValue();
		if (theLong == 0)
			return null;

		return this.getTransRep(iTxn).get(theLong);
		}

	public final ZTuple get(ZTxn iTxn, long iID)
		{
		if (iID == 0)
			return null;

		return this.getTransRep(iTxn).get(iID);
		}

	public final long[] search(ZTxn iTxn, ZTBQuery iQuery)
		{
		return this.getTransRep(iTxn).search(iQuery);
		}

	public final long count(ZTxn iTxn, ZTBQuery iQuery)
		{
		return this.getTransRep(iTxn).count(iQuery);
		}

	abstract ZTBTransRep getTransRep(ZTxn iTxn);
	}
