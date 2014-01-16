// "@(#) $Id: ZTxn.java,v 1.4 2004/07/16 15:57:24 agreen Exp $";

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

import org.zoolib.ZDebug;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public final class ZTxn
	{
	public ZTxn()
		{}

	public void finalize()
		{
		// We don't have reliable invocation of destructors in java, so
		// you *MUST* call abort or commit on any transaction you create.
		ZDebug.sAssert(fEnded);
		}

	public final long getID()
		{
		return fID;
		}

	public final synchronized void registerTarget(ZTxnTarget iTarget)
		{
		fTargets.add(iTarget);
		}

	public final synchronized void abort()
		{
		fEnded = true;
		for (Iterator i = fTargets.iterator(); i.hasNext();)
			{
			ZTxnTarget theTarget = (ZTxnTarget)i.next();
			theTarget.TxnAbortPreValidate(fID);
			}
		}

	public final synchronized boolean commit()
		{
		fEnded = true;
		for (Iterator i = fTargets.iterator(); i.hasNext();)
			{
			ZTxnTarget theTarget = (ZTxnTarget)i.next();
			if (theTarget.TxnValidate(fID))
				continue;

			// We failed to validate. So abort all the prior ones.
			for (Iterator iterFirst = fTargets.iterator(); iterFirst.hasNext();)
				{
				ZTxnTarget current = (ZTxnTarget)iterFirst.next();
				if (current == theTarget)
					break;
				current.TxnAbortPostValidate(fID);
				}

			for (;i.hasNext();)
				((ZTxnTarget)i.next()).TxnAbortPreValidate(fID);

			return false;
			}

		for (Iterator i = fTargets.iterator(); i.hasNext();)
			{
			ZTxnTarget theTarget = (ZTxnTarget)i.next();
			theTarget.TxnCommit(fID);
			}

		return true;
		}

	final long fID = sNextID();
	final Set fTargets = new HashSet();

	static long sID = 1;
	private synchronized static long sNextID()
		{ return ++sID; }

	private boolean fEnded = false;
	}
