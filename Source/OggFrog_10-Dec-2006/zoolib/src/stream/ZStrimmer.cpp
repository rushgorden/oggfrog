static const char rcsid[] = "@(#) $Id: ZStrimmer.cpp,v 1.4 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZStrimmer.h"

/**
\defgroup group_Strimmer Strimmers

The various \c ZStrimmerXX interfaces fulfill the same role for strims that \c ZStreamerXX
does for \c ZStreamXX. They are standard types representing reference counted entities
that can provide a reference to a strim of the appropriate type when their \c GetStrimXX
method is called. Although there's no physical protection against it, it's a requirement
that when a \c ZStrimXX reference is returned that reference must remain valid for
the lifetime of the ZStrimmerXX that returned it.
*/

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimmerU

const ZStrimR& ZStrimmerU::GetStrimR()
	{ return this->GetStrimU(); }
