/*  @(#) $Id: ZTBRep_Client_Net.h,v 1.3 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZTBRep_Client_Net__
#define __ZTBRep_Client_Net__ 1

#include "ZTBRep_Client.h"
#include "ZNet.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client_Net

class ZTBRep_Client_Net : public ZTBRep_Client
	{
public:
	ZTBRep_Client_Net(ZNetAddress* iAddress);
	virtual ~ZTBRep_Client_Net();

// From ZTBRep_Client
	virtual ZRef<ZStreamerRW> EstablishConnection();

protected:
	ZNetAddress* fAddress;
	};

#endif // __ZTBRep_Client_Net__
