static const char rcsid[] = "@(#) $Id: ZNet_AppleTalk.cpp,v 1.5 2004/05/05 15:29:25 agreen Exp $";

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

#include "ZNet_AppleTalk.h"
#include "ZNet_AppleTalk_MacClassic.h"

using std::string;

// ==================================================
#pragma mark -
#pragma mark * ZNetAddress_AppleTalk

ZNetAddress_AppleTalk::ZNetAddress_AppleTalk()
:	fNet(0),
	fNode(0),
	fSocket(0)
	{}

ZNetAddress_AppleTalk::ZNetAddress_AppleTalk(int16 iNet, uint8 iNode, int8 iSocket)
:	fNet(iNet),
	fNode(iNode),
	fSocket(iSocket)
	{}

ZNetAddress_AppleTalk::ZNetAddress_AppleTalk(const ZNetAddress_AppleTalk& other)
:	fNet(other.fNet),
	fNode(other.fNode),
	fSocket(other.fSocket)
	{}

ZNetAddress_AppleTalk& ZNetAddress_AppleTalk::operator=(const ZNetAddress_AppleTalk& other)
	{
	fNet = other.fNet;
	fNode = other.fNode;
	fSocket = other.fSocket;
	return *this;
	}

ZNetAddress_AppleTalk::~ZNetAddress_AppleTalk()
	{}

ZNetAddress* ZNetAddress_AppleTalk::Clone() const
	{ return new ZNetAddress_AppleTalk(*this); }

ZRef<ZNetEndpoint> ZNetAddress_AppleTalk::OpenRW() const
	{ return ZNetEndpoint_ADSP::sCreateConnectedEndpoint(fNet, fNode, fSocket); }

// ==================================================
#pragma mark -
#pragma mark * ZNetName_AppleTalk

ZNetName_AppleTalk::ZNetName_AppleTalk()
	{}

ZNetName_AppleTalk::ZNetName_AppleTalk(const ZNetName_AppleTalk& other)
:	fName(other.fName),
	fType(other.fType),
	fZone(other.fZone)
	{}

ZNetName_AppleTalk::ZNetName_AppleTalk(const string& iName, const string& iType)
:	fName(iName),
	fType(iType)
	{}

ZNetName_AppleTalk::ZNetName_AppleTalk(const string& iName, const string& iType, const string& iZone)
:	fName(iName),
	fType(iType),
	fZone(iZone)
	{}

ZNetName_AppleTalk& ZNetName_AppleTalk::operator=(const ZNetName_AppleTalk& other)
	{
	fName = other.fName;
	fType = other.fType;
	fZone = other.fZone;
	return *this;
	}

ZNetName_AppleTalk::~ZNetName_AppleTalk()
	{}

ZNetName* ZNetName_AppleTalk::Clone() const
	{ return new ZNetName_AppleTalk(*this); }

string ZNetName_AppleTalk::AsString() const
	{ return fName; }

ZNetNameLookup* ZNetName_AppleTalk::CreateLookup(size_t iMaxAddresses) const
	{
#if ZCONFIG(API_Net, MacClassic)
	return new ZNetNameLookup_AppleTalk_MacClassic(fName, fType, fZone, iMaxAddresses);
#endif // ZCONFIG(API_Net, MacClassic)
	return nil;
	}

string ZNetName_AppleTalk::GetType() const
	{ return fType; }

string ZNetName_AppleTalk::GetZone() const
	{ return fZone; }

ZNetNameRegistered_AppleTalk* ZNetName_AppleTalk::RegisterSocket(uint8 iSocket) const
	{
#if ZCONFIG(API_Net, MacClassic)
	ZNetNameRegistered_AppleTalk_MacClassic* theNN = new ZNetNameRegistered_AppleTalk_MacClassic(fName, fType, fZone, iSocket);
	if (theNN->IsRegistrationGood())
		return theNN;
	delete theNN;
#endif // ZCONFIG(API_Net, MacClassic)
	return nil;
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetNameRegistered_AppleTalk

ZNetNameRegistered_AppleTalk::~ZNetNameRegistered_AppleTalk()
	{}

// ==================================================
#pragma mark -
#pragma mark * ZNetListener_ADSP

ZNetListener_ADSP::ZNetListener_ADSP()
	{}

ZNetListener_ADSP::~ZNetListener_ADSP()
	{}

ZRef<ZNetListener_ADSP> ZNetListener_ADSP::sCreateListener()
	{
#if ZCONFIG(API_Net, MacClassic)
	return new ZNetListener_ADSP_MacClassic;
#endif // ZCONFIG(API_Net, MacClassic)

	return ZRef<ZNetListener_ADSP>();
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_ADSP

ZRef<ZNetEndpoint_ADSP> ZNetEndpoint_ADSP::sCreateConnectedEndpoint(int16 iNet, uint8 iNode, int8 iSocket)
	{
	try
		{
#if ZCONFIG(API_Net, MacClassic)
		return new ZNetEndpoint_ADSP_MacClassic(iNet, iNode, iSocket);
#endif // ZCONFIG(API_Net, MacClassic)
		}
	catch (...)
		{}
	return ZRef<ZNetEndpoint_ADSP>();
	}
