// "@(#) $Id: ZTBQuery.java,v 1.19 2006/10/22 13:24:35 agreen Exp $";

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

import org.zoolib.ZDebug;
import org.zoolib.ZID;
import org.zoolib.ZTuple;
import org.zoolib.ZUtil_IO;

import java.util.List;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

// ====================================================================================================

public final class ZTBQuery
	{
	public ZTBQuery()
		{ fNode = null; }

	public ZTBQuery(ZTBQuery iOther)
		{ fNode = iOther.fNode; }

	public ZTBQuery(ZTBQueryNode iNode)
		{ fNode = iNode; }

	public ZTBQuery(ZTuple iTuple)
		{ fNode = sNodeFromTuple(iTuple); }

	// All tuples whose ID is \a iID (ie just the one)
	public ZTBQuery(ZID iID)
		{ fNode = new ZTBQueryNode_ID_Constant(iID); }

	public ZTBQuery(long iID)
		{ fNode = new ZTBQueryNode_ID_Constant(iID); }

	// All tuples whose IDs are in \a iIDs
	public ZTBQuery(ZID iIDs[])
		{ fNode = new ZTBQueryNode_ID_Constant(iIDs); }

	public ZTBQuery(long iIDs[])
		{ fNode = new ZTBQueryNode_ID_Constant(iIDs); }

	public ZTBQuery(List iIDs)
		{ fNode = new ZTBQueryNode_ID_Constant(iIDs); }

	// All tuples which match iSpec.
	public ZTBQuery(ZTBSpec iSpec)
		{
		fNode = new ZTBQueryNode_Combo(new Intersection(iSpec, new ZTBQueryNode_All()));
		}

	// All tuples whose ID matches values from \a iSourceQuery's property of type ID and name \a iSourcePropName.
	public ZTBQuery(ZTBQuery iSourceQuery, String iSourcePropName)
		{
		if (iSourceQuery != null && iSourceQuery.fNode != null)
			fNode = new ZTBQueryNode_ID_FromSource(iSourceQuery.fNode, iSourcePropName);
		else
			fNode = null;
		}

	// All tuples whose property \a iPropName is of type ID and matches IDs of tuples from \a iSourceQuery.
	public ZTBQuery(String iPropName, ZTBQuery iSourceQuery)
		{
		if (iSourceQuery != null && iSourceQuery.fNode != null)
			fNode = new ZTBQueryNode_Property(iPropName, iSourceQuery.fNode);
		else
			fNode = null;
		}

	public final ZTuple asTuple()
		{
		if (fNode != null)
			return fNode.asTuple();
		return new ZTuple();
		}

	public final void toDataOutput(DataOutput iDO) throws IOException
		{
		if (fNode == null)
			{
			iDO.writeByte(0);
			}
		else
			{
			fNode.toDataOutput(iDO);
			}
		}

	public final ZTBQuery and(ZTBSpec iSpec)
		{
		if (fNode == null)
			{
			return this;
			}
		else if (iSpec == null || iSpec.isNone())
			{
			return new ZTBQuery();
			}

		Intersection theSect[];
		if (fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo myQNC = (ZTBQueryNode_Combo)fNode;
			theSect = new Intersection[myQNC.fIntersections.length];
			for (int x = 0; x < myQNC.fIntersections.length; ++x)
				theSect[x] = new Intersection(iSpec.and(myQNC.fIntersections[x].fFilter), myQNC.fIntersections[x].fNodes);
			}
		else
			{
			theSect = new Intersection[1];
			theSect[0] = new Intersection(iSpec, fNode);
			}

		return new ZTBQuery(new ZTBQueryNode_Combo(theSect));
		}

	ZTBQueryNode[] sCrossProduct(ZTBQueryNode a[], ZTBQueryNode b[])
		{
		return a;
		}

	public final ZTBQuery and(ZTBQuery iQuery)
		{
		if (fNode == null)
			{
			return this;
			}
		else if (iQuery == null || iQuery.fNode == null)
			{
			return new ZTBQuery();
			}

		if (fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo myQNC = (ZTBQueryNode_Combo)fNode;
			Intersection newSect[];
			Intersection mySect[] = myQNC.fIntersections;
			if (iQuery.fNode instanceof ZTBQueryNode_Combo)
				{
				ZTBQueryNode_Combo otherQNC = (ZTBQueryNode_Combo)iQuery.fNode;
				Intersection otherSect[] = otherQNC.fIntersections;
				newSect = new Intersection[mySect.length * otherSect.length];
				for (int iterMy = 0; iterMy < mySect.length; ++iterMy)
					{
					for (int iterOther = 0; iterOther < otherSect.length; ++iterOther)
						{
						ZTBSpec filter;
						if (mySect[iterMy].fFilter != null)
							filter = mySect[iterMy].fFilter.and(otherSect[iterOther].fFilter);
						else
							filter = otherSect[iterOther].fFilter;
						ZTBQueryNode myNodes[] = mySect[iterMy].fNodes;
						ZTBQueryNode otherNodes[] = otherSect[iterOther].fNodes;
						ZTBQueryNode newNodes[] = new ZTBQueryNode[myNodes.length + otherNodes.length];
						System.arraycopy(myNodes, 0, newNodes, 0, myNodes.length);
						System.arraycopy(otherNodes, 0, newNodes, myNodes.length, otherNodes.length);
						newSect[iterMy * otherSect.length + iterOther] = new Intersection(filter, newNodes);
						}
					}
				}
			else
				{
				newSect = new Intersection[mySect.length];
				for (int x = 0; x < mySect.length; ++x)
					newSect[x] = new Intersection(mySect[x], iQuery.fNode);
				}

			if (newSect == null || newSect.length == 0)
				return new ZTBQuery();
			else
				return new ZTBQuery(new ZTBQueryNode_Combo(myQNC.fSortSpecs, newSect));
			}
		else if (iQuery.fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo otherQNC = (ZTBQueryNode_Combo)iQuery.fNode;
			Intersection otherSect[] = otherQNC.fIntersections;
			Intersection newSect[] = new Intersection[otherSect.length];
			for (int x = 0; x < otherSect.length; ++x)
				newSect[x] = new Intersection(otherSect[x], fNode);
			return new ZTBQuery(new ZTBQueryNode_Combo(newSect));
			}
		else
			{
			Intersection theIntersection = new Intersection(fNode, iQuery.fNode);
			return new ZTBQuery(new ZTBQueryNode_Combo(theIntersection));
			}
		}

	public final ZTBQuery or(ZTBSpec iSpec)
		{
		if (iSpec == null || iSpec.isNone())
			return this;
		return this.or(new ZTBQuery(iSpec));
		}

	public final ZTBQuery or(ZTBQuery iQuery)
		{
		if (fNode == null)
			{
			return iQuery;
			}
		else if (iQuery == null || iQuery.fNode == null)
			{
			return this;
			}
		
		Intersection newSect[];
		if (fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo myQNC = (ZTBQueryNode_Combo)fNode;
			Intersection mySect[] = myQNC.fIntersections;

			if (iQuery.fNode instanceof ZTBQueryNode_Combo)
				{
				ZTBQueryNode_Combo otherQNC = (ZTBQueryNode_Combo)iQuery.fNode;
				Intersection otherSect[] = otherQNC.fIntersections;
				newSect = new Intersection[mySect.length + otherSect.length];
				System.arraycopy(mySect, 0, newSect, 0, mySect.length);
				System.arraycopy(otherSect, 0, newSect, mySect.length, otherSect.length);
				}
			else
				{
				newSect = new Intersection[mySect.length + 1];
				System.arraycopy(mySect, 0, newSect, 0, mySect.length);
				newSect[mySect.length] = new Intersection(iQuery.fNode);
				}
			}
		else if (iQuery.fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo otherQNC = (ZTBQueryNode_Combo)iQuery.fNode;
			Intersection otherSect[] = otherQNC.fIntersections;
			newSect = new Intersection[otherSect.length + 1];
			System.arraycopy(otherSect, 0, newSect, 0, otherSect.length);
			newSect[otherSect.length] = new Intersection(fNode);
			}
		else
			{
			newSect = new Intersection[2];
			newSect[0] = new Intersection(fNode);
			newSect[1] = new Intersection(iQuery.fNode);
			}
		
		return new ZTBQuery(new ZTBQueryNode_Combo(newSect));
		}

	public final ZTBQuery sorted(String iPropName)
		{
		return this.sorted(iPropName, true, 0);
		}

	public final ZTBQuery sorted(String iPropName, boolean iAscending)
		{
		return this.sorted(iPropName, iAscending, 0);
		}

	public final ZTBQuery sorted(String iPropName, boolean iAscending, int iStrength)
		{
		if (fNode == null)
			return this;
		
		SortSpec theSortSpecs[];
		Intersection theSects[];

		if (fNode instanceof ZTBQueryNode_Combo)
			{
			ZTBQueryNode_Combo myQNC = (ZTBQueryNode_Combo)fNode;
			theSects = myQNC.fIntersections;
			if (myQNC.fSortSpecs != null && myQNC.fSortSpecs.length != 0)
				{
				theSortSpecs = new SortSpec[myQNC.fSortSpecs.length + 1];
				theSortSpecs[0] = new SortSpec(iPropName, iAscending, iStrength);
				System.arraycopy(myQNC.fSortSpecs, 0, theSortSpecs, 1, myQNC.fSortSpecs.length);
				}
			else
				{
				theSortSpecs = new SortSpec[1];
				theSortSpecs[0] = new SortSpec(iPropName, iAscending, iStrength);
				}
			}
		else
			{
			theSortSpecs = new SortSpec[1];
			theSortSpecs[0] = new SortSpec(iPropName, iAscending, iStrength);
			theSects = new Intersection[1];
			theSects[0] = new Intersection(fNode);
			}

		return new ZTBQuery(new ZTBQueryNode_Combo(theSortSpecs, theSects));
		}

	public final ZTBQuery first(String iPropName)
		{
		if (fNode == null)
			return this;

		if (fNode instanceof ZTBQueryNode_First)
			{
			ZTBQueryNode_First nodeAs_First = (ZTBQueryNode_First)fNode;
			if (iPropName == null || iPropName.length() == 0)
				return new ZTBQuery(nodeAs_First.getSourceNode());

			return new ZTBQuery(new ZTBQueryNode_First(iPropName, nodeAs_First.getSourceNode()));
			}

		if (iPropName == null || iPropName.length() == 0)
			return this;

		return new ZTBQuery(new ZTBQueryNode_First(iPropName, fNode));
		}

	ZTBQueryNode getNode() { return fNode; }


	public final int compare(ZTBQuery iOther)
		{
		if (fNode != null)
			return fNode.compare(iOther.fNode);
		else if (iOther.fNode != null)
			return 1;
		return 0;
		}

	static ZTBQueryNode sNodeFromTuple(ZTuple iTuple)
		{
		String nodeKind = iTuple.getString("Kind");
		if (nodeKind.equals("All"))
			{
			return new ZTBQueryNode_All();
			}
		else if (nodeKind.equals("ID_Constant"))
			{
			List theListIDs = iTuple.getListNoClone("IDs");
			if (theListIDs.size() != 0)
				{
				long theIDs[] = new long[theListIDs.size()];
				for (int x = 0; x < theIDs.length; ++x)
					{
					ZID anID = (ZID)theListIDs.get(x);
					if (anID != null)
						theIDs[x] = anID.longValue();
					}
				return new ZTBQueryNode_ID_Constant(theIDs);
				}
			}
		else if (nodeKind.equals("ID_FromSource"))
			{
			ZTBQueryNode sourceNode = sNodeFromTuple(iTuple.getTuple("SourceNode"));
			String sourcePropName = iTuple.getString("SourcePropName");
			return new ZTBQueryNode_ID_FromSource(sourceNode, sourcePropName);
			}
		else if (nodeKind.equals("Property"))
			{
			String propName = iTuple.getString("PropName");
			ZTBQueryNode sourceNode = sNodeFromTuple(iTuple.getTuple("SourceNode"));
			return new ZTBQueryNode_Property(propName, sourceNode);
			}
		else if (nodeKind.equals("Combo"))
			{
			SortSpec theSortSpecs[] = null;
			List listSort = iTuple.getListNoClone("Sort");
			if (listSort.size() > 0)
				{
				theSortSpecs = new SortSpec[listSort.size()];
				for (int x = 0; x < listSort.size(); ++x)
					{
					ZTuple theTuple = (ZTuple)listSort.get(x);
					if (theTuple != null)
						theSortSpecs[x] = new SortSpec(theTuple.getString("PropName"), theTuple.getBool("Ascending"), theTuple.getInt32("Strength"));
					}
				}

			Intersection theIntersections[] = null;
			List listSect = iTuple.getListNoClone("Intersections");
			if (listSect.size() > 0)
				{
				theIntersections = new Intersection[listSect.size()];
				for (int x = 0; x < listSect.size(); ++x)
					{
					ZTuple theTuple = (ZTuple)listSect.get(x);
					if (theTuple != null)
						theIntersections[x] = new Intersection(theTuple);
					}
				}
			return new ZTBQueryNode_Combo(theSortSpecs, theIntersections);
			}
		else if (nodeKind.equals("First"))
			{
			String propName = iTuple.getString("PropName");
			ZTBQueryNode sourceNode = sNodeFromTuple(iTuple.getTuple("SourceNode"));
			return new ZTBQueryNode_First(propName, sourceNode);
			}
		return null;
		}

	private final ZTBQueryNode fNode;
	}

