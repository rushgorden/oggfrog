static const char rcsid[] = "@(#) $Id: ZTuple_Java.cpp,v 1.4 2004/05/05 15:25:59 agreen Exp $";

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

#include "ZTuple_Java.h"
#include "ZMemory.h"
#include "ZMemoryBlock.h"

static string sGetString(JNIEnv* iEnv, jstring iString)
	{
	jboolean isCopy;
	return iEnv->GetStringUTFChars(iString, &isCopy);
	}

static jstring sGetString(JNIEnv* iEnv, const string& iString)
	{
	return iEnv->NewStringUTF(iString.c_str());
	}

static bool sCheckClass(JNIEnv* iEnv, const char* iClassName, jobject iValue, jclass& oClass)
	{
	oClass = iEnv->FindClass(iClassName);
	if (oClass)
		{
		if (iEnv->IsInstanceOf(iValue, oClass))
			return true;
		iEnv->DeleteLocalRef(oClass);
		}
	return false;
	}

static bool sCheckClass(JNIEnv* iEnv, const char* iClassName, jobject iValue)
	{
	if (jclass theClass = iEnv->FindClass(iClassName))
		{
		bool result = iEnv->IsInstanceOf(iValue, theClass);
		iEnv->DeleteLocalRef(theClass);
		return result;
		}
	return false;
	}

static ZTupleValue sVectorToTupleValue(JNIEnv* iEnv, jclass iClass_Vector, jobject iVector)
	{
	// use iClass_Vector
	jfieldID fid_elementCount = iEnv->GetFieldID(iClass_Vector, "elementCount", "I");
	jfieldID fid_elementData = iEnv->GetFieldID(iClass_Vector, "elementData", "[Ljava/lang/Object;");

	ZTupleValue theTV;
	vector<ZTupleValue>& theVector = theTV.SetMutableVector();

	if (jobjectArray theObjectArray = static_cast<jobjectArray>(iEnv->GetObjectField(iVector, fid_elementData)))
		{
		size_t theCount = iEnv->GetIntField(iVector, fid_elementCount);

		for (size_t x = 0; x < theCount; ++x)
			{
			jobject theObject = iEnv->GetObjectArrayElement(theObjectArray, x);
			theVector.push_back(ZTuple_Java::sObjectToTupleValue(iEnv, theObject));
			iEnv->DeleteLocalRef(theObject);
			}
		iEnv->DeleteLocalRef(theObjectArray);
		}
	iEnv->DeleteLocalRef(iClass_Vector);
	return theTV;
	}

