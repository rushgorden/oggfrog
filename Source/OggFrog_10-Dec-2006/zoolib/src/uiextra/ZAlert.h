/*  @(#) $Id: ZAlert.h,v 1.1.1.1 2002/02/17 18:39:53 agreen Exp $ */

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

#ifndef __ZAlert__
#define __ZAlert__ 1
#include "zconfig.h"

#include "ZTypes.h"
#include "ZUI.h"

#include <string>
#include <vector>

class ZWindow;

namespace ZAlert {

enum Result { eResultCancel, eResultOK, eResultNo }; // OK is non-zero, cancel is zero

enum Buttons { eButtonsOK, eButtonsOKCancel_OKDefault, eButtonsOKCancel_CancelDefault, eButtonsSaveChanges, eButtonsOKNoCancel };

enum Level { eLevelNote, eLevelCaution, eLevelStop,
// And the Windows equivalent names
				eLevelInformation = eLevelNote, eLevelWarning = eLevelCaution, eLevelCritical = eLevelStop };

ZAlert::Result sNoteAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut = -1);
ZAlert::Result sNoteAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut = -1);
ZAlert::Result sCautionAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut = -1);
ZAlert::Result sCautionAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut = -1);
ZAlert::Result sStopAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut = -1);
ZAlert::Result sStopAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut = -1);

ZAlert::Result sSaveChanges(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut = -1);
ZAlert::Result sOKNoCancel(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut = -1);

int32 sAlert(Level inLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
								const vector<ZRef<ZUICaption> >& inButtonCaptions, int32 inDefaultButton, int32 inEscapeButton);
int32 sAlert(Level inLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
								const vector<string>& inButtonTitles, int32 inDefaultButton, int32 inEscapeButton);

// AG 97-11-04 Can't think of a good name for this right now.
ZWindow* sTextBox(const string& inMessage);

} // namespace ZAlert

#endif // __ZAlert__
