/*  @(#) $Id: ZStreamer.h,v 1.17 2006/04/27 03:19:32 agreen Exp $ */

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

#ifndef __ZStreamer__
#define __ZStreamer__ 1
#include "zconfig.h"

#include "ZRefCount.h"
#include "ZStream.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerR

/** A refcounted entity that provides access to a ZStreamR.
\ingroup group_Streamer
*/

class ZStreamerR : public virtual ZRefCountedWithFinalization
	{
public:
	virtual const ZStreamR& GetStreamR() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerU

/** A refcounted entity that provides access to a ZStreamU.
\ingroup group_Streamer
*/

class ZStreamerU : public ZStreamerR
	{
public:
// From ZStreamerR
	virtual const ZStreamR& GetStreamR();

// Our protocol
	virtual const ZStreamU& GetStreamU() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos

/** A refcounted entity that provides access to a ZStreamRPos.
\ingroup group_Streamer
*/

class ZStreamerRPos : public ZStreamerU
	{
public:
// From ZStreamerR
	virtual const ZStreamR& GetStreamR();

// From ZStreamerU
	virtual const ZStreamU& GetStreamU();

// Our protocol
	virtual const ZStreamRPos& GetStreamRPos() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerW

/** A refcounted entity that provides access to a ZStreamW.
\ingroup group_Streamer
*/

class ZStreamerW : public virtual ZRefCountedWithFinalization
	{
public:
	virtual const ZStreamW& GetStreamW() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos

/** A refcounted entity that provides access to a ZStreamWPos.
\ingroup group_Streamer
*/

class ZStreamerWPos : public ZStreamerW
	{
public:
// From ZStreamerW
	virtual const ZStreamW& GetStreamW();

// Our protocol
	virtual const ZStreamWPos& GetStreamWPos() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos

/** A refcounted entity that provides access to a ZStreamRWPos.
\ingroup group_Streamer
*/

class ZStreamerRWPos : public ZStreamerRPos, public ZStreamerWPos
	{
public:
// From ZStreamerR
	virtual const ZStreamR& GetStreamR();

// From ZStreamerU
	virtual const ZStreamU& GetStreamU();

// From ZStreamerRPos
	virtual const ZStreamRPos& GetStreamRPos();

// From ZStreamerW
	virtual const ZStreamW& GetStreamW();

// From ZStreamerWPos
	virtual const ZStreamWPos& GetStreamWPos();

// Our protocol
	virtual const ZStreamRWPos& GetStreamRWPos() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRW

/** A refcounted entity that implements both ZStreamerR and ZStreamerW interfaces, and
thus provides access to a ZStreamR and a ZStreamW.
\ingroup group_Streamer
*/

class ZStreamerRW : public ZStreamerR, public ZStreamerW
	{
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerR_T

template <class Stream_t>
class ZStreamerR_T : public ZStreamerR
	{
public:
	ZStreamerR_T() {}

	virtual ~ZStreamerR_T() {}

	template <class P>
	ZStreamerR_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerR_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerR_FT

template <class Stream_t>
class ZStreamerR_FT : public ZStreamerR
	{
protected:
	ZStreamerR_FT() {}

public:
	virtual ~ZStreamerR_FT() {}

	template <class P>
	ZStreamerR_FT(P& iParam, ZRef<ZStreamerR> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamR())
		{}

	template <class P>
	ZStreamerR_FT(const P& iParam, ZRef<ZStreamerR> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamR())
		{}

	ZStreamerR_FT(ZRef<ZStreamerR> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamR())
		{}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerR> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerU_T

template <class Stream_t>
class ZStreamerU_T : public ZStreamerU
	{
public:
	ZStreamerU_T() {}

	virtual ~ZStreamerU_T() {}

	template <class P>
	ZStreamerU_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerU_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU
	virtual const ZStreamU& GetStreamU() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerU_FT

template <class Stream_t>
class ZStreamerU_FT : public ZStreamerU
	{
protected:
	ZStreamerU_FT() {}

public:
	virtual ~ZStreamerU_FT() {}

	template <class P>
	ZStreamerU_FT(P& iParam, ZRef<ZStreamerU> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamU())
		{}

	template <class P>
	ZStreamerU_FT(const P& iParam, ZRef<ZStreamerU> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamU())
		{}

	ZStreamerU_FT(ZRef<ZStreamerU> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamU())
		{}

// From ZStreamerR via ZStreamerU
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU
	virtual const ZStreamU& GetStreamU() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerU> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_T

template <class Stream_t>
class ZStreamerRPos_T : public ZStreamerRPos
	{
public:
	ZStreamerRPos_T() {}

	virtual ~ZStreamerRPos_T() {}

	template <class P>
	ZStreamerRPos_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerRPos_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU
	virtual const ZStreamU& GetStreamU() { return fStream; }

// From ZStreamerRPos
	virtual const ZStreamRPos& GetStreamRPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_FT

template <class Stream_t>
class ZStreamerRPos_FT : public ZStreamerU
	{
protected:
	ZStreamerRPos_FT() {}

public:
	virtual ~ZStreamerRPos_FT() {}

	template <class P>
	ZStreamerRPos_FT(P& iParam, ZRef<ZStreamerRPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamRPos())
		{}

	template <class P>
	ZStreamerRPos_FT(const P& iParam, ZRef<ZStreamerRPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamRPos())
		{}

	ZStreamerRPos_FT(ZRef<ZStreamerRPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamRPos())
		{}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU
	virtual const ZStreamU& GetStreamU() { return fStream; }

// From ZStreamerRPos
	virtual const ZStreamRPos& GetStreamRPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerRPos> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerW_T

template <class Stream_t>
class ZStreamerW_T : public ZStreamerW
	{
public:
	ZStreamerW_T() {}

	virtual ~ZStreamerW_T() {}

	template <class P>
	ZStreamerW_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerW_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerW
	virtual const ZStreamW& GetStreamW() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerW_FT

template <class Stream_t>
class ZStreamerW_FT : public ZStreamerW
	{
protected:
	ZStreamerW_FT() {}

public:
	virtual ~ZStreamerW_FT() {}

	template <class P>
	ZStreamerW_FT(P& iParam, ZRef<ZStreamerW> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamW())
		{}

	template <class P>
	ZStreamerW_FT(const P& iParam, ZRef<ZStreamerW> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamW())
		{}

	ZStreamerW_FT(ZRef<ZStreamerW> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamW())
		{}

// From ZStreamerW
	virtual const ZStreamW& GetStreamW() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerW> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos_T

template <class Stream_t>
class ZStreamerWPos_T : public ZStreamerWPos
	{
public:
	ZStreamerWPos_T() {}

	virtual ~ZStreamerWPos_T() {}

	template <class P>
	ZStreamerWPos_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerWPos_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerW via ZStreamerWPos
	virtual const ZStreamW& GetStreamW() { return fStream; }

// From ZStreamerWPos
	virtual const ZStreamWPos& GetStreamWPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerWPos_FT

template <class Stream_t>
class ZStreamerWPos_FT : public ZStreamerWPos
	{
protected:
	ZStreamerWPos_FT() {}

public:
	virtual ~ZStreamerWPos_FT() {}

	template <class P>
	ZStreamerWPos_FT(P& iParam, ZRef<ZStreamerWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamWPos())
		{}

	template <class P>
	ZStreamerWPos_FT(const P& iParam, ZRef<ZStreamerWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamWPos())
		{}

	ZStreamerWPos_FT(ZRef<ZStreamerWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamWPos())
		{}

// From ZStreamerW via ZStreamerWPos
	virtual const ZStreamW& GetStreamW() { return fStream; }

// From ZStreamerWPos
	virtual const ZStreamWPos& GetStreamWPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerWPos> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos_T

template <class Stream_t>
class ZStreamerRWPos_T : public ZStreamerRWPos
	{
public:
	ZStreamerRWPos_T() {}

	virtual ~ZStreamerRWPos_T() {}

	template <class P>
	ZStreamerRWPos_T(P& iParam) : fStream(iParam) {}

	template <class P>
	ZStreamerRWPos_T(const P& iParam) : fStream(iParam) {}

// From ZStreamerR via ZStreamerRWPos
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU via ZStreamerRWPos
	virtual const ZStreamU& GetStreamU() { return fStream; }

// From ZStreamerRPos via ZStreamerRWPos
	virtual const ZStreamRPos& GetStreamRPos() { return fStream; }

// From ZStreamerW via ZStreamerRWPos
	virtual const ZStreamW& GetStreamW() { return fStream; }

// From ZStreamerWPos via ZStreamerRWPos
	virtual const ZStreamWPos& GetStreamWPos() { return fStream; }

// From ZStreamerRWPos
	virtual const ZStreamRWPos& GetStreamRWPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRWPos_FT

template <class Stream_t>
class ZStreamerRWPos_FT : public ZStreamerRWPos
	{
protected:
	ZStreamerRWPos_FT() {}

public:
	virtual ~ZStreamerRWPos_FT() {}

	template <class P>
	ZStreamerRWPos_FT(P& iParam, ZRef<ZStreamerRWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamRWPos())
		{}

	template <class P>
	ZStreamerRWPos_FT(const P& iParam, ZRef<ZStreamerRWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iParam, iStreamer->GetStreamRWPos())
		{}

	ZStreamerRWPos_FT(ZRef<ZStreamerRWPos> iStreamer)
	:	fStreamerReal(iStreamer),
		fStream(iStreamer->GetStreamRWPos())
		{}

// From ZStreamerR via ZStreamerRWPos
	virtual const ZStreamR& GetStreamR() { return fStream; }

// From ZStreamerU via ZStreamerRWPos
	virtual const ZStreamU& GetStreamU() { return fStream; }

// From ZStreamerRPos via ZStreamerRWPos
	virtual const ZStreamRPos& GetStreamRPos() { return fStream; }

// From ZStreamerW via ZStreamerRWPos
	virtual const ZStreamW& GetStreamW() { return fStream; }

// From ZStreamerWPos via ZStreamerRWPos
	virtual const ZStreamWPos& GetStreamWPos() { return fStream; }

// From ZStreamerRWPos
	virtual const ZStreamRWPos& GetStreamRWPos() { return fStream; }

// Our protocol
	Stream_t& GetStream() { return fStream; }

protected:
	ZRef<ZStreamerRWPos> fStreamerReal;
	Stream_t fStream;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_Streamer

class ZStreamR_Streamer : public ZStreamR
	{
public:
	ZStreamR_Streamer(ZRef<ZStreamerR> iStreamer);
	~ZStreamR_Streamer();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

	virtual void Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

private:
	ZRef<ZStreamerR> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamU_Streamer

class ZStreamU_Streamer : public ZStreamU
	{
public:
	ZStreamU_Streamer(ZRef<ZStreamerU> iStreamer);
	~ZStreamU_Streamer();

// From ZStreamR via ZStreamU
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

	virtual void Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// From ZStreamU
	virtual void Imp_Unread();

private:
	ZRef<ZStreamerU> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_Streamer

class ZStreamRPos_Streamer : public ZStreamRPos
	{
public:
	ZStreamRPos_Streamer(ZRef<ZStreamerRPos> iStreamer);
	~ZStreamRPos_Streamer();

// From ZStreamR via ZStreamRPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

	virtual void Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// From ZStreamRPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

private:
	ZRef<ZStreamerRPos> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamW_Streamer

class ZStreamW_Streamer : public ZStreamW
	{
public:
	ZStreamW_Streamer(ZRef<ZStreamerW> iStreamer);
	~ZStreamW_Streamer();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

	virtual void Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Flush();

private:
	ZRef<ZStreamerW> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_Streamer

class ZStreamWPos_Streamer : public ZStreamWPos
	{
public:
	ZStreamWPos_Streamer(ZRef<ZStreamerWPos> iStreamer);
	~ZStreamWPos_Streamer();

// From ZStreamW via ZStreamWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

	virtual void Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Flush();

// From ZStreamWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();
	virtual void Imp_SetSize(uint64 iSize);

private:
	ZRef<ZStreamerWPos> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_Streamer

class ZStreamRWPos_Streamer : public ZStreamRWPos
	{
public:
	ZStreamRWPos_Streamer(ZRef<ZStreamerRWPos> iStreamer);
	~ZStreamRWPos_Streamer();

// From ZStreamR via ZStreamRWPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);

	virtual void Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

	virtual void Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

// From ZStreamRPos/ZStreamWPos via ZStreamRWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 iSize);

// Our protocol
	ZRef<ZStreamerRWPos> GetStreamer()
		{ return fStreamer; }

	void SetStreamer(ZRef<ZStreamerRWPos> iStreamer)
		{ fStreamer = iStreamer; }
private:
	ZRef<ZStreamerRWPos> fStreamer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRW_Wrapper

/// A RW streamer that wraps a ZStreamerR and a ZStreamerW into a single entity.

class ZStreamerRW_Wrapper : public ZStreamerRW
	{
public:
	ZStreamerRW_Wrapper(ZRef<ZStreamerR> iStreamerR, ZRef<ZStreamerW> iStreamerW);
	virtual ~ZStreamerRW_Wrapper();

// From ZStreamerR via ZStreamerRW
	virtual const ZStreamR& GetStreamR();

// From ZStreamerW via ZStreamerRW
	virtual const ZStreamW& GetStreamW();

protected:
	ZRef<ZStreamerR> fStreamerR;
	const ZStreamR& fStreamR;
	ZRef<ZStreamerW> fStreamerW;
	const ZStreamW& fStreamW;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerR_Stream

/** \brief An R streamer that wraps a ZStreamR.

Note that the ZStreamR's lifetime must extend beyond that of the ZStreamerR.
*/

class ZStreamerR_Stream : public ZStreamerR
	{
public:
	ZStreamerR_Stream(const ZStreamR& iStreamR)
	:	fStreamR(iStreamR)
		{}

// From ZStreamerR
	virtual const ZStreamR& GetStreamR() { return fStreamR; }

protected:
	const ZStreamR& fStreamR;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerW_Stream

/** \brief A W streamer that wraps a ZStreamW.

Note that the ZStreamW's lifetime must extend beyond that of the ZStreamerW.
*/

class ZStreamerW_Stream : public ZStreamerW
	{
public:
	ZStreamerW_Stream(const ZStreamW& iStreamW)
	:	fStreamW(iStreamW)
		{}

// From ZStreamerR
	virtual const ZStreamW& GetStreamW() { return fStreamW; }

protected:
	const ZStreamW& fStreamW;
	};

#endif // __ZStreamer__
