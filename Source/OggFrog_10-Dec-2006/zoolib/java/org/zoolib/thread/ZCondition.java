// "@(#) $Id: ZCondition.java,v 1.2 2005/07/29 15:16:46 agreen Exp $";

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

// ====================================================================================================

public final class ZCondition
	{
	public final void wait(ZMutex iMutex)
		{
		int oldCount;
		synchronized(this)
			{
			oldCount = iMutex.fullRelease();
			try
				{
				super.wait();
				}
			catch (InterruptedException a)
				{}
			}
		iMutex.fullAcquire(oldCount);
		}

	public final void wait(ZMutex iMutex, long iTimeout)
		{
		int oldCount;
		synchronized(this)
			{
			oldCount = iMutex.fullRelease();
			try
				{
				super.wait(iTimeout);
				}
			catch (InterruptedException a)
				{}
			}
		iMutex.fullAcquire(oldCount);
		}

	public final void broadcast()
		{
		synchronized(this)
			{
			this.notifyAll();
			}
		}

	public final void signal()
		{
		synchronized(this)
			{
			this.notify();
			}
		}
	}
