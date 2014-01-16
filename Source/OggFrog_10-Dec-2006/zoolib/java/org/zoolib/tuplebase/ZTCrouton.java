// "@(#) $Id: ZTCrouton.java,v 1.2 2006/10/23 21:24:50 agreen Exp $";

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

import org.zoolib.ZID;
import org.zoolib.ZTuple;

import org.zoolib.DList;

import java.util.Map;

// ====================================================================================================

public class ZTCrouton
	{
	public static final int eChange_Local = 1;
	public static final int eChange_Remote = 2;

	public final void register(ZTSoup iTSoup, long iID)
		{ iTSoup.pRegisterTCrouton(this, iID); }

	public final void register(ZTSoup iTSoup)
		{ iTSoup.pRegisterTCrouton(this); }

	public final void close()
		{ fPCrouton.fTSoup.pCloseTCrouton(this); }

	public void changed(int iChange)
		{}

	public final long getID()
		{ return fPCrouton.fID; }

	public final ZID getZID()
		{ return fPCrouton.fZID; }

	public final boolean hasCurrent()
		{ return fPCrouton.fHasValue_Current; }

	public final boolean hasPrior()
		{ return fPCrouton.fHasValue_Prior; }
	
	public final ZTuple getCurrent()
		{ return new ZTuple(fPCrouton.fValue_Current); }

	public final ZTuple getPrior()
		{ return new ZTuple(fPCrouton.fValue_Prior); }

	public final void set(Map iMap)
		{ fPCrouton.fTSoup.pSetFromTCrouton(fPCrouton, new ZTuple(iMap)); }

	ZTSoup.PCrouton fPCrouton;

	final DList.Link fLink_PCrouton = new DList.Link(this);
	};
