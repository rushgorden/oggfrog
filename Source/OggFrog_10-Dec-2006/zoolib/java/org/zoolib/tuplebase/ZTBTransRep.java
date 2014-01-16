// "@(#) $Id: ZTBTransRep.java,v 1.2 2005/05/09 06:05:36 agreen Exp $";

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

import java.util.Map;
import org.zoolib.ZID;
import org.zoolib.ZTuple;

abstract public class ZTBTransRep
	{
	public abstract long[] search(ZTBQuery iQuery);

	public abstract long count(ZTBQuery iQuery);

	public abstract void setTuple(long iID, Map iMap);

	public abstract ZTuple[] getTuples(long iID[]);

	public ZTuple get(long iID)
		{
		if (iID == 0)
			return null;

		long idArray[] = new long[1];
		idArray[0] = iID;
		ZTuple results[] = this.getTuples(idArray);

		return results[0];
		}
	}