// ====================================================================================================

abstract class ZTBQueryNode
	{
	public abstract ZTuple asTuple();
	public abstract int kind();

	public abstract void toDataOutput(DataOutput iDO) throws IOException;

	public int compare(ZTBQueryNode iOther)
		{
		if (iOther == null)
			return 1;

		int result = this.kind() - iOther.kind();
		if (result != 0)
			return result;

		// Both nodes are of the same type.
		if (this instanceof ZTBQueryNode_All)
			{
			// They're both _All, and so they are equal.
			return 0;
			}

		if (this instanceof ZTBQueryNode_Combo)
			return ((ZTBQueryNode_Combo)this).compare_Combo((ZTBQueryNode_Combo)iOther);

		if (this instanceof ZTBQueryNode_First)
			return ((ZTBQueryNode_First)this).compare_First((ZTBQueryNode_First)iOther);

		if (this instanceof ZTBQueryNode_ID_Constant)
			return ((ZTBQueryNode_ID_Constant)this).compare_ID_Constant((ZTBQueryNode_ID_Constant)iOther);

		if (this instanceof ZTBQueryNode_ID_FromSource)
			return ((ZTBQueryNode_ID_FromSource)this).compare_ID_FromSource((ZTBQueryNode_ID_FromSource)iOther);

		if (this instanceof ZTBQueryNode_Property)
			return ((ZTBQueryNode_Property)this).compare_Property((ZTBQueryNode_Property)iOther);

		ZDebug.sAssert(false);
		return 0;
		}
	}

