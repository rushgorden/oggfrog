// "@(#) $Id: ZMutex.java,v 1.2 2005/04/14 18:51:06 agreen Exp $";

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

package org.zoolib.thread;
import org.zoolib.ZDebug;

// ====================================================================================================

public final class ZMutex
	{
	public final void acquire()
		{
		Thread current = Thread.currentThread();
		if (fOwner != current)
			{
			synchronized(this)
				{
				while (fOwner != null)
					{
					try
						{
						this.wait();
						}
					catch (InterruptedException a)
						{}
					}
				fOwner = current;
				fCount = 1;
				}
			}
		else
			{
			++fCount;
			}
		}

	public final void release()
		{
		if (--fCount == 0)
			{
			synchronized(this)
				{
				fOwner = null;
				this.notify();
				}
			}
		}

	public final void fullAcquire(int iCount)
		{
		Thread current = Thread.currentThread();
		if (fOwner != current)
			{
			synchronized(this)
				{
				while (fOwner != null)
					{
					try
						{
						this.wait();
						}
					catch (InterruptedException a)
						{}
					}
				fOwner = current;
				fCount = iCount;
				}
			}
		else
			{
			// can't happen.
			ZDebug.sAssert(false);
			}
		}

	public final int fullRelease()
		{
		synchronized(this)
			{
			int result = fCount;
			fOwner = null;
			fCount = 0;
			this.notify();
			return result;
			}
		}

	public final boolean isLocked()
		{
		return fOwner == Thread.currentThread();
		}

	private Thread fOwner = null;
	private int fCount = 0;
	}
