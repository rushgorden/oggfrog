// "@(#) $Id: ZTBTxn.java,v 1.7 2005/05/09 06:05:36 agreen Exp $";

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

import java.util.Map;
import org.zoolib.*;

public class ZTBTxn
	{
	public ZTBTxn(ZTxn iTxn, ZTB iTB)
		{
		fTxn = iTxn;
		fTB = iTB;
		}

	public ZTBTransRep getTransRep()
		{
		return fTB.getTransRep(fTxn);
		}

	public long allocateID()
		{
		return fTB.allocateID();
		}

	public ZID allocateZID()
		{
		return fTB.allocateZID();
		}

	public ZID add(Map iMap)
		{
		return fTB.add(fTxn, iMap);
		}

	public void set(ZID iID, Map iMap)
		{
		fTB.set(fTxn, iID, iMap);
		}

	public void set(long iID, Map iMap)
		{
		fTB.set(fTxn, iID, iMap);
		}

	public ZTuple get(ZID iID)
		{
		return fTB.get(fTxn, iID);
		}

	public ZTuple get(long iID)
		{
		return fTB.get(fTxn, iID);
		}

	public final long count(ZTBQuery iQuery)
		{
		return fTB.count(fTxn, iQuery);
		}

	public final long[] search(ZTBQuery iQuery)
		{
		return fTB.search(fTxn, iQuery);
		}

	private final ZTxn fTxn;
	private final ZTB fTB;
	}