// ====================================================================================================

class ZTBQueryNode_All extends ZTBQueryNode
	{
	ZTBQueryNode_All()
		{}

	public int kind()
		{ return 1; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "All");
		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		iDO.writeByte(1);
		}
	}

// ====================================================================================================

class ZTBQueryNode_Combo extends ZTBQueryNode
	{
	private static Intersection[] sSimplify(Intersection iSect[])
		{
		return iSect;
		}

	ZTBQueryNode_Combo(Intersection iSect[])
		{
		fSortSpecs = null;
		fIntersections = sSimplify(iSect);
		}

	ZTBQueryNode_Combo(SortSpec iSortSpecs[], Intersection iSect[])
		{
		fSortSpecs = iSortSpecs;
		fIntersections = sSimplify(iSect);
		}

	ZTBQueryNode_Combo(Intersection iIntersection)
		{
		fSortSpecs = null;
		fIntersections = new Intersection[1];
		fIntersections[0] = iIntersection;
		}

	public int kind()
		{ return 2; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "Combo");

		if (fSortSpecs != null && fSortSpecs.length != 0)
			{
			List theList = theTuple.setMutableList("Sort");
			for (int x = 0; x < fSortSpecs.length; ++x)
				theList.add(fSortSpecs[x].asTuple());
			}

		if (fIntersections != null && fIntersections.length != 0)
			{
			List theList = theTuple.setMutableList("Intersections");
			for (int x = 0; x < fIntersections.length; ++x)
				theList.add(fIntersections[x].asTuple());
			}

		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		iDO.writeByte(2);

		if (fSortSpecs == null)
			{
			ZUtil_IO.sWriteCount(iDO, 0);
			}
		else
			{
			ZUtil_IO.sWriteCount(iDO, fSortSpecs.length);
			for (int x = 0; x < fSortSpecs.length; ++x)
				fSortSpecs[x].toDataOutput(iDO);
			}

		if (fIntersections == null)
			{
			ZUtil_IO.sWriteCount(iDO, 0);
			}
		else
			{
			ZUtil_IO.sWriteCount(iDO, fIntersections.length);
			for (int x = 0; x < fIntersections.length; ++x)
				fIntersections[x].toDataOutput(iDO);
			}
		}

	private static final int sCompare(SortSpec[] iLeft, SortSpec[] iRight)
		{
		if (iLeft == null || iLeft.length == 0)
			{
			if (iRight == null || iRight.length == 0)
				return 0;
			return -1;
			}
		else if (iRight == null || iRight.length == 0)
			{
			return 1;
			}

		int countLeft = iLeft.length;
		int countRight = iRight.length;
		int shortest = Math.min(countLeft, countRight);
		for (int x = 0; x < shortest; ++x)
			{
			int result = iLeft[x].compare(iRight[x]);
			if (result != 0)
				return result;
			}
		
		if (countLeft < countRight)
			return -1;
		else if (countLeft > countRight)
			return 1;
		return 0;				
		}

	private static final int sCompare(Intersection[] iLeft, Intersection[] iRight)
		{
		if (iLeft == null || iLeft.length == 0)
			{
			if (iRight == null || iRight.length == 0)
				return 0;
			return -1;
			}
		else if (iRight == null || iRight.length == 0)
			{
			return 1;
			}

		int countLeft = iLeft.length;
		int countRight = iRight.length;
		int shortest = Math.min(countLeft, countRight);
		for (int x = 0; x < shortest; ++x)
			{
			int result = iLeft[x].compare(iRight[x]);
			if (result != 0)
				return result;
			}
		
		if (countLeft < countRight)
			return -1;
		else if (countLeft > countRight)
			return 1;
		return 0;				
		}

	public final int compare_Combo(ZTBQueryNode_Combo iOther)
		{
		int result = sCompare(fSortSpecs, iOther.fSortSpecs);
		if (result != 0)
			return result;
		return sCompare(fIntersections, iOther.fIntersections);
		}

	public final SortSpec fSortSpecs[];
	public final Intersection fIntersections[];
	};

