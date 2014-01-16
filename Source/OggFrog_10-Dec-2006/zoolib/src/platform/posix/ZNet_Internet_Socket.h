/*  @(#) $Id: ZNet_Internet_Socket.h,v 1.18 2006/07/17 18:58:30 agreen Exp $ */

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

#ifndef __ZNet_Internet_Socket__
#define __ZNet_Internet_Socket__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"
#include "ZCONFIG_SPI.h"

#ifndef ZCONFIG_API_Avail__Internet_Socket
#	define ZCONFIG_API_Avail__Internet_Socket (ZCONFIG(OS, POSIX) || ZCONFIG(OS, MacOSX))
#endif

#ifndef ZCONFIG_API_Desired__Internet_Socket
#	define ZCONFIG_API_Desired__Internet_Socket 1
#endif

#include "ZNet_Internet.h"

#if ZCONFIG_API_Enabled(Internet_Socket)

// =================================================================================================

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// ==================================================
#pragma mark -
#pragma mark * ZNet_Internet_Socket

class ZNet_Internet_Socket
	{
public:
	static ZNet::Error sTranslateError(int iNativeError);
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_Socket

class ZNetNameLookup_Internet_Socket : public ZNetNameLookup
	{
public:
	ZNetNameLookup_Internet_Socket(const std::string& iName, ip_port iPort, size_t iMaxAddresses);
	virtual ~ZNetNameLookup_Internet_Socket();

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
#pragma mark * ZNetListener_TCP_Socket

class ZNetListener_TCP_Socket : public ZNetListener_TCP,
							private ZNet_Internet_Socket
	{
public:
	ZNetListener_TCP_Socket(ip_port iLocalPort, size_t iListenQueueSize);
	ZNetListener_TCP_Socket(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize);
	virtual ~ZNetListener_TCP_Socket();

// From ZNetListener via ZNetListener_TCP
	virtual ZRef<ZNetEndpoint> Listen();
	virtual void CancelListen();

// From ZNetListener_TCP
	virtual ip_port GetPort();

protected:
	void pInit(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize);

	ZMutexNR fMutexNR;
	ZCondition fCondition;
	int fSocketFD;
	bool fListening;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_Socket

class ZNetEndpoint_TCP_Socket : private ZStreamR,
						private ZStreamW,
						public ZNetEndpoint_TCP,
						private ZNet_Internet_Socket
	{
public:
	ZNetEndpoint_TCP_Socket(int iSocketFD);
	ZNetEndpoint_TCP_Socket(ip_addr iRemoteHost, ip_port iRemotePort);

	virtual ~ZNetEndpoint_TCP_Socket();

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
	int fSocketFD;
	};

// =================================================================================================

#endif // ZCONFIG_API_Enabled(Internet_Socket)

#endif // __ZNet_Internet_Socket__
