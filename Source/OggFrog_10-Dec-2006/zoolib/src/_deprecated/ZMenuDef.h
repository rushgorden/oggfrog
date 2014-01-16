/*  @(#) $Id: ZMenuDef.h,v 1.2 2002/03/15 19:07:58 agreen Exp $ */

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

#ifndef __ZMenuDef__
#define __ZMenuDef__ 1
#include "zconfig.h"

#define mcNoCommand 0


#define mcAppleMenu 100
#define mcDAChosen 101
#define mcAbout 102

#define mcFileMenu 110
#define mcNew 111
#define mcOpen 112
#define mcClose 113
#define mcSave 114
#define mcSaveAs 115
#define mcRevert 116
#define mcPageSetup 117
#define mcPrint 118
#define mcQuit 119

#define mcEditMenu 130
#define mcUndo 131
#define mcCut 132
#define mcCopy 133
#define mcPaste 134
#define mcClear 135
#define mcDelete 136
#define mcSelectAll 137

#define mcPreferences 140

#define mcWindowMenu 150
#define mcCascade 151
#define mcTile 152
#define mcArrange 153
#define mcCloseAll 154

// AG 98-09-17. New defines for font handling. See ZFontMenus.h/cpp
#define mcFontMenu 155
#define mcFontItem 156
#define mcStyleMenu 157
#define mcStyleItem 158
#define mcSizeMenu 159
#define mcSizeItem 160


// A base number for user commands -- just use mcUser + 0, mcUser + 1 etc
#define mcUser 20000

#endif // __ZMenuDef__
