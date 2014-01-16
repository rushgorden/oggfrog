// "@(#) $Id: ZTBSpec.java,v 1.15 2006/10/22 13:23:40 agreen Exp $";

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

import org.zoolib.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

public final class ZTBSpec
	{
	public static final int eRel_Invalid = 0;
	public static final int eRel_Less = 1;
	public static final int eRel_LessEqual = 2;
	public static final int eRel_Equal = 3;
	public static final int eRel_GreaterEqual = 4;
	public static final int eRel_Greater = 5;
	public static final int eRel_Has = 6;
	public static final int eRel_HasOfType = 7;
	public static final int eRel_Lacks = 8;
	public static final int eRel_LacksOfType = 9;
	public static final int eRel_VectorContains = 10;
	public static final int eRel_StringContains = 11;
	public static final int eRel_Regex = 12;

	public static final class Comp
		{
		public Comp(DataInput iDI) throws IOException
			{
			fRel = iDI.readByte();
			fStrength = iDI.readByte();
			}

		public Comp(int iRel, int iStrength)
			{
			fRel = iRel;
			fStrength = iStrength;
			}

		public Comp(int iRel)
			{
			fRel = iRel;
			fStrength = 0;
			}

		public final int compare(Comp iOther)
			{
			int result = fRel - iOther.fRel;
			if (result != 0)
				return result;

			return fStrength - iOther.fStrength;
			}

		public void toDataOutput(DataOutput iDO) throws IOException
			{
			iDO.writeByte(fRel);
			iDO.writeByte(fStrength);
			}

		public final int fRel;
		public final int fStrength;
		}

	public static final class Criterion
		{
		public Criterion(DataInput iDI) throws IOException
			{
			fPropName = ZUtil_IO.sReadString(iDI);
			fComp = new Comp(iDI);
			fValue = ZTuple.sReadValue(iDI);
			}

		public Criterion(String iPropName)
			{ this(iPropName, eRel_Has, 0, null); }

		public Criterion(String iPropName, int iRel, Object iValue)
			{ this(iPropName, iRel, 0, iValue); }

		public Criterion(String iPropName, int iRel, int iStrength, Object iValue)
			{
			fPropName = iPropName;
			fComp = new Comp(iRel, iStrength);
			fValue = iValue;
			}

		public Criterion(String iPropName, Comp iComp, Object iValue)
			{
			fPropName = iPropName;
			fComp = iComp;
			fValue = iValue;
			}

		public final int compare(Criterion iOther)
			{
			int result;

			result = fComp.compare(iOther.fComp);
			if (result != 0)
				return result;

			result = fPropName.compareTo(iOther.fPropName);
			if (result != 0)
				return result;

			return ZTuple.sCompareTV(fValue, iOther.fValue);
			}

		public void toDataOutput(DataOutput iDO) throws IOException
			{
			ZUtil_IO.sWriteString(iDO, fPropName);
			fComp.toDataOutput(iDO);
			ZTuple.sWriteValue(iDO, fValue);
			}

		public final String fPropName;
		public final Comp fComp;
		public final Object fValue;
		}

	private static int sCompare(Criterion[] iLeft, Criterion[] iRight)
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
	
	public ZTBSpec()
		{
		fCriteria = null;
		}

	public ZTBSpec(boolean iAny)
		{
		if (iAny)
			{
			fCriteria = new Criterion[1][];
			fCriteria[0] = new Criterion[0];
			}
		else
			{
			fCriteria = null;
			}
		}

	public ZTBSpec(Criterion iCrit)
		{
		fCriteria = new Criterion[1][];
		fCriteria[0] = new Criterion[1];
		fCriteria[0][0] = iCrit;
		}

	public ZTBSpec(Criterion iCriteria[][])
		{
		fCriteria = iCriteria;
		}

	public ZTBSpec(String iPropName, int iRel, int iStrength, Object iValue)
		{
		this(new Criterion(iPropName, iRel, iStrength, iValue));
		}

	private static final String sStringFromRel(int iRel)
		{
		switch (iRel)
			{
			case eRel_Less: return "<";
			case eRel_LessEqual: return "<=";
			case eRel_Equal: return "==";
			case eRel_GreaterEqual: return ">=";
			case eRel_Greater: return ">";
			case eRel_Has: return "Has";
			case eRel_HasOfType: return "HasOfType";
			case eRel_Lacks: return "Lacks";
			case eRel_LacksOfType: return "LacksOfType";
			case eRel_VectorContains: return "VectorContains";
			case eRel_StringContains: return "StringContains";
			case eRel_Regex: return "re";
			}
		return "???";
		}

	private static final int sRelFromString(String iString)
		{
		if (iString.equals("<")) return eRel_Less;
		else if (iString.equals("<=")) return eRel_LessEqual;
		else if (iString.equals("==")) return eRel_Equal;
		else if (iString.equals(">=")) return eRel_GreaterEqual;
		else if (iString.equals(">")) return eRel_Greater;
		else if (iString.equals("Has")) return eRel_Has;
		else if (iString.equals("HasOfType")) return eRel_HasOfType;
		else if (iString.equals("Lacks")) return eRel_Lacks;
		else if (iString.equals("LacksOfType")) return eRel_LacksOfType;
		else if (iString.equals("VectorContains")) return eRel_VectorContains;
		else if (iString.equals("StringContains")) return eRel_StringContains;
		else if (iString.equals("re")) return eRel_Regex;
			 
		return eRel_Invalid;
		}

	private static final ZTuple sTupleFromCriteria(Criterion[][] iCriteria)
		{
		ZTuple result = new ZTuple();
		List outerVector = result.setMutableList("Criteria");
		if (iCriteria != null)
			{
			for (int outer = 0; outer < iCriteria.length; ++outer)
				{
				List innerVector = new ArrayList();
				for (int inner = 0; inner < iCriteria[outer].length; ++inner)
					{
					ZTuple aTuple = new ZTuple();
					aTuple.setString("PropName", iCriteria[outer][inner].fPropName);
					aTuple.setString("Rel", sStringFromRel(iCriteria[outer][inner].fComp.fRel));
					if (0 != iCriteria[outer][inner].fComp.fStrength)
						aTuple.setInt32("Strength", iCriteria[outer][inner].fComp.fStrength);
					aTuple.setValue("Value", iCriteria[outer][inner].fValue);
					innerVector.add(aTuple);
					}
				outerVector.add(innerVector);
				}
			}
		return result;
		}

	private static final Criterion[][] sCriteriaFromMap(Map iMap)
		{
		ZTuple theTuple = new ZTuple(iMap);
		List outerVector = theTuple.getListNoClone("Criteria");
		Criterion result[][] = new Criterion[outerVector.size()][];
		int resultOffset = 0;
		for (int outer = 0; outer < outerVector.size(); ++outer)
			{
			Object innerOb = outerVector.get(outer);
			if (innerOb instanceof List)
				{
				List innerVector = (List)innerOb;
				Criterion[] criteria = new Criterion[innerVector.size()];
				int innerOffset = 0;
				for (int inner = 0; inner < innerVector.size(); ++inner)
					{
					Object critOb = innerVector.get(inner);
					if (critOb instanceof Map)
						{
						ZTuple critTuple = new ZTuple((Map)critOb);
						String thePropName = critTuple.getString("PropName");
						int theRel = sRelFromString(critTuple.getString("Rel"));
						int theStrength = critTuple.getInt32("Strength");
						Object theValue = critTuple.getValue("Value");
						criteria[innerOffset] = new Criterion(thePropName, theRel, theStrength, theValue);
						++innerOffset;
						}
					}

				if (innerOffset < innerVector.size())
					{
					// We over-allocated criteria, so create an array of the right size.
					Criterion newCriteria[] = new Criterion[innerOffset];
					System.arraycopy(criteria, 0, newCriteria, 0, innerOffset);
					criteria = newCriteria;
					}
				result[resultOffset] = criteria;
				++resultOffset;
				}
			}

		if (resultOffset < outerVector.size())
			{
			// We over-allocated result, so create an array of the right size.
			Criterion newResult[][] = new Criterion[resultOffset][];
			System.arraycopy(result, 0, newResult, 0, resultOffset);
			result = newResult;
			}
		
		return result;
		}

	private static final Criterion[][] sCriteriaFromDataInput(DataInput iDI) throws IOException
		{
		Criterion[][] result = null;
		int outerCount = ZUtil_IO.sReadCount(iDI);
		if (outerCount != 0)
			{
			result = new Criterion[outerCount][];
			for (int outerIndex = 0; outerIndex < outerCount; ++outerIndex)
				{
				int innerCount = ZUtil_IO.sReadCount(iDI);
				result[outerIndex] = new Criterion[innerCount];
				for (int innerIndex = 0; innerIndex < innerCount; ++innerIndex)
					result[outerIndex][innerIndex] = new Criterion(iDI);
				}
			}
		return result;
		}

	private static final void sCriteriaToDataOutput(DataOutput iDO, Criterion[][] iCriteria) throws IOException
		{
		if (iCriteria == null)
			{
			iDO.writeByte(0);
			}
		else
			{
			ZUtil_IO.sWriteCount(iDO, iCriteria.length);
			for (int outerIndex = 0; outerIndex < iCriteria.length; ++outerIndex)
				{
				ZUtil_IO.sWriteCount(iDO, iCriteria[outerIndex].length);
				for (int innerIndex = 0; innerIndex < iCriteria[outerIndex].length; ++innerIndex)
					iCriteria[outerIndex][innerIndex].toDataOutput(iDO);
				}	
			}
		}

	/// Initialize self from iMap
	public ZTBSpec(Map iMap)
		{
		fCriteria = sCriteriaFromMap(iMap);
		}

	/// Initialize self from iDI
	public ZTBSpec(DataInput iDI) throws IOException
		{
		fCriteria = sCriteriaFromDataInput(iDI);
		}

	public void finalize()
		{
		}

	public final ZTuple asTuple()
		{
		return sTupleFromCriteria(fCriteria);
		}

	public final void toDataOutput(DataOutput iDO) throws IOException
		{
		sCriteriaToDataOutput(iDO, fCriteria);
		}

	public int compare(ZTBSpec iOther)
		{
		if (iOther == this)
			return 0;

		if (fCriteria == null || fCriteria.length == 0)
			{
			if (iOther.fCriteria == null || iOther.fCriteria.length == 0)
				return 0;
			return -1;
			}
		else if (iOther.fCriteria == null || iOther.fCriteria.length == 0)
			{
			return 1;
			}

		int countThis = fCriteria.length;
		int countOther = iOther.fCriteria.length;
		int shortest = Math.min(countThis, countOther);
		for (int x = 0; x < shortest; ++x)
			{
			int result = sCompare(fCriteria[x], iOther.fCriteria[x]);
			if (result != 0)
				return result;
			}

		if (countThis < countOther)
			return -1;
		else if (countThis > countOther)
			return 1;
		return 0;				
		}

	// Pseudo constructors.

	/// Any tuple
	public static final ZTBSpec sAny()
		{ return new ZTBSpec(true); }

	/// No tuple
	public static final ZTBSpec sNone()
		{ return new ZTBSpec(); }

	/// Tuples which have a property \a iPropName
	public static final ZTBSpec sHas(String iPropName)
		{ return new ZTBSpec(new Criterion(iPropName)); }

	/// Tuples which do not have a property \a iPropName
	public static final ZTBSpec sLacks(String iPropName)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_Lacks, null)); }

	/// Tuples which have a property iPropName of type iType
	// static ZTBSpec sHas(String iPropName, ZType iType);

	/// Tuples whose property \a iPropName has the value \a iValue
	public static final ZTBSpec sEquals(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_Equal, iValue)); }

	/// Tuples whose property \a iPropName is not the value \a iValue
	public static final ZTBSpec sNotEqual(String iPropName, Object iValue)
		{ return sLess(iPropName, iValue).or(sGreater(iPropName, iValue)); }

	/// Tuples whose property \a iPropName is less than the value \a iValue
	public static final ZTBSpec sLess(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_Less, iValue)); }

	/// Tuples whose property \a iPropName is less than or equal to the value \a iValue
	public static final ZTBSpec sLessEqual(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_LessEqual, iValue)); }

	/// Tuples whose property \a iPropName is greater than the value \a iValue
	public static final ZTBSpec sGreater(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_Greater, iValue)); }

	/// Tuples whose property \a iPropName is greater than or equal to the value \a iValue
	public static final ZTBSpec sGreaterEqual(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_GreaterEqual, iValue)); }

	public static final ZTBSpec sStringContains(String iPropName, String iString, int iLevel)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_StringContains, iLevel, iString)); }

	public static final ZTBSpec sVectorContains(String iPropName, Object iValue)
		{ return new ZTBSpec(new Criterion(iPropName, eRel_VectorContains, iValue)); }

	/// Tuples whose string property \a iPropName match the regex \a iRegex
