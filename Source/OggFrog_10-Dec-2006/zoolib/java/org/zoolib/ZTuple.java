// "@(#) $Id: ZTuple.java,v 1.20 2006/10/29 03:42:22 agreen Exp $";

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


import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.Writer;

public final class ZTuple implements Map, java.io.Serializable
	{
	public static final int eZType_Null = 0;
	public static final int eZType_String = 1;
	public static final int eZType_CString = 2;
	public static final int eZType_Int8 = 3;
	public static final int eZType_Int16 = 4;
	public static final int eZType_Int32 = 5;
	public static final int eZType_Int64 = 6;
	public static final int eZType_Float = 7;
	public static final int eZType_Double = 8;
	public static final int eZType_Bool = 9;
	public static final int eZType_Pointer = 10;
	public static final int eZType_Raw = 11;
	public static final int eZType_Tuple = 12;
	public static final int eZType_RefCounted = 13;
	public static final int eZType_Rect = 14;
	public static final int eZType_Point = 15;
	public static final int eZType_Region = 16;
	public static final int eZType_ID = 17;
	public static final int eZType_Vector = 18;
	public static final int eZType_Type = 19;
	public static final int eZType_Time = 20;

	public static final int sTypeOf(Object iObject)
		{
		if (iObject instanceof Number)
			{
			if (iObject instanceof Byte)
				return eZType_Int8;
			else if (iObject instanceof Short)
				return eZType_Int16;
			else if (iObject instanceof Integer)
				return eZType_Int32;
			else if (iObject instanceof Long)
				return eZType_Int64;
			else if (iObject instanceof Float)
				return eZType_Float;
			else if (iObject instanceof Double)
				return eZType_Double;
			else
				return eZType_Null;
			}
		else if (iObject instanceof String)
			return eZType_String;
		else if (iObject instanceof Boolean)
			return eZType_Bool;
		else if (iObject instanceof byte[])
			return eZType_Raw;
		else if (iObject instanceof Map)
			return eZType_Tuple;
		else if (iObject instanceof java.awt.Point)
			return eZType_Point;
		else if (iObject instanceof ZID)
			return eZType_ID;
		else if (iObject instanceof List)
			return eZType_Vector;
		else if (iObject instanceof Date)
			return eZType_Time;
		else
			return eZType_Null;
		}

	public static final int sCompareTV(Object iLeft, Object iRight)
		{
		int typeLeft = sTypeOf(iLeft);
		int typeRight = sTypeOf(iRight);

		if (typeLeft < typeRight)
			return -1;
		else if (typeLeft > typeRight)
			return 1;
		else
			return sCompareTVUnchecked(typeLeft, iLeft, iRight);
		}

	public static final int sCompareDouble(double iLeft, double iRight)
		{
		if (Double.isNaN(iLeft))
			{
			if (Double.isNaN(iRight))
				return 0;
			return -1;
			}
		else if (Double.isNaN(iRight))
			{
			return 1;
			}
		else
			{
			if (iLeft < iRight)
				return -1;
			else if (iLeft > iRight)
				return 1;
			else
				return 0;
			}
		}

	public static final int sCompareTVUnchecked(int iType, Object iLeft, Object iRight)
		{
		switch (iType)
			{
			case eZType_Null: return 0;
			case eZType_String: return ((String)iLeft).compareTo((String)iRight);
			case eZType_Int8: return ((Byte)iLeft).intValue() - ((Byte)iRight).intValue();
			case eZType_Int16: return ((Short)iLeft).intValue() - ((Short)iRight).intValue();
			case eZType_Int32: return ((Integer)iLeft).intValue() - ((Integer)iRight).intValue();
			case eZType_Int64:
				{
				long valueLeft = ((Long)iLeft).longValue();
				long valueRight = ((Long)iRight).longValue();
				if (valueLeft < valueRight)
					return -1;
				else if (valueLeft > valueRight)
					return 1;
				else
					return 0;
				}
			case eZType_Float:
				{
				Float floatLeft = (Float)iLeft;
				Float floatRight = (Float)iRight;
				if (floatLeft.isNaN())
					{
					if (floatRight.isNaN())
						return 0;
					return -1;
					}
				else if (floatRight.isNaN())
					{
					return 1;
					}
				else
					{
					float valueLeft = floatLeft.floatValue();
					float valueRight = floatRight.floatValue();
					if (valueLeft < valueRight)
						return -1;
					else if (valueLeft > valueRight)
						return 1;
					else
						return 0;
					}
				}
			case eZType_Double:
				{
				return sCompareDouble(((Double)iLeft).doubleValue(), ((Double)iRight).doubleValue());
				}
			case eZType_Bool:
				{
				boolean valueLeft = ((Boolean)iLeft).booleanValue();
				boolean valueRight = ((Boolean)iRight).booleanValue();
				if (valueLeft)
					{
					if (valueRight)
						return 0;
					else
						return 1;
					}
				else
					{
					if (valueRight)
						return -1;
					else
						return 0;
					}
				}
			case eZType_Raw:
				{
				byte[] valueLeft = (byte[])iLeft;
				byte[] valueRight = (byte[])iRight;
				int countLeft = valueLeft.length;
				int countRight = valueRight.length;
				int shortest = Math.min(countLeft, countRight);
				for (int x = 0; x < shortest; ++x)
					{
					byte tempLeft = valueLeft[x];
					byte tempRight = valueLeft[x];
					if (tempLeft < tempRight)
						return -1;
					else if (tempLeft > tempRight)
						return 1;
					}
				if (countLeft < countRight)
					return -1;
				else if (countLeft > countRight)
					return 1;
				return 0;
				}
			case eZType_Tuple:
				{
				// Don't know how best to compare tuples.
				ZDebug.sAssert(false);
				}
			case eZType_Point:
				{
				java.awt.Point valueLeft = (java.awt.Point)iLeft;
				java.awt.Point valueRight = (java.awt.Point)iRight;
				if (valueLeft.x < valueRight.x)
					return -1;
				else if (valueLeft.x > valueRight.x)
					return 1;
				else if (valueLeft.y < valueRight.y)
					return -1;
				else if (valueLeft.y > valueRight.y)
					return 1;
				else
					return 0;
				}
			case eZType_ID:
				{
				long valueLeft = ((ZID)iLeft).longValue();
				long valueRight = ((ZID)iRight).longValue();
				if (valueLeft < valueRight)
					return -1;
				else if (valueLeft > valueRight)
					return 1;
				else
					return 0;
				}
			case eZType_Vector:
				{
				List listLeft = (List)iLeft;
				List listRight = (List)iRight;
				int countLeft = listLeft.size();
				int countRight = listRight.size();
				int shortest = Math.min(countLeft, countRight);
				for (int x = 0; x < shortest; ++x)
					{
					Object tempLeft = listLeft.get(x);
					Object tempRight = listRight.get(x);
					int result = sCompareTV(listLeft.get(x), listRight.get(x));
					if (result != 0)
						return result;
					}
				if (countLeft < countRight)
					return -1;
				else if (countLeft > countRight)
					return 1;
				return 0;
				}
			case eZType_Time:
				{
				Date dateLeft = (Date)iLeft;
				Date dateRight = (Date)iRight;
				double doubleLeft = dateLeft.getTime() / 1000.00;
				double doubleRight = dateRight.getTime() / 1000.00;
				return sCompareDouble(doubleLeft, doubleRight);
				}
			}
		return 0;
		}

	public ZTuple()
		{
		fMap = new TreeMap();
		}

	public ZTuple(Map iMap)
		{
		if (iMap == null)
			fMap = new TreeMap();
		else if (iMap instanceof ZTuple)
			fMap = new TreeMap(((ZTuple)iMap).fMap);
		else
			fMap = new TreeMap(iMap);
		}

	public ZTuple(Map iMap, boolean iClone)
		{
		if (iMap == null)
			{
			fMap = new TreeMap();
			}
		else
			{
			if (iMap instanceof ZTuple)
				iMap = ((ZTuple)iMap).fMap;

			if (iClone)
				fMap = new TreeMap(iMap);
			else
				fMap = iMap;
			}
		}

	public ZTuple(InputStream iStream) throws IOException
		{
		fMap = sReadMap(new DataInputStream(iStream));
		}

	public ZTuple(DataInput iDataInput) throws IOException
		{
		fMap = sReadMap(iDataInput);
		}

	public void toStream(OutputStream iStream) throws IOException
		{
		sWriteMap(new DataOutputStream(iStream), fMap);
		}

	public void toDataOutput(DataOutput iDO) throws IOException
		{
		sWriteMap(iDO, fMap);
		}

//	public void fromStream(InputStream iStream) throws IOException
//		{
//		fMap = sReadMap(new DataInputStream(iStream));
//		}

//	public void fromDataInput(DataInput iDI) throws IOException
//		{
//		fMap = sReadMap(iDI);
//		}

	public final void toWriter(Writer iW) throws IOException
		{
		sWriteMapToWriter(iW, fMap);
		}

// Unioning tuples
	public final ZTuple over(ZTuple iUnder)
		{
		if (fMap != null)
			return (ZTuple)iUnder.clone();

		if (iUnder.fMap == null)
			return (ZTuple)this.clone();

		ZTuple result = ((ZTuple)iUnder.clone());
		result.putAll(fMap);
		return result;
		}

	public final ZTuple under(ZTuple iOver) { return iOver.over(this); }


// Property meta-data.
	public final boolean empty()
		{
		if (fMap != null)
			return fMap.isEmpty();
		return true;
		}

	public final int count()
		{
		if (fMap != null)
			return fMap.size();
		return 0;
		}

	public final boolean has(String iPropName)
		{
		return fMap.containsKey(iPropName);
		}

	public final int typeOf(String iPropName)
		{
		return sTypeOf(fMap.get(iPropName));
		}

/*	bool TypeOf(size_t iPropNum, ZType& oPropertyType) const;
	bool TypeOf(const char* iPropName, ZType& oPropertyType) const;
	bool TypeOf(const string& iPropName, ZType& oPropertyType) const;

	ZType TypeOf(size_t iPropNum) const;
	ZType TypeOf(const char* iPropName) const;
	ZType TypeOf(const string& iPropName) const;*/


	public final void erase(String iPropName)
		{
		if (fMap != null)
			fMap.remove(iPropName);
		}

// Getting.
	public final Object getValue(String iPropName)
		{
		return fMap.get(iPropName);
		}

/*	ZType getType(String iPropName)
		{
		}*/

	public final long getID(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof ZID)
			return ((ZID)ob).longValue();
		return 0;
		}

	public final ZID getZID(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof ZID)
			return (ZID)ob;
		return new ZID();
		}

	public final byte getInt8(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Byte)
			return ((Byte)ob).byteValue();
		return 0;
		}

	public final short getInt16(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Short)
			return ((Short)ob).shortValue();
		return 0;
		}

	public final int getInt32(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Integer)
			return ((Integer)ob).intValue();
		return 0;
		}

	public final long getInt64(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Long)
			return ((Long)ob).longValue();
		return 0;
		}

	public final Long getLong(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Long)
			return (Long)ob;
		return new Long(0);
		}

	public final boolean getBool(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Boolean)
			return ((Boolean)ob).booleanValue();
		return false;
		}

	public final float getFloat(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Float)
			return ((Float)ob).floatValue();
		return 0;
		}

	public final double getDouble(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Double)
			return ((Double)ob).doubleValue();
		return 0;
		}

	public final Date getTime(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Date)
			return (Date)ob;
		return new Date(0);
		}

	public final java.awt.Point getPoint(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof java.awt.Point)
			return (java.awt.Point)ob;
		return new java.awt.Point();
		}

	public final String getString(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof String)
			return (String)ob;
		return "";
		}

	public final ZTuple getTuple(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Map)
			return new ZTuple((Map)ob);
		return new ZTuple();
		}

	public final ZTuple getTupleNoClone(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Map)
			return new ZTuple((Map)ob, false);

		return new ZTuple();
		}

	public final ZTuple getMutableTuple(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof Map)
			return new ZTuple((Map)ob, false);

		throw new NullPointerException();
		}

	public final byte[] getRaw(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof byte[])
			return (byte[])((byte[])ob).clone();

		return new byte[0];
		}

	public final byte[] getRawNoClone(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof byte[])
			return (byte[])ob;

		return new byte[0];
		}

	public final byte[] getMutableRaw(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof byte[])
			return (byte[])ob;

		throw new NullPointerException();
		}

	public final List getList(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof List)
			return new ArrayList((List)ob);

		return new ArrayList();
		}

	public final List getListNoClone(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof List)
			return (List)ob;

		return new ArrayList();
		}

	public final List getMutableList(String iPropName)
		{
		Object ob = fMap.get(iPropName);
		if (ob != null && ob instanceof List)
			return (List)ob;

		throw new NullPointerException();
		}

