/*  @(#) $Id: ZNet_AppleTalk.h,v 1.5 2004/05/05 15:29:25 agreen Exp $ */

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

#ifndef __ZNet_AppleTalk__
#define __ZNet_AppleTalk__ 1
#include "zconfig.h"

#include "ZNet.h"
#include "ZTypes.h"

// ==================================================
#pragma mark -
#pragma mark * ZNetAddress_AppleTalk

class ZNetAddress_AppleTalk : public ZNetAddress
	{
	friend class ZNetName_AppleTalk;
public:
	ZNetAddress_AppleTalk();
	ZNetAddress_AppleTalk(int16 iNet, uint8 iNode, int8 iSocket);
	ZNetAddress_AppleTalk(const ZNetAddress_AppleTalk& other);
	ZNetAddress_AppleTalk& operator=(const ZNetAddress_AppleTalk& other);
	virtual ~ZNetAddress_AppleTalk();

// From ZNetAddress
	virtual ZNetAddress* Clone() const;
	virtual ZRef<ZNetEndpoint> OpenRW() const;

private:
	int16 fNet;
	uint8 fNode;
	uint8 fSocket;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetName_AppleTalk

class ZNetNameRegistered_AppleTalk;

class ZNetName_AppleTalk : public ZNetName
	{
	ZNetName_AppleTalk();
public:
	ZNetName_AppleTalk(const ZNetName_AppleTalk& other);
	ZNetName_AppleTalk(const std::string& iName, const std::string& iType);
	ZNetName_AppleTalk(const std::string& iName, const std::string& iType, const std::string& iZone);
	ZNetName_AppleTalk& operator=(const ZNetName_AppleTalk& other);
	virtual ~ZNetName_AppleTalk();

// From ZNetName
	virtual ZNetName* Clone() const;
	virtual std::string AsString() const;
	virtual ZNetNameLookup* CreateLookup(size_t iMaxAddresses) const;

// Our protocol
	std::string GetType() const;
	std::string GetZone() const;

	ZNetNameRegistered_AppleTalk* RegisterSocket(uint8 iSocket) const;

private:
	std::string fName;
	std::string fType;
	std::string fZone;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetNameRegistered_AppleTalk

class ZNetNameRegistered_AppleTalk
	{
protected:
	ZNetNameRegistered_AppleTalk() {}
	ZNetNameRegistered_AppleTalk(const ZNetNameRegistered_AppleTalk& other); // Not implemented
	ZNetNameRegistered_AppleTalk& operator=(const ZNetNameRegistered_AppleTalk& other); // Not implemented

public:
	virtual ~ZNetNameRegistered_AppleTalk();

	virtual std::string GetName() const = 0;
	virtual std::string GetType() const = 0;
	virtual std::string GetZone() const = 0;
	virtual uint8 GetSocket() const = 0;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetListener_ADSP

class ZNetListener_ADSP : public ZNetListener
	{
public:
	ZNetListener_ADSP();
	virtual ~ZNetListener_ADSP();

	virtual uint8 GetSocket() = 0;

	static ZRef<ZNetListener_ADSP> sCreateListener();
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_ADSP

class ZNetEndpoint_ADSP : public ZNetEndpoint
	{
public:
	static ZRef<ZNetEndpoint_ADSP> sCreateConnectedEndpoint(int16 iNet, uint8 iNode, int8 iSocket);
	};

#endif // __ZNet_AppleTalk__