//	public static final ZTBSpec sRegex(String iPropName, String iRegex)
//		{ return new ZTBSpec(new Criterion(iPropName, eRel_Equal, iValue)); }

	private static final Criterion[][] sCrossProduct(Criterion[][] iSourceA, Criterion[][] iSourceB)
		{
		Criterion[][] newCriteria = new Criterion[iSourceA.length * iSourceB.length][];
		for (int iterA = 0; iterA < iSourceA.length; ++iterA)
			{
			for (int iterB = 0; iterB < iSourceB.length; ++iterB)
				{
				Criterion a[] = iSourceA[iterA];
				Criterion b[] = iSourceB[iterB];
				Criterion temp[] = new Criterion[a.length + b.length];
				System.arraycopy(a, 0, temp, 0, a.length);
				System.arraycopy(b, 0, temp, a.length, b.length);
				newCriteria[iterA * iSourceB.length + iterB] = temp;
				}
			}
		return newCriteria;
		}

	public final ZTBSpec and(ZTBSpec iOther)
		{
		if (fCriteria == null)
			return this;

		if (iOther == null)
			return new ZTBSpec();

		if (iOther.fCriteria == null)
			return iOther;

		return new ZTBSpec(sCrossProduct(fCriteria, iOther.fCriteria));
		}

	public final ZTBQuery and(ZTBQuery iQuery)
		{
		if (iQuery == null)
			return iQuery;
		return iQuery.and(this);
		}

	public final ZTBSpec or(ZTBSpec iOther)
		{
		if (fCriteria == null)
			return iOther;

		if (iOther == null || iOther.fCriteria == null)
			return this;

		Criterion newCriteria[][] = new Criterion[fCriteria.length + iOther.fCriteria.length][];
		System.arraycopy(fCriteria, 0, newCriteria, 0, fCriteria.length);
		System.arraycopy(iOther.fCriteria, 0, newCriteria, fCriteria.length, iOther.fCriteria.length);
		return new ZTBSpec(newCriteria);
		}

	public final ZTBQuery or(ZTBQuery iQuery)
		{
		if (iQuery == null)
			return new ZTBQuery(this);
		return iQuery.or(this);
		}

	public boolean isAny()
		{
		if (fCriteria != null && fCriteria.length == 1 && fCriteria[0].length == 0)
			return true;
		return false;
		}

	public boolean isNone()
		{
		if (fCriteria == null || fCriteria.length == 0)
			return true;
		return false;
		}

	Criterion[][] getCriteria()
		{
		return fCriteria;
		}

	private final Criterion[][] fCriteria;
	}
