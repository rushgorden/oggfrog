static const char rcsid[] = "@(#) $Id: ZAsset_MacOS.cpp,v 1.4 2006/07/15 20:54:22 goingware Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZAsset_MacOS.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/Resources.h>
#else
#	include <Resources.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZAssetTree_MacOS_Resource

ZAssetTree_MacOS_Resource::ZAssetTree_MacOS_Resource(Handle iResourceHandle, bool iAdopt)
	{
	ZAssertStop(1, iResourceHandle && *iResourceHandle);
	fResourceHandle = iResourceHandle;
	fAdopted = iAdopt;
	::HLockHi(fResourceHandle);

	ZAssetTree_Std_Memory::LoadUp(*fResourceHandle, ::GetHandleSize(fResourceHandle));
	}

ZAssetTree_MacOS_Resource::~ZAssetTree_MacOS_Resource()
	{
	ZAssetTree_Std_Memory::ShutDown();

	if (fAdopted)
		::ReleaseResource(fResourceHandle);
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