// Setting
	public final ZTuple setValue(String iPropName, Object iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setZID(String iPropName, ZID iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setID(String iPropName, long iVal)
		{
		fMap.put(iPropName, new ZID(iVal));
		return this;
		}

	public final ZTuple setInt8(String iPropName, byte iVal)
		{
		fMap.put(iPropName, new Byte(iVal));
		return this;
		}

	public final ZTuple setInt16(String iPropName, short iVal)
		{
		fMap.put(iPropName, new Short(iVal));
		return this;
		}

	public final ZTuple setInt32(String iPropName, int iVal)
		{
		fMap.put(iPropName, new Integer(iVal));
		return this;
		}

	public final ZTuple setInt64(String iPropName, long iVal)
		{
		fMap.put(iPropName, new Long(iVal));
		return this;
		}

	public final ZTuple setBool(String iPropName, boolean iVal)
		{
		fMap.put(iPropName, new Boolean(iVal));
		return this;
		}

	public final ZTuple setFloat(String iPropName, float iVal)
		{
		fMap.put(iPropName, new Double(iVal));
		return this;
		}

	public final ZTuple setDouble(String iPropName, double iVal)
		{
		fMap.put(iPropName, new Double(iVal));
		return this;
		}

	public final ZTuple setTime(String iPropName, Date iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setPoint(String iPropName, java.awt.Point iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setString(String iPropName, String iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setTuple(String iPropName, ZTuple iVal)
		{
		if (iVal != null)
			fMap.put(iPropName, iVal.fMap);
		return this;
		}

	public final ZTuple setTuple(String iPropName, Map iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setRaw(String iPropName, byte[] iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setList(String iPropName, List iVal)
		{
		fMap.put(iPropName, iVal);
		return this;
		}

	public final ZTuple setMutableTuple(String iPropName)
		{
		ZTuple theTuple = new ZTuple();
		fMap.put(iPropName, theTuple.fMap); //??
		return theTuple;
		}

	public final List setMutableList(String iPropName)
		{
		List theList = new ArrayList();
		fMap.put(iPropName, theList);
		return theList;
		}

// From java.lang.Object
	public Object clone()
		{
		return new ZTuple(fMap);
		}

	public String toString()
		{
		try
			{
			Writer theWriter = new java.io.StringWriter();
			this.toWriter(theWriter);
			return theWriter.toString();
			}
		catch (Throwable ex)
			{}
		return "";
		}


// From java.io.Serializable
	private void writeObject(java.io.ObjectOutputStream out) throws IOException
		{
		sWriteMap(out, fMap);
		}

	private void readObject(java.io.ObjectInputStream in) throws IOException, ClassNotFoundException
		{
		fMap = sReadMap(in);
		}


// From java.util.Map
	public void clear()
		{
		fMap.clear();
		}

	public boolean containsKey(Object iKey)
		{
		return fMap.containsKey(iKey);
		}

	public boolean containsValue(Object iValue)
		{
		return fMap.containsValue(iValue);
		}

	public Set entrySet()
		{
		return fMap.entrySet();
		}

	public boolean equals(Object iOther)
		{
		return fMap.equals(iOther);
		}

	public Object get(Object iKey)
		{
		return fMap.get(iKey);
		}

	public int hashCode()
		{
		return fMap.hashCode();
		}

	public boolean isEmpty()
		{
		return fMap.isEmpty();
		}

	public Set keySet()
		{
		return fMap.keySet();
		}

	public Object put(Object iKey, Object iValue)
		{
		return fMap.put(iKey, iValue);
		}

	public void putAll(Map iOther)
		{
		fMap.putAll(iOther);
		}

	public Object remove(Object iKey)
		{
		return fMap.remove(iKey);
		}

	public int size()
		{
		return fMap.size();
		}

	public Collection values()
		{
		return fMap.values();
		}

// Private methods
	private static void sWriteHexDigit(Writer iW, int iDigit) throws IOException
		{
		if (iDigit >= 10)
			iW.write('a' + iDigit);
		else
			iW.write('0' + iDigit);
		}

	private static void sWriteValueToWriter(Writer iW, Object iObject) throws IOException
		{
		if (iObject instanceof Number)
			{
			if (iObject instanceof Byte)
				{
				iW.write("int8(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else if (iObject instanceof Short)
				{
				iW.write("int16(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else if (iObject instanceof Integer)
				{
				iW.write("int32(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else if (iObject instanceof Long)
				{
				iW.write("int64(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else if (iObject instanceof Float)
				{
				iW.write("float(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else if (iObject instanceof Double)
				{
				iW.write("double(");
				iW.write(iObject.toString());
				iW.write(")");
				}
			else
				{
				iW.write("null");
				}
			}
		else if (iObject instanceof String)
			{
			// Need to do escaping.
			iW.write("\"");
			iW.write(iObject.toString());
			iW.write("\"");
			}
		else if (iObject instanceof Boolean)
			{
			if (((Boolean)iObject).booleanValue())
				iW.write("true");
			else
				iW.write("false");
			}
		else if (iObject instanceof byte[])
			{
			byte[] theArray = (byte[])iObject;
			if (theArray.length == 0)
				{
				iW.write("()");
				}
			else
				{
				iW.write("(");
				for (int x = 0; x < theArray.length; ++x)
					{
					sWriteHexDigit(iW, theArray[x] / 16);
					sWriteHexDigit(iW, theArray[x] % 16);
					iW.write(" ");
					}
				iW.write(")");
				}
			}
		else if (iObject instanceof Map)
			{
			sWriteMapToWriter(iW, (Map)iObject);
			}
		else if (iObject instanceof ZID)
			{
			iW.write("ID(");
			iW.write(String.valueOf(((ZID)iObject).longValue()));
			iW.write(")");
			}
		else if (iObject instanceof List)
			{
			List theList = (List)iObject;
			int count = theList.size();
			if (count == 0)
				{
				iW.write("[]");
				}
			else
				{
				iW.write("[");
				for (int x = 0; x < count; ++x)
					{
					if (x != 0)
						iW.write(", ");
					sWriteValueToWriter(iW, theList.get(x));
					}
				iW.write("]");
				}
			}
		else if (iObject instanceof Date)
			{
			iW.write("date(");
			iW.write(String.valueOf(((Date)iObject).getTime() / 1000.0));
			iW.write(")");
			iW.write("/* ");
			iW.write(((Date)iObject).toString());
			iW.write(" */");
			}
		else if (iObject instanceof java.awt.Point)
			{
			iW.write("point(");
			iW.write(String.valueOf(((java.awt.Point)iObject).x));
			iW.write(", ");
			iW.write(String.valueOf(((java.awt.Point)iObject).y));
			iW.write(")");
			}
		else
			{
			// type, rect, region not handled.
			iW.write("null");
			}
		}

	private static void sWriteMapToWriter(Writer iW, Map iMap) throws IOException
		{
		int count = iMap.size();
		if (count == 0)
			{
			iW.write("{}");
			}
		else
			{
			iW.write("{ ");
			Set theSet = iMap.entrySet();
			Iterator theIter = theSet.iterator();
			while (theIter.hasNext())
				{
				Map.Entry theEntry = (Map.Entry)(theIter.next());
				String theName;
				try
					{ theName = (String)theEntry.getKey(); }
				catch (Throwable ex)
					{ theName = new String(); }
				iW.write(theName);
				iW.write(" = ");
				sWriteValueToWriter(iW, theEntry.getValue());
				iW.write("; ");
				}
			iW.write("}");
			}
		}

	public static final Object sReadValue(DataInput iDI) throws IOException
		{
		int theType = iDI.readUnsignedByte();
		switch (theType)
			{
			case eZType_Null:
				return null;
			case eZType_String:
				{
				return ZUtil_IO.sReadString(iDI);
				}
			case eZType_Int8:
				{
				return new Byte(iDI.readByte());
				}
			case eZType_Int16:
				{
				return new Short(iDI.readShort());
				}
			case eZType_Int32:
				{
				return new Integer(iDI.readInt());
				}
			case eZType_Int64:
				{
				return new Long(iDI.readLong());
				}
			case eZType_Float:
				{
				return new Float(iDI.readFloat());
				}
			case eZType_Double:
				{
				return new Double(iDI.readDouble());
				}
			case eZType_Bool:
				{
				return new Boolean(0 != iDI.readByte());
				}
			case eZType_Pointer:
				{
				// Ignore pointers.
				int length = ZUtil_IO.sReadCount(iDI);
				iDI.skipBytes(length);
				return null;
				}
			case eZType_Raw:
				{
				int length = ZUtil_IO.sReadCount(iDI);
				byte[] theBytes = new byte[length];
				iDI.readFully(theBytes);
				return theBytes;
				}
			case eZType_Tuple:
				{
				return new ZTuple(iDI);
				}
			case eZType_Rect:
				{
				// Ignore
				iDI.skipBytes(16);
				return null;
				}
			case eZType_Point:
				{
				int theH = iDI.readInt();
				int theV = iDI.readInt();
				return new java.awt.Point(theH, theV);
				}
			case eZType_ID:
				{
				return new ZID(iDI.readLong());
				}
			case eZType_Vector: // (or list in java vernacular)
				{
				int count = ZUtil_IO.sReadCount(iDI);
				List theList = new ArrayList();
				while (count-- != 0)
					theList.add(sReadValue(iDI));
				return theList;
				}
			case eZType_Type:
				{
				// Ignore
				iDI.skipBytes(1);
				return null;
				}
			case eZType_Time:
				{
				double theDouble = iDI.readDouble();
				return new Date((long)(theDouble * 1000));
				}
			case eZType_CString: // CString, never gets used by ZTuple
			case eZType_RefCounted:
			case eZType_Region:
			}
		throw new RuntimeException("unrecognized type in ZTuple.sReadValue");
		}

	private static final Map sReadMap(DataInput iDI) throws IOException
		{
		Map theMap = new TreeMap();
		int count = ZUtil_IO.sReadCount(iDI);
		while (count-- != 0)
			{
			String thePropName = ZUtil_IO.sReadString(iDI);
			Object theValue = sReadValue(iDI);
			theMap.put(thePropName, theValue);
			}
		return theMap;
		}

	public static void sWriteValue(DataOutput iDO, Object iObject) throws IOException
		{
		if (iObject instanceof Number)
			{
			if (iObject instanceof Byte)
				{
				iDO.write(eZType_Int8);
				iDO.writeByte(((Byte)iObject).byteValue());
				}
			else if (iObject instanceof Short)
				{
				iDO.write(eZType_Int16);
				iDO.writeShort(((Short)iObject).shortValue());
				}
			else if (iObject instanceof Integer)
				{
				iDO.write(eZType_Int32);
				iDO.writeInt(((Integer)iObject).intValue());
				}
			else if (iObject instanceof Long)
				{
				iDO.write(eZType_Int64);
				iDO.writeLong(((Long)iObject).longValue());
				}
			else if (iObject instanceof Float)
				{
				iDO.write(eZType_Float);
				iDO.writeFloat(((Float)iObject).floatValue());
				}
			else if (iObject instanceof Double)
				{
				iDO.write(eZType_Double);
				iDO.writeDouble(((Double)iObject).doubleValue());
				}
			else
				{
				iDO.write(0); // null
				}
			}
		else if (iObject instanceof String)
			{
			iDO.write(eZType_String);
			ZUtil_IO.sWriteString(iDO, (String)iObject);
			}
		else if (iObject instanceof Boolean)
			{
			iDO.write(eZType_Bool);
			iDO.writeByte(((Boolean)iObject).booleanValue() ? 1 : 0);
			}
		else if (iObject instanceof byte[])
			{
			iDO.write(eZType_Raw);
			byte[] theBytes = (byte[])iObject;
			ZUtil_IO.sWriteCount(iDO, theBytes.length);
			iDO.write(theBytes);
			}
		else if (iObject instanceof Map)
			{
			iDO.write(eZType_Tuple);
			sWriteMap(iDO, (Map)iObject);
			}
		else if (iObject instanceof java.awt.Point)
			{
			iDO.write(eZType_Point);
			iDO.writeInt(((java.awt.Point)iObject).x);
			iDO.writeInt(((java.awt.Point)iObject).y);
			}
		else if (iObject instanceof ZID)
			{
			iDO.write(eZType_ID);
			iDO.writeLong(((ZID)iObject).longValue());
			}
		else if (iObject instanceof List)
			{
			iDO.write(eZType_Vector);
			List theList = (List)iObject;
			int count = theList.size();
			ZUtil_IO.sWriteCount(iDO, count);
			for (int x = 0; x < count; ++x)
				sWriteValue(iDO, theList.get(x));
			}
		else if (iObject instanceof Date)
			{
			iDO.write(eZType_Time);
			iDO.writeDouble(((Date)iObject).getTime() / 1000.0);
			}
		else
			{
			// type, rect, region not handled.
			iDO.write(eZType_Null);
			}
		}

	private static void sWriteMap(DataOutput iDO, Map iMap) throws IOException
		{
		int count = iMap.size();
		ZUtil_IO.sWriteCount(iDO, count);

		Set theSet = iMap.entrySet();
		Iterator theIter = theSet.iterator();
		while (theIter.hasNext())
			{
			Map.Entry theEntry = (Map.Entry)(theIter.next());
			String theName;
			try
				{ theName = (String)theEntry.getKey(); }
			catch (Throwable ex)
				{ theName = new String(); }
			ZUtil_IO.sWriteString(iDO, theName);
			sWriteValue(iDO, theEntry.getValue());
			--count;
			}
		// count should be zero.
		}

	private Map fMap;
	}
