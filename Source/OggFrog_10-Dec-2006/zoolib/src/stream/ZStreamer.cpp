static const char rcsid[] = "@(#) $Id: ZStreamer.cpp,v 1.15 2006/04/27 03:19:32 agreen Exp $";

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

#include "ZStreamer.h"

/**
\defgroup group_Streamer Streamers

The various \c ZStreamXX classes are just interfaces. They make it easy to wrap a
standard lightweight API around arbitrary data sources, sinks and filters, and
pass them to library and application code. They deliberately do not address lifetime issues.
The various \c ZStreamerXX classes together define a standard mechanism for encapsulating
real streams inside refcounted containers, making it possible to manage the lifetime
of a stream in a safe fashion.

The defining aspects of a \c ZStreamerXX are the fact that it is refcounted, and has the
\c GetStreamXX virtual methods a concrete instance is required to implement. Although there's
no physical protection against it, it's a requirement that when a \c ZStreamXX reference is
returned that reference must remain valid for the lifetime of the \c ZStreamerXX that returned it.

Many of the standard \c ZStreamXX classes have matching \c ZStreamerXX classes, but
this is not mandatory for each \c ZStreamXX derivative that you define. If you do it will
make it possible for that stream to particpate in streamer operations. To make it a little
easier to wrap arbitrary streams inside a streamer there are template definitions. For
\c ZStreamerX the templates are called \c ZStreamerX_T and \c ZStreamerX_FT.

\c ZStreamerX_T takes a template parameter which is your stream's class name, and has
two constructors. One takes no parameters, the other takes a single parameter which is
passed on to your stream's constructor.

\c ZStreamerX_FT is the template for a filter stream. Its default constructor is
protected and thus inaccessible to code, because a filter stream makes no sense without
a sink/source stream. Of the two accessible constructors one takes a \c ZRef<ZStreamerXX>
and a single parameter, the other takes only a \c ZRef<ZStreamerXX>. Your filter stream
will be constructed with either the parameter and the result of \c ZStreamerXX::GetStreamXX,
or with just the result of \c ZStreamerXX::GetStreamXX.

You can see that if you want a \c ZStreamXX that can be wrapped in a \c ZStreamerXX_T or a
\c ZStreamerXX_FT, and your stream requires parameters to initialize it then it must have
a constructor that takes a single parameter, or a single parameter and a sink/source stream.
Although you can use <code>pair<Type1, Type2></code> or define a struct to carry the
initialization payload if you need more than a single parameter, it can often be easier
to manually write a \c ZStreamerXX derivative.

The \c ZStreamerXX classes have a hierarchy that parallels the \c ZStreamXX hierarchy:

\verbatim
  ZStreamerR
  +--ZStreamerU
  +----ZStreamerRPos
  +------ZStreamerRWPos
  +--ZStreamerRW

  ZStreamerW
  +--ZStreamerWPos
  +----ZStreamerRWPos
  +--ZStreamerRW
\endverbatim

You'll notice that ZStreamerRWPos multiply inherits from both ZStreamerRPos and ZStreamerWPos,
and ZStreamerRW from both ZStreamerR and ZStreamerW. As ZStreamerR and ZStreamerW each
descend from ZRefCounted, and we require a single ZRefCounted subobject in any ZRefCounted
descendant, ZStreamerR and ZStreamerW have ZRefCounted as a virtual base class. The
first and only time in 17 years of C++ programming where I've needed to do this.
*/

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerU

