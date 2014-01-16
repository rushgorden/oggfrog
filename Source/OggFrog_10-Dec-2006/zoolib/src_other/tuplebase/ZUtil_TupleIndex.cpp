static const char rcsid[] = "@(#) $Id: ZUtil_TupleIndex.cpp,v 1.1 2006/10/13 20:23:54 agreen Exp $";

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

#include "ZUtil_TupleIndex.h"

#include "ZTupleIndex_General.h"
#include "ZTupleIndex_String.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_TupleIndex

ZRef<ZTupleIndexFactory> ZUtil_TupleIndex::sCreate_General(const string& iPropName0)
	{
	vector<string> propNames;

	propNames.push_back(iPropName0);

	return new ZTupleIndexFactory_General(propNames);
	}

ZRef<ZTupleIndexFactory> ZUtil_TupleIndex::sCreate_General(
	const string& iPropName0,
	const string& iPropName1)
	{
	vector<string> propNames;

	propNames.push_back(iPropName0);
	propNames.push_back(iPropName1);

	return new ZTupleIndexFactory_General(propNames);
	}

ZRef<ZTupleIndexFactory> ZUtil_TupleIndex::sCreate_General(
	const string& iPropName0,
	const string& iPropName1,
	const string& iPropName2)
	{
	vector<string> propNames;

	propNames.push_back(iPropName0);
	propNames.push_back(iPropName1);
	propNames.push_back(iPropName2);

	return new ZTupleIndexFactory_General(propNames);
	}

ZRef<ZTupleIndexFactory> ZUtil_TupleIndex::sCreate_String(const string& iPropName)
	{
	return new ZTupleIndexFactory_String(iPropName);
	}
