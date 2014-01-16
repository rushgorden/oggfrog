/*  @(#) $Id: ZNet_Internet_WinSock.h,v 1.12 2006/05/06 02:28:36 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZNet_Internet_WinSock__
#define __ZNet_Internet_WinSock__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"

#ifndef ZCONFIG_API_Avail__Internet_WinSock
#	define ZCONFIG_API_Avail__Internet_WinSock ZCONFIG(OS, Win32)
#endif

#ifndef ZCONFIG_API_Desired__Internet_WinSock
#	define ZCONFIG_API_Desired__Internet_WinSock 1
#endif

#include "ZNet_Internet.h"

#if ZCONFIG_API_Enabled(Internet_WinSock)

#include <winsock.h>

// ==================================================
#pragma mark -
#pragma mark * ZNet_Internet_WinSock

class ZNet_Internet_WinSock
	{
public:
	static ZNet::Error sTranslateError(int inNativeError);
private:
	class InitHelper__;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_WinSock

class ZNetNameLookup_Internet_WinSock : public ZNetNameLookup
	{
public:
	ZNetNameLookup_Internet_WinSock(const std::string& inName, ip_port inPort, size_t inMaxAddresses);
	virtual ~ZNetNameLookup_Internet_WinSock();

// From ZNetNameLookup
	virtual void Start();
	virtual bool Finished();
	virtual void Next();
	virtual const ZNetAddress& CurrentAddress();
	virtual const ZNetName& CurrentName();

protected:
	void Internal_GrabNextResult();

	ZNetName_Internet fNetName;
	bool fStarted;
	size_t fCountAddressesToReturn;
	ZNetAddress* fCurrentAddress;
	size_t fNextIndex;
	std::vector<ip_addr> fAddresses;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_WinSock

class ZNetListener_TCP_WinSock : public ZNetListener_TCP, private ZNet_Internet_WinSock
	{
public:
	ZNetListener_TCP_WinSock(ip_port iLocalPort, size_t iListenQueueSize);
	ZNetListener_TCP_WinSock(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize);
	virtual ~ZNetListener_TCP_WinSock();

// From ZNetListener_TCP
	virtual ZRef<ZNetEndpoint> Listen();
	virtual void CancelListen();

// From ZNetListener_TCP
	virtual ip_port GetPort();

protected:
	void pInit(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize);

	ZMutex fListenMutex;
	ZMutex fAccessMutex;
	SOCKET fSOCKET;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_WinSock

class ZNetEndpoint_TCP_WinSock : protected ZStreamR,
						protected ZStreamW,
						public ZNetEndpoint_TCP,
						private ZNet_Internet_WinSock
	{
public:
	ZNetEndpoint_TCP_WinSock(SOCKET iSOCKET);
	ZNetEndpoint_TCP_WinSock(ip_addr iRemoteHost, ip_port iRemotePort);
	virtual ~ZNetEndpoint_TCP_WinSock();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

// From ZStreamerR via ZNetEndpoint_TCP
	virtual const ZStreamR& GetStreamR();

// From ZStreamerW via ZNetEndpoint_TCP
	virtual const ZStreamW& GetStreamW();

// From ZNetEndpoint via ZNetEndpoint_TCP
	virtual bool WaitTillReadable(int iMilliseconds);
	virtual void SendDisconnect();
	virtual bool ReceiveDisconnect(int iMilliseconds);
	virtual void Abort();

	virtual ZNetAddress* GetRemoteAddress();

private:
	SOCKET fSOCKET;
	};

#if 0
// ==================================================
#pragma mark -
#pragma mark * ZNetEndpointDG_UDP_WinSock

class ZNetEndpointDG_UDP_WinSock : public ZNetEndpointDG_UDP
	{
public:
	ZNetEndpointDG_UDP_WinSock();
	ZNetEndpointDG_UDP_WinSock(ip_port inLocalPort);

	virtual ZNet::Error Receive(void* inBuffer, size_t inBufferSize, size_t& outCountReceived, ZNetAddress*& outSourceAddress);
	virtual ZNet::Error Send(const void* inBuffer, size_t inCount, ZNetAddress* inDestAddress);

protected:
	void InternalAllocateSocket(ip_port inLocalPort);

private:
	SOCKET fSOCKET;
	};
#endif

#endif // ZCONFIG_API_Enabled(Internet_WinSock)

#endif // __ZNet_Internet_WinSock__
