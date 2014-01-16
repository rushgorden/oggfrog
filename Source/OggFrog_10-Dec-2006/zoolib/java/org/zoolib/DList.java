// "@(#) $Id: DList.java,v 1.1 2006/10/23 22:06:52 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

// ====================================================================================================

public class DList
	{
	private Link fHead;

	public DList()
		{
		fHead = null;
		}

	public final boolean empty()
		{
		return fHead == null;
		}

	public final boolean contains(Link iLink)
		{
		return iLink.fNext != null;
		}

	public final void insert(Link iLink)
		{
		ZDebug.sAssert(iLink.fPrev == null);
		ZDebug.sAssert(iLink.fNext == null);
		
		if (fHead == null)
			{
			fHead = iLink;
			iLink.fPrev = iLink;
			iLink.fNext = iLink;
			}
		else
			{
			iLink.fNext = fHead;
			iLink.fPrev = fHead.fPrev;
			fHead.fPrev.fNext = iLink;
			fHead.fPrev = iLink;
			}
		}

	public final void insertIfNotContains(Link iLink)
		{
		if (iLink.fNext == null)
			this.insert(iLink);
		}

	public final void remove(Link iLink)
		{
		ZDebug.sAssert(fHead != null);
		ZDebug.sAssert(iLink.fPrev != null);
		ZDebug.sAssert(iLink.fNext != null);

		if (iLink.fPrev == iLink)
			{
			ZDebug.sAssert(fHead == iLink);
			fHead = null;
			}
		else
			{
			if (fHead == iLink)
				fHead = iLink.fNext;
			iLink.fNext.fPrev = iLink.fPrev;
			iLink.fPrev.fNext = iLink.fNext;
			}
		iLink.fNext = null;
		iLink.fPrev = null;		
		}

	public final void removeIfContains(Link iLink)
		{
		if (iLink.fNext != null)
			this.remove(iLink);
		}

	public final java.util.Iterator iteratorEraseAll()
		{
		return new IteratorEA(this);
		}		

	public final java.util.Iterator iterator()
		{
		return new Iterator(this);
		}		

	//=====
	public static final class Link
		{
		private Link() {}
		public Link(Object iObject)
			{ fObject = iObject; }

		public final Object getObject()
			{ return fObject; }

		Object fObject;
		Link fPrev;
		Link fNext;
		};
	//=====
	private static final class Iterator implements java.util.Iterator
		{
		private final DList fList;
		private DList.Link fNextToReturn;

		Iterator(DList iList)
			{
			fList = iList;
			fNextToReturn = fList.fHead;
			}

		public boolean hasNext()
			{
			return fNextToReturn != null;
			}

		public Object next()
			{
			Object result = fNextToReturn.getObject();
			fNextToReturn = fNextToReturn.fNext;
			if (fNextToReturn == fList.fHead)
				fNextToReturn = null;
			return result;
			}

		public void remove()
			{
			throw new UnsupportedOperationException();
			}
		}
	//=====
	private static final class IteratorEA implements java.util.Iterator
		{
		private final DList fList;
		private DList.Link fNextToReturn;

		IteratorEA(DList iList)
			{
			fList = iList;
			fNextToReturn = fList.fHead;
			}

		public boolean hasNext()
			{
			return fNextToReturn != null;
			}

		public Object next()
			{
			Object result = fNextToReturn.getObject();
			Link newNextToReturn = fNextToReturn.fNext;
			fNextToReturn.fPrev = null;
			fNextToReturn.fNext = null;
			if (newNextToReturn == fList.fHead)
				{
				fList.fHead = null;
				newNextToReturn = null;
				}
			fNextToReturn = newNextToReturn;
			return result;
			}

		public void remove()
			{
			throw new UnsupportedOperationException();
			}
		}
	//=====
	};

