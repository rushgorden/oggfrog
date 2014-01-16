/*  @(#) $Id: ZUtil_Asset.h,v 1.7 2004/05/05 15:20:57 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZUtil_Asset__
#define __ZUtil_Asset__ 1
#include "zconfig.h"

#include "ZAsset.h"
#include "ZFile.h"

#include <string>
#include <vector>

namespace ZUtil_Asset {

void sGetAssetTreeNamesFromExecutable(std::vector<std::string>& oAssetTreeNames);

ZRef<ZAssetTree> sGetAssetTreeFromExecutable(const std::string& iAssetTreeName);
ZAsset sGetAssetRootFromExecutable(const std::string& iAssetTreeName);

ZRef<ZAssetTree> sGetAssetTreeFromFileSpec(const ZFileSpec& iFileSpec);
ZAsset sGetAssetRootFromFileSpec(const ZFileSpec& iFileSpec);

ZAsset sCreateOverlay(const std::vector<ZAsset>& iAssets, bool iLowestToHighest);

} // namespace ZUtil_Asset

#endif // __ZUtil_Asset__
