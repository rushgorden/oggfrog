/*  @(#) $Id: ZUtil_File.h,v 1.5 2005/09/02 18:53:48 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2000 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZUtil_File__
#define __ZUtil_File__
#include "zconfig.h"

#include "ZFile_MacClassic.h"
#include "ZFile_Win.h"

namespace ZUtil_File {

#if ZCONFIG_API_Enabled(File_MacClassic)

bool sOpenFileDialog(const string& iPrompt, const vector<OSType>& iTypes, ZFileSpec& oSpec);
bool sChooseFolderDialog(const string& iPrompt, ZFileSpec& oSpec);
bool sSaveFileDialog(const string& iPrompt, const string* iSuggestedName, const ZFileSpec* iLocation, ZFileSpec& oSpec);

#endif // ZCONFIG_API_Enabled(File_MacClassic)


#if ZCONFIG_API_Enabled(File_Win)

bool sOpenFileDialog(const string& iPrompt, const vector<pair<string, string> >& iTypes, ZFileSpec& oSpec);
bool sSaveFileDialog(const string& iPrompt, const string* iSuggestedName, const ZFileSpec* iLocation, ZFileSpec& oSpec);

#endif // ZCONFIG_API_Enabled(File_MacClassic)


} // namespace ZUtil_File

#endif // __ZUtil_File__