// ====================================================================================================

class ZTBQueryNode_First extends ZTBQueryNode
	{
	public ZTBQueryNode_First(String iPropName, ZTBQueryNode iSourceNode)
		{
		fPropName = iPropName;
		fSourceNode = iSourceNode;
		}

	public int kind()
		{ return 3; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "First");
		theTuple.setString("PropName", fPropName);
		if (fSourceNode != null)
			theTuple.setTuple("SourceNode", fSourceNode.asTuple());
		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		// Our type differs -- in C++ we've quit using the kind method, the
		// only place we use type codes is in ToStream/FromStream, and over
		// there type code 4 is a _First node.
		iDO.writeByte(4);
		ZUtil_IO.sWriteString(iDO, fPropName);
		fSourceNode.toDataOutput(iDO);
		}

	public final int compare_First(ZTBQueryNode_First iOther)
		{
		int result = fPropName.compareTo(iOther.fPropName);
		if (result != 0)
			return result;
		return fSourceNode.compare(iOther.fSourceNode);
		}

	public final String getPropName()
		{ return fPropName; }

	public final ZTBQueryNode getSourceNode()
		{ return fSourceNode; }

	private final String fPropName;
	private final ZTBQueryNode fSourceNode;
	};

// ====================================================================================================

class ZTBQueryNode_ID_Constant extends ZTBQueryNode
	{
	ZTBQueryNode_ID_Constant(ZID iID)
		{
		fIDs = null;
		if (iID != null && iID.longValue() != 0)
			{
			fIDs = new long[1];
			fIDs[0] = iID.longValue();
			}
		}

	ZTBQueryNode_ID_Constant(long iID)
		{
		fIDs = null;
		if (iID != 0)
			{
			fIDs = new long[1];
			fIDs[0] = iID;
			}
		}

	ZTBQueryNode_ID_Constant(ZID iIDs[])
		{
		fIDs = null;
		if (iIDs != null)
			{
			int count = 0;
			for (int i = 0; i < iIDs.length; ++i)
				{
				if (iIDs[i].longValue() != 0)
					++count;
				}
			if (count != 0)
				{
				fIDs = new long[count];
				int index = 0;
				for (int i = 0; i < iIDs.length; ++i)
					{
					long theID = iIDs[i].longValue();
					if (theID != 0)
						fIDs[index++] = theID;
					}
				}
			}
		}

	ZTBQueryNode_ID_Constant(long iIDs[])
		{
		fIDs = null;
		if (iIDs != null)
			{
			int count = 0;
			for (int i = 0; i < iIDs.length; ++i)
				{
				if (iIDs[i] != 0)
					++count;
				}
			if (count != 0)
				{
				fIDs = new long[count];
				int index = 0;
				for (int i = 0; i < iIDs.length; ++i)
					{
					long theID = iIDs[i];
					if (theID != 0)
						fIDs[index++] = theID;
					}
				}
			}
		}
	
	ZTBQueryNode_ID_Constant(List iIDs)
		{
		fIDs = null;
		if (iIDs != null)
			{
			int count = 0;
			for (java.util.Iterator i = iIDs.iterator(); i.hasNext(); /*no increment*/)
				{
				Object cur = i.next();
				if (cur instanceof ZID)
					{
					if (((ZID)cur).longValue() != 0)
						++count;
					}
				}
			if (count != 0)
				{
				fIDs = new long[count];
				int index = 0;
				for (java.util.Iterator i = iIDs.iterator(); i.hasNext(); /*no increment*/)
					{
					Object cur = i.next();
					if (cur instanceof ZID)
						{
						long theID = ((ZID)cur).longValue();
						if (theID != 0)
							fIDs[index++] = theID;
						}
					}
				}
			}
		}
	
	public int kind()
		{ return 4; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "ID_Constant");
		if (fIDs != null && fIDs.length != 0)
			{
			List theList = theTuple.setMutableList("IDs");
			for (int x = 0; x < fIDs.length; ++x)
				theList.add(new ZID(fIDs[x]));
			}
		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		iDO.writeByte(5);
		if (fIDs == null)
			{
			ZUtil_IO.sWriteCount(iDO, 0);
			}
		else
			{
			ZUtil_IO.sWriteCount(iDO, fIDs.length);
			for (int x = 0; x < fIDs.length; ++x)
				iDO.writeLong(fIDs[x]);
			}
		}

	public final int compare_ID_Constant(ZTBQueryNode_ID_Constant iOther)
		{
		if (fIDs == null || fIDs.length == 0)
			{
			if (iOther.fIDs == null || iOther.fIDs.length == 0)
				return 0;
			return -1;
			}
		else if (iOther.fIDs == null || iOther.fIDs.length == 0)
			{
			return 1;
			}
		
		int countThis = fIDs.length;
		int countOther = iOther.fIDs.length;
		int shortest = Math.min(countThis, countOther);
		for (int x = 0; x < shortest; ++x)
			{
			if (fIDs[x] < iOther.fIDs[x])
				return -1;
			else if (fIDs[x] > iOther.fIDs[x])
				return 1;
			}
		
		if (countThis < countOther)
			return -1;
		else if (countThis > countOther)
			return 1;
		return 0;				
		}

	public final long[] getIDs()
		{ return fIDs; }

	private long fIDs[];
	};

