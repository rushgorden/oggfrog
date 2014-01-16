/*  @(#) $Id: ZStream_Filter.h,v 1.1 2006/10/13 22:14:51 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZStream_Filter__
#define __ZStream_Filter__ 1
#include "zconfig.h"

#include "ZStream.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_Filter

class ZStreamR_Filter : public ZStreamR
	{
public:
	ZStreamR_Filter(const ZStreamR& iStreamReal);

	~ZStreamR_Filter();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();
	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

protected:
	const ZStreamR& fStreamReal;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamU_Filter

class ZStreamU_Filter : public ZStreamU
	{
public:
	ZStreamU_Filter(const ZStreamU& iStreamReal);

	~ZStreamU_Filter();

// From ZStreamR via ZStreamU
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();
	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// from ZStreamU
	virtual void Imp_Unread();

protected:
	const ZStreamU& fStreamReal;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_Filter

class ZStreamRPos_Filter : public ZStreamRPos
	{
public:
	ZStreamRPos_Filter(const ZStreamRPos& iStreamReal);

	~ZStreamRPos_Filter();

// From ZStreamR via ZStreamRPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();
	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// From ZStreamRPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

protected:
	const ZStreamRPos& fStreamReal;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamW_Filter

class ZStreamW_Filter : public ZStreamW
	{
public:
	ZStreamW_Filter(const ZStreamW& iStreamReal);

	~ZStreamW_Filter();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

protected:
	const ZStreamW& fStreamReal;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_Filter

class ZStreamWPos_Filter : public ZStreamWPos
	{
public:
	ZStreamWPos_Filter(const ZStreamWPos& iStreamReal);

	~ZStreamWPos_Filter();

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

// From ZStreamWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 iSize);

protected:
	const ZStreamWPos& fStreamReal;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_Filter

class ZStreamRWPos_Filter : public ZStreamRWPos
	{
public:
	ZStreamRWPos_Filter(const ZStreamRWPos& iStreamReal);

	~ZStreamRWPos_Filter();

// From ZStreamR via ZStreamRWPos
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();
	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

// From ZStreamW via ZStreamRWPos
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

// From ZStreamRPos/ZStreamWPos via ZStreamRWPos
	virtual uint64 Imp_GetPosition();
	virtual void Imp_SetPosition(uint64 iPosition);

	virtual uint64 Imp_GetSize();

// From ZStreamWPos via ZStreamRWPos
	virtual void Imp_SetSize(uint64 iSize);

protected:
	const ZStreamRWPos& fStreamReal;
	};

#endif // __ZStream_Filter__
