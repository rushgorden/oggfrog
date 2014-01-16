// "@(#) $Id: ZID.java,v 1.3 2004/06/24 22:29:08 eric_cooper Exp $";

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

package org.zoolib;

import java.io.ObjectOutputStream;
import java.io.IOException;
import java.io.Serializable;
import java.lang.Comparable;
import java.lang.Long;
import java.lang.Object;

public class ZID extends Object implements Serializable, Comparable
	{
	public ZID()
		{
		fValue = 0;
		}

	public ZID(long iValue)
		{
		fValue = iValue;
		}

	public long longValue()
		{
		return fValue;
		}

	public boolean equals( Object obj )
		{
		if (!(obj instanceof ZID))
			return false ;

		return this.fValue == ((ZID)obj).fValue;
		}

	public String toString()
		{
//		return new String(getClass().getName() + ":" + fValue + " [0x" + Long.toHexString(fValue).toUpperCase() + "]");
		return Long.toString(fValue);
		}

	// Serializable methods
	private void writeObject(java.io.ObjectOutputStream out) throws IOException
		{
		out.writeLong(fValue);
		}

	private void readObject(java.io.ObjectInputStream in) throws IOException, ClassNotFoundException
		{
		fValue = in.readLong();
		}

	// Comparable method
	public int compareTo(Object obj)
		{
		if (!(obj instanceof ZID))
			return 1; // ZIDs will be less than non-ZIDs.
		long otherValue = ((ZID)obj).longValue();
		if (fValue < otherValue)
			return -1;
		if (fValue > otherValue)
			return 1;
		return 0; // equal.
		}

	private long fValue;
	}