// ====================================================================================================

class ZTBQueryNode_ID_FromSource extends ZTBQueryNode
	{
	public ZTBQueryNode_ID_FromSource(ZTBQueryNode iSourceNode, String iSourcePropName)
		{
		fSourceNode = iSourceNode;
		fSourcePropName = iSourcePropName;
		}

	public int kind()
		{ return 5; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "ID_FromSource");
		theTuple.setString("SourcePropName", fSourcePropName);
		if (fSourceNode != null)
			theTuple.setTuple("SourceNode", fSourceNode.asTuple());
		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		iDO.writeByte(6);
		ZUtil_IO.sWriteString(iDO, fSourcePropName);
		fSourceNode.toDataOutput(iDO);
		}

	public final int compare_ID_FromSource(ZTBQueryNode_ID_FromSource iOther)
		{
		int result = fSourcePropName.compareTo(iOther.fSourcePropName);
		if (result != 0)
			return result;
		return fSourceNode.compare(iOther.fSourceNode);
		}

	public final ZTBQueryNode getSourceNode()
		{ return fSourceNode; }

	public final String getSourcePropName()
		{ return fSourcePropName; }

	private final ZTBQueryNode fSourceNode;
	private final String fSourcePropName;
	};

// ====================================================================================================

class ZTBQueryNode_Property extends ZTBQueryNode
	{
	public ZTBQueryNode_Property(String iPropName, ZTBQueryNode iSourceNode)
		{
		fPropName = iPropName;
		fSourceNode = iSourceNode;
		}

	public int kind()
		{ return 6; }

	public ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("Kind", "Property");
		theTuple.setString("PropName", fPropName);
		if (fSourceNode != null)
			theTuple.setTuple("SourceNode", fSourceNode.asTuple());
		return theTuple;
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		iDO.writeByte(7);
		ZUtil_IO.sWriteString(iDO, fPropName);
		fSourceNode.toDataOutput(iDO);
		}

	public final int compare_Property(ZTBQueryNode_Property iOther)
		{
		int result = fPropName.compareTo(iOther.fPropName);
		if (result != 0)
			return result;
		return fSourceNode.compare(iOther.fSourceNode);
		}

	public final String getPropName()
		{ return fPropName; }

	public final ZTBQueryNode getSourceNode()
		{ return fSourceNode; }

	private final String fPropName;
	private final ZTBQueryNode fSourceNode;
	};

// ====================================================================================================

class Intersection
	{
	public Intersection(ZTBQueryNode iNode)
		{
		fFilter = ZTBSpec.sAny();
		fNodes = new ZTBQueryNode[1];
		fNodes[0] = iNode;
		}

	public Intersection(ZTBSpec iFilter, ZTBQueryNode iNode)
		{
		fFilter = iFilter;
		fNodes = new ZTBQueryNode[1];
		fNodes[0] = iNode;
		}

	public Intersection(ZTBQueryNode iNodeA, ZTBQueryNode iNodeB)
		{
		fFilter = ZTBSpec.sAny();
		fNodes = new ZTBQueryNode[2];
		fNodes[0] = iNodeA;
		fNodes[1] = iNodeB;
		}

	public Intersection(ZTBSpec iFilter, ZTBQueryNode iNodes[])
		{
		fFilter = iFilter;
		fNodes = iNodes;
		}

	public Intersection(Intersection iSect, ZTBQueryNode iNode)
		{
		fFilter = iSect.fFilter;
		fNodes = new ZTBQueryNode[iSect.fNodes.length + 1];
		System.arraycopy(iSect.fNodes, 0, fNodes, 0, iSect.fNodes.length);
		fNodes[iSect.fNodes.length] = iNode;
		}
	
	public Intersection(ZTuple iTuple)
		{
		fFilter = new ZTBSpec(iTuple.getTuple("Filter"));
		List listNodes = iTuple.getListNoClone("Nodes");
		fNodes = new ZTBQueryNode[listNodes.size()];
		for (int x = 0; x < listNodes.size(); ++x)
			fNodes[x] = ZTBQuery.sNodeFromTuple((ZTuple)listNodes.get(x));
		}
	
	public final ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		if (fFilter != null)
			theTuple.setTuple("Filter", fFilter.asTuple());
		if (fNodes != null && fNodes.length != 0)
			{
			List theList = theTuple.setMutableList("Nodes");
			for (int x = 0; x < fNodes.length; ++x)
				theList.add(fNodes[x].asTuple());
			}
		return theTuple;
		}

	public final void toDataOutput(DataOutput iDO) throws IOException
		{
		fFilter.toDataOutput(iDO);
		ZUtil_IO.sWriteCount(iDO, fNodes.length);
		for (int x = 0; x < fNodes.length; ++x)
			fNodes[x].toDataOutput(iDO);
		}

	public final int compare(Intersection iOther)
		{
		int result = fFilter.compare(iOther.fFilter);
		if (result != 0)
			return result;

		int countThis = fNodes.length;
		int countOther = iOther.fNodes.length;
		int shortest = Math.min(countThis, countOther);
		for (int x = 0; x < shortest; ++x)
			{
			result = fNodes[x].compare(iOther.fNodes[x]);
			if (result != 0)
				return result;
			}
		
		if (countThis < countOther)
			return -1;
		else if (countThis > countOther)
			return 1;
		return 0;				
		}

	public final ZTBSpec fFilter;
	public final ZTBQueryNode fNodes[];
	}

// ====================================================================================================

class SortSpec
	{
	public SortSpec(DataInput iDI) throws IOException
		{
		fPropName = ZUtil_IO.sReadString(iDI);
		fAscending = iDI.readByte() != 0;
		fStrength = iDI.readByte();
		}

	public SortSpec(String iPropName, boolean iAscending, int iStrength)
		{
		fPropName = iPropName;
		fAscending = iAscending;
		fStrength = iStrength;
		}

	public final ZTuple asTuple()
		{
		ZTuple theTuple = new ZTuple();
		theTuple.setString("PropName", fPropName);
		theTuple.setBool("Ascending", fAscending);
		if (fStrength != 0)
			theTuple.setInt32("Strength", fStrength);
		return theTuple;
		}

	public final int compare(SortSpec iOther)
		{
		if (fStrength < iOther.fStrength)
			return -1;
		else if (fStrength > iOther.fStrength)
			return 1;
		else if (!fAscending && iOther.fAscending)
			return -1;
		else if (fAscending && !iOther.fAscending)
			return 1;
		else
			return fPropName.compareTo(iOther.fPropName);
		}

	public final void toDataOutput(DataOutput iDO) throws IOException
		{
		ZUtil_IO.sWriteString(iDO, fPropName);
		iDO.writeByte(fAscending ? 1 : 0);
		iDO.writeByte(fStrength);
		}

	public final String fPropName;
	public final boolean fAscending;
	public final int fStrength;
	};