const ZStreamR& ZStreamerU::GetStreamR()
	{ return this->GetStreamU(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos

const ZStreamR& ZStreamerRPos::GetStreamR()
	{ return this->GetStreamRPos(); }

const ZStreamU& ZStreamerRPos::GetStreamU()
	{ return this->GetStreamRPos(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos

const ZStreamW& ZStreamerWPos::GetStreamW()
	{ return this->GetStreamWPos(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos

const ZStreamR& ZStreamerRWPos::GetStreamR()
	{ return this->GetStreamRWPos(); }

const ZStreamU& ZStreamerRWPos::GetStreamU()
	{ return this->GetStreamRWPos(); }

const ZStreamRPos& ZStreamerRWPos::GetStreamRPos()
	{ return this->GetStreamRWPos(); }

const ZStreamW& ZStreamerRWPos::GetStreamW()
	{ return this->GetStreamRWPos(); }

const ZStreamWPos& ZStreamerRWPos::GetStreamWPos()
	{ return this->GetStreamRWPos(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_Streamer

ZStreamR_Streamer::ZStreamR_Streamer(ZRef<ZStreamerR> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamR_Streamer::~ZStreamR_Streamer()
	{}

void ZStreamR_Streamer::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamR().Read(iDest, iCount, oCountRead);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		}
	}

void ZStreamR_Streamer::Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamR().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamR_Streamer::Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamR().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamR_Streamer::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamR().Skip(iCount, oCountSkipped);
		}
	else
		{
		if (oCountSkipped)
			*oCountSkipped = 0;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamU_Streamer

ZStreamU_Streamer::ZStreamU_Streamer(ZRef<ZStreamerU> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamU_Streamer::~ZStreamU_Streamer()
	{}

void ZStreamU_Streamer::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamU().Read(iDest, iCount, oCountRead);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		}
	}

void ZStreamU_Streamer::Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamU().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamU_Streamer::Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamU().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamU_Streamer::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamU().Skip(iCount, oCountSkipped);
		}
	else
		{
		if (oCountSkipped)
			*oCountSkipped = 0;
		}
	}

void ZStreamU_Streamer::Imp_Unread()
	{
	if (fStreamer)
		return fStreamer->GetStreamU().Unread();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_Streamer

ZStreamRPos_Streamer::ZStreamRPos_Streamer(ZRef<ZStreamerRPos> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamRPos_Streamer::~ZStreamRPos_Streamer()
	{}

void ZStreamRPos_Streamer::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRPos().Read(iDest, iCount, oCountRead);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		}
	}

void ZStreamRPos_Streamer::Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRPos().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRPos_Streamer::Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRPos().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRPos_Streamer::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRPos().Skip(iCount, oCountSkipped);
		}
	else
		{
		if (oCountSkipped)
			*oCountSkipped = 0;
		}
	}

uint64 ZStreamRPos_Streamer::Imp_GetPosition()
	{
	if (fStreamer)
		return fStreamer->GetStreamRPos().GetPosition();
	return 0;
	}

void ZStreamRPos_Streamer::Imp_SetPosition(uint64 iPosition)
	{
	if (fStreamer)
		return fStreamer->GetStreamRPos().SetPosition(iPosition);
	}

uint64 ZStreamRPos_Streamer::Imp_GetSize()
	{
	if (fStreamer)
		return fStreamer->GetStreamRPos().GetSize();
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamW_Streamer

ZStreamW_Streamer::ZStreamW_Streamer(ZRef<ZStreamerW> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamW_Streamer::~ZStreamW_Streamer()
	{}

void ZStreamW_Streamer::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamW().Write(iSource, iCount, oCountWritten);
		}
	else
		{
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamW_Streamer::Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamW().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamW_Streamer::Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamW().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamW_Streamer::Imp_Flush()
	{
	if (fStreamer)
		fStreamer->GetStreamW().Flush();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_Streamer

ZStreamWPos_Streamer::ZStreamWPos_Streamer(ZRef<ZStreamerWPos> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamWPos_Streamer::~ZStreamWPos_Streamer()
	{}

void ZStreamWPos_Streamer::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamWPos().Write(iSource, iCount, oCountWritten);
		}
	else
		{
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamWPos_Streamer::Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamWPos().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamWPos_Streamer::Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamWPos().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamWPos_Streamer::Imp_Flush()
	{
	if (fStreamer)
		fStreamer->GetStreamWPos().Flush();
	}

uint64 ZStreamWPos_Streamer::Imp_GetPosition()
	{
	if (fStreamer)
		return fStreamer->GetStreamWPos().GetPosition();
	return 0;
	}

void ZStreamWPos_Streamer::Imp_SetPosition(uint64 iPosition)
	{
	if (fStreamer)
		fStreamer->GetStreamWPos().SetPosition(iPosition);
	}

uint64 ZStreamWPos_Streamer::Imp_GetSize()
	{
	if (fStreamer)
		return fStreamer->GetStreamWPos().GetSize();
	return 0;
	}

void ZStreamWPos_Streamer::Imp_SetSize(uint64 iSize)
	{
	if (fStreamer)
		fStreamer->GetStreamWPos().SetSize(iSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_Streamer

ZStreamRWPos_Streamer::ZStreamRWPos_Streamer(ZRef<ZStreamerRWPos> iStreamer)
:	fStreamer(iStreamer)
	{}

ZStreamRWPos_Streamer::~ZStreamRWPos_Streamer()
	{}

void ZStreamRWPos_Streamer::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().Read(iDest, iCount, oCountRead);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().CopyTo(iStreamW, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().Skip(iCount, oCountSkipped);
		}
	else
		{
		if (oCountSkipped)
			*oCountSkipped = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().Write(iSource, iCount, oCountWritten);
		}
	else
		{
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

void ZStreamRWPos_Streamer::Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{
	if (fStreamer)
		{
		fStreamer->GetStreamRWPos().CopyFrom(iStreamR, iCount, oCountRead, oCountWritten);
		}
	else
		{
		if (oCountRead)
			*oCountRead = 0;
		if (oCountWritten)
			*oCountWritten = 0;
		}
	}

uint64 ZStreamRWPos_Streamer::Imp_GetPosition()
	{
	if (fStreamer)
		return fStreamer->GetStreamRWPos().GetPosition();
	return 0;
	}

void ZStreamRWPos_Streamer::Imp_SetPosition(uint64 iPosition)
	{
	if (fStreamer)
		fStreamer->GetStreamRWPos().SetPosition(iPosition);
	}

uint64 ZStreamRWPos_Streamer::Imp_GetSize()
	{
	if (fStreamer)
		return fStreamer->GetStreamRWPos().GetSize();
	return 0;
	}

void ZStreamRWPos_Streamer::Imp_SetSize(uint64 iSize)
	{
	if (fStreamer)
		fStreamer->GetStreamRWPos().SetSize(iSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRW_Wrapper

ZStreamerRW_Wrapper::ZStreamerRW_Wrapper(ZRef<ZStreamerR> iStreamerR, ZRef<ZStreamerW> iStreamerW)
:	fStreamerR(iStreamerR),
	fStreamR(iStreamerR->GetStreamR()),
	fStreamerW(iStreamerW),
	fStreamW(iStreamerW->GetStreamW())
	{}

ZStreamerRW_Wrapper::~ZStreamerRW_Wrapper()
	{}

const ZStreamR& ZStreamerRW_Wrapper::GetStreamR()
	{ return fStreamR; }

const ZStreamW& ZStreamerRW_Wrapper::GetStreamW()
	{ return fStreamW; }
