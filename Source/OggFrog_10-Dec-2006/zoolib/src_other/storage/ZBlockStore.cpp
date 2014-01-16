static const char rcsid[] = "@(#) $Id: ZBlockStore.cpp,v 1.5 2003/01/22 01:00:11 agreen Exp $";

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

#include "ZBlockStore.h"

ZBlockStore::ZBlockStore()
	{}

ZBlockStore::~ZBlockStore()
	{}

void ZBlockStore::Flush()
	{}

ZRef<ZStreamerR> ZBlockStore::OpenR(BlockID iBlockID)
	{ return this->OpenRPos(iBlockID); }

ZRef<ZStreamerRPos> ZBlockStore::OpenRPos(BlockID iBlockID)
	{ return this->OpenRWPos(iBlockID); }

ZRef<ZStreamerW> ZBlockStore::OpenW(BlockID iBlockID)
	{ return this->OpenWPos(iBlockID); }
	
ZRef<ZStreamerWPos> ZBlockStore::OpenWPos(BlockID iBlockID)
	{ return this->OpenRWPos(iBlockID); }
