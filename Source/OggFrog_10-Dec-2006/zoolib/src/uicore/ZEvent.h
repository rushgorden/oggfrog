/*  @(#) $Id: ZEvent.h,v 1.3 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZEvent__
#define __ZEvent__ 1
#include "zconfig.h"

#include "ZGeom.h" // For ZPoint
#include "ZKeyboard.h"
#include "ZMessage.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZEvent_Key

class ZEvent_Key
	{
public:
	ZEvent_Key(const ZMessage& inMessage);

	const ZMessage& GetMessage() const
		{ return fMessage; }

	ZKeyboard::CharCode GetCharCode() const;
	ZKeyboard::KeyCode GetKeyCode() const;
	bool GetShift() const;
	bool GetCommand() const;
	bool GetOption() const;
	bool GetControl() const;
	bool GetCapsLock() const;

//	bool GetMeta() const;

protected:
	ZMessage fMessage;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZEvent_Mouse

class ZEvent_Mouse
	{
public:
	ZEvent_Mouse(const ZMessage& inMessage);

	const ZMessage& GetMessage() const
		{ return fMessage; }

	long GetClickCount() const;
	ZPoint GetLocation() const;
	bool GetShift() const;
	bool GetCommand() const;
	bool GetOption() const;
	bool GetControl() const;
	bool GetCapsLock() const;
	bool GetMeta() const;

protected:
	ZMessage fMessage;
	};

#endif //  __ZEvent__