ZTupleValue ZTuple_Java::sObjectToTupleValue(JNIEnv* iEnv, jobject iValue)
	{
	ZTupleValue theTV;
	if (!iValue)
		return theTV;
	jclass theClass;

	if (false)
		{
		}
	else if (sCheckClass(iEnv, "java/util/Map", iValue, theClass))
		{
		theTV = sMapToTuple(iEnv, theClass, iValue);
		}
	else if (sCheckClass(iEnv, "java/lang/String", iValue))
		{
		theTV = sGetString(iEnv, (jstring)iValue);
		}
	else if (sCheckClass(iEnv, "java/lang/Boolean", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "booleanValue", "()Z"))
			theTV.SetBool(iEnv->CallBooleanMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Byte", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "byteValue", "()B"))
			theTV.SetInt8(iEnv->CallByteMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Character", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "charValue", "()C"))
			{
			// Hmmm.
			theTV.SetInt16(iEnv->CallCharMethod(iValue, mid));
			}
		}
	else if (sCheckClass(iEnv, "java/lang/Short", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "shortValue", "()S"))
			theTV.SetInt16(iEnv->CallShortMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Integer", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "intValue", "()I"))
			theTV.SetInt32(iEnv->CallIntMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Long", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "longValue", "()J"))
			theTV.SetInt64(iEnv->CallLongMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Float", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "floatValue", "()F"))
			theTV.SetFloat(iEnv->CallFloatMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/lang/Double", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "doubleValue", "()D"))
			theTV.SetDouble(iEnv->CallDoubleMethod(iValue, mid));
		}
	else if (sCheckClass(iEnv, "java/util/Date", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "getTime", "()J"))
			{
			// Convert from milliseconds to seconds.
			theTV.SetTime(iEnv->CallLongMethod(iValue, mid)/1000.0);
			}
		}
	else if (sCheckClass(iEnv, "java/util/Vector", iValue, theClass))
		{
		theTV = sVectorToTupleValue(iEnv, theClass, iValue);
		}
	else if (sCheckClass(iEnv, "org/zoolib/tuplebase/ZID", iValue, theClass))
		{
		if (jmethodID mid = iEnv->GetMethodID(theClass, "longValue", "()J"))
			theTV.SetID(iEnv->CallLongMethod(iValue, mid));
		}
	#if 0
	// This causes the whole awt to be pulled in.
	else if (sCheckClass(iEnv, "java/awt/Rectangle", iValue, theClass))
		{
		jfieldID fid_X = iEnv->GetFieldID(theClass, "x", "I");
		jfieldID fid_Y = iEnv->GetFieldID(theClass, "y", "I");
		jfieldID fid_width = iEnv->GetFieldID(theClass, "width", "I");
		jfieldID fid_height = iEnv->GetFieldID(theClass, "height", "I");
		ZRectPOD theRect;
		theRect.left = iEnv->GetIntField(iValue, fid_X);
		theRect.top = iEnv->GetIntField(iValue, fid_Y);
		theRect.right = theRect.left + iEnv->GetIntField(iValue, fid_width);
		theRect.bottom = theRect.top + iEnv->GetIntField(iValue, fid_height);
		theTV = theRect;
		}
	#endif
	else if (sCheckClass(iEnv, "java/awt/Point", iValue, theClass))
		{
		jfieldID fid_X = iEnv->GetFieldID(theClass, "x", "I");
		jfieldID fid_Y = iEnv->GetFieldID(theClass, "y", "I");
		ZPointPOD thePoint;
		thePoint.h = iEnv->GetIntField(iValue, fid_X);
		thePoint.v = iEnv->GetIntField(iValue, fid_Y);
		theTV = thePoint;
		}
	else if (sCheckClass(iEnv, "[Z", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			ZAssertCompile(sizeof(jboolean) == sizeof(bool));
			bool* elems = static_cast<bool*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		}
	else if (sCheckClass(iEnv, "[B", iValue))
		{
		ZMemoryBlock theMemoryBlock;
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			theMemoryBlock.SetSize(count);
			jboolean isCopy;
			int8* elems = static_cast<int8*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theMemoryBlock.CopyFrom(0, elems, count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		theTV = theMemoryBlock;
		}
	else if (sCheckClass(iEnv, "[C", iValue))
		{
		// printf("char Array, not handled yet\n");
		}
	else if (sCheckClass(iEnv, "[S", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			int16* elems = static_cast<int16*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		}
	else if (sCheckClass(iEnv, "[I", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			int32* elems = static_cast<int32*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
 		}
	else if (sCheckClass(iEnv, "[J", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			int64* elems = static_cast<int64*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		}
	else if (sCheckClass(iEnv, "[F", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			float* elems = static_cast<float*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		}
	else if (sCheckClass(iEnv, "[D", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jarray theArray = static_cast<jarray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			jboolean isCopy;
			double* elems = static_cast<double*>(iEnv->GetPrimitiveArrayCritical(theArray, &isCopy));
			theVector.insert(theVector.begin(), elems, elems + count);
			iEnv->ReleasePrimitiveArrayCritical(theArray, elems, JNI_ABORT);
			}
		}
	else if (sCheckClass(iEnv, "[Ljava/lang/Object;", iValue))
		{
		vector<ZTupleValue>& theVector = theTV.SetMutableVector();
		jobjectArray theArray = static_cast<jobjectArray>(iValue);
		if (size_t count = iEnv->GetArrayLength(theArray))
			{
			for (size_t x = 0; x < count; ++x)
				{
				jobject theObject = iEnv->GetObjectArrayElement(theArray, x);
				theVector.push_back(sObjectToTupleValue(iEnv, theObject));
				iEnv->DeleteLocalRef(theObject);
				}
			}
		}

	return theTV;
	}

ZTuple ZTuple_Java::sMapToTuple(JNIEnv* iEnv, jobject iMap)
	{
	return sMapToTuple(iEnv, iEnv->FindClass("java/util/Map"), iMap);
	}

ZTuple ZTuple_Java::sMapToTuple(JNIEnv* iEnv, jclass iClass_Map, jobject iMap)
	{
	ZTuple theTuple;
	jclass class_String = iEnv->FindClass("java/lang/String");
	jclass class_Set = iEnv->FindClass("java/util/Set");
	jclass class_Iterator = iEnv->FindClass("java/util/Iterator");
	jclass class_Map_Entry = iEnv->FindClass("java/util/Map$Entry");

	if (jobject theEntrySet = iEnv->CallObjectMethod(iMap, iEnv->GetMethodID(iClass_Map, "entrySet", "()Ljava/util/Set;")))
		{
		if (jobject theIterator = iEnv->CallObjectMethod(theEntrySet, iEnv->GetMethodID(class_Set, "iterator", "()Ljava/util/Iterator;")))
			{
			jmethodID mid_hasNext = iEnv->GetMethodID(class_Iterator, "hasNext", "()Z");
			jmethodID mid_next = iEnv->GetMethodID(class_Iterator, "next", "()Ljava/lang/Object;");
			jmethodID mid_getKey = iEnv->GetMethodID(class_Map_Entry, "getKey", "()Ljava/lang/Object;");
			jmethodID mid_getValue = iEnv->GetMethodID(class_Map_Entry, "getValue", "()Ljava/lang/Object;");
			for (;;)
				{
				if (!iEnv->CallBooleanMethod(theIterator, mid_hasNext))
					break;
				if (jobject theEntry = iEnv->CallObjectMethod(theIterator, mid_next))
					{
					if (iEnv->IsInstanceOf(theEntry, class_Map_Entry))
						{
						if (jobject theKey = iEnv->CallObjectMethod(theEntry, mid_getKey))
							{
							if (iEnv->IsInstanceOf(theKey, class_String))
								{
								string propertyName = sGetString(iEnv, jstring(theKey));
								if (jobject theValue = iEnv->CallObjectMethod(theEntry, mid_getValue))
									{
									theTuple.SetValue(propertyName, sObjectToTupleValue(iEnv, theValue));
									iEnv->DeleteLocalRef(theValue);
									}
								}
							iEnv->DeleteLocalRef(theKey);
							}
						}						
					iEnv->DeleteLocalRef(theEntry);
					}
				}
			iEnv->DeleteLocalRef(theIterator);
			}
		iEnv->DeleteLocalRef(theEntrySet);
		}
	
	// use iClass_Map
	iEnv->DeleteLocalRef(iClass_Map);
	return theTuple;
	}

jobject ZTuple_Java::sTupleValueToObject(JNIEnv* iEnv, const ZTupleValue& iTV)
	{
	switch (iTV.TypeOf())
		{
		case eZType_Null:
			{
			// This is okay -- a null is a nil object.
			break;
			}
		case eZType_ID:
			{
			if (jclass theClass = iEnv->FindClass("org/zoolib/tuplebase/ZID"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(J)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetID());
				}
			break;
			}
		case eZType_Int8:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Byte"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(B)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetInt8());
				}
			break;
			}
		case eZType_Int16:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Short"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(S)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetInt16());
				}
			break;
			}
		case eZType_Int32:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Integer"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(I)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetInt32());
				}
			break;
			}
		case eZType_Int64:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Long"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(J)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetInt64());
				}
			break;
			}
		case eZType_Bool:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Boolean"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(Z)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetBool());
				}
			break;
			}
		case eZType_Float:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Float"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(F)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetFloat());
				}
			break;
			}
		case eZType_Double:
			{
			if (jclass theClass = iEnv->FindClass("java/lang/Double"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(D)V"))
					return iEnv->NewObject(theClass, theMID, iTV.GetDouble());
				}
			break;
			}
		case eZType_Time:
			{
			if (jclass theClass = iEnv->FindClass("java/util/Date"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(J)V"))
					{
					// Convert from seconds to milliseconds.
					return iEnv->NewObject(theClass, theMID, int64(iTV.GetTime().fVal * 1000));
					}
				}
			break;
			}
		case eZType_Pointer:
			{
			// Not supported
			break;
			}
		case eZType_Raw:
			{
			const void* sourceAddress;
			size_t sourceSize;
			iTV.GetRawAttributes(&sourceAddress, &sourceSize);
			jbyteArray theByteArray = iEnv->NewByteArray(sourceSize);
			jboolean isCopy;
			void* destAddress = iEnv->GetPrimitiveArrayCritical(theByteArray, &isCopy);
			ZBlockCopy(sourceAddress, destAddress, sourceSize);
			iEnv->ReleasePrimitiveArrayCritical(theByteArray, destAddress, JNI_ABORT);
			return theByteArray;
			}
		case eZType_Tuple:
			{
			return sTupleToMap(iEnv, iTV.GetTuple());
			}
		case eZType_RefCounted:
			{
			// Not supported
			break;
			}
		#if 0
		// This causes the whole awt to be pulled in.
		case eZType_Rect:
			{
			if (jclass theClass = iEnv->FindClass("java/awt/Rectangle"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(IIII)V"))
					{
					ZRectPOD theRect = iTV.GetRect();
					return iEnv->NewObject(theClass, theMID, jint(theRect.left), jint(theRect.top), jint(theRect.right - theRect.left), jint(theRect.bottom - theRect.top));
					}
				}
			break;
			}		
		#endif
		case eZType_Point:
			{
			if (jclass theClass = iEnv->FindClass("java/awt/Point"))
				{
				if (jmethodID theMID = iEnv->GetMethodID(theClass, "<init>", "(II)V"))
					{
					ZPointPOD thePoint = iTV.GetPoint();
					return iEnv->NewObject(theClass, theMID, jint(thePoint.h), jint(thePoint.v));
					}
				}
			break;
			}
		case eZType_Region:
			{
			// Not supported -- use Rect[]?
			break;
			}
		case eZType_String:
			{
			return sGetString(iEnv, iTV.GetString());
			}
		case eZType_Vector:
			{
			const vector<ZTupleValue>& theVector = iTV.GetVector();
			size_t theCount = theVector.size();
			jclass theClass = iEnv->FindClass("java/lang/Object");
			jobjectArray theArray = iEnv->NewObjectArray(theCount, theClass, nil);
			for (size_t x = 0; x < theCount; ++x)
				{
				jobject theObject = sTupleValueToObject(iEnv, theVector[x]);
				iEnv->SetObjectArrayElement(theArray, x, theObject);
				iEnv->DeleteLocalRef(theObject);
				}
			return theArray;
			}
		case eZType_Type:
			{
			// Not supported
			break;
			}
		}
	return nil;
	}

jobject ZTuple_Java::sTupleToMap(JNIEnv* iEnv, const ZTuple& iTuple)
	{
	// Later we might return a native class that simply wraps the tuple.
	// For now I'm creating a TreeMap.
	jclass class_TreeMap = iEnv->FindClass("java/util/TreeMap");
	jclass class_String = iEnv->FindClass("java/lang/String");
	jmethodID mid_Map_Construct = iEnv->GetMethodID(class_TreeMap, "<init>", "()V");
	jmethodID mid_Map_Put = iEnv->GetMethodID(class_TreeMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	jobject theMap = iEnv->NewObject(class_TreeMap, mid_Map_Construct);
	if (ZRef<ZTupleRep> theRep = iTuple.fRep)
		{
		const vector<pair<string, ZTupleValue> >& properties = theRep->fProperties;
		vector<pair<string, ZTupleValue> >::const_iterator theEnd = properties.end();		
		for (vector<pair<string, ZTupleValue> >::const_iterator i = properties.begin(); i != theEnd; ++i)
			{
			jstring propertyName = sGetString(iEnv, (*i).first);
			jobject value = sTupleValueToObject(iEnv, (*i).second);
			iEnv->CallObjectMethod(theMap, mid_Map_Put, propertyName, value);
			iEnv->DeleteLocalRef(propertyName);
			iEnv->DeleteLocalRef(value);
			}
		}

	return theMap;
	}