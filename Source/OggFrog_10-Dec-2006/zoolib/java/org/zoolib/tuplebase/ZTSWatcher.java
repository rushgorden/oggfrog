// "@(#) $Id: ZTSWatcher.java,v 1.4 2006/10/30 06:17:01 agreen Exp $";

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

import java.util.Map;
import java.util.Set;

abstract public class ZTSWatcher
	{
	public void close()
		{}

	public abstract long allocateID();

	/**
	\param iRemovedIDs a set of ZID instances, being the IDs which we no
	longer wish to be notified of changes to.

	\param iAddedIDs a set of ZID instances, being the IDs which we would in
	future like to be notified of changes to.

	\param iRemovedQueries a set of Long instances, being the refcons allocated
	locally identifying queries for which we no longer wish to be notified
	of changes to their result lists.
	
	\param iAddedQueries a Map of Longs to ZTBQueries, the Longs
	are locally allocated refcons.

	\param oAddedIDs a set that will be filled with ZID instances, being the IDs
	of tuples the server has preemptively added to the list of those we'll
	be notified of changes to.

	\param oChangedTuples a Map of ZIDs to ZTuples, being the IDs and new
	values of tuples that had changed since doIt was last called, excluding
	any changes made by values passed in iWrittenTuples.
	
	\param iWrittenTuples a Map of ZIDs to ZTuples, being the IDs and new
	values of tuples which should be stored in the tuplestore.

	\param oChangedQueries a Map of Longs to Lists, being the local refcons
	of queries passed in iAddedQueries now or in the past. The Lists
	contain the result list of the query (as ZID instances).
	*/

	public abstract void doIt(Set iRemovedIDs, Set iAddedIDs,
					Set iRemovedQueries, Map iAddedQueries,
					Set oAddedIDs,
					Map oChangedTuples,
					Map iWrittenTuples,
					Map oChangedQueries);
	}
