/*  @(#) $Id: ZNet_Internet_Be.h,v 1.6 2006/04/10 20:14:51 agreen Exp $ */

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

#ifndef __ZNet_Internet_Be__
#define __ZNet_Internet_Be__ 1
#include "zconfig.h"
#include "ZCONFIG_API.h"
#include "ZCONFIG_SPI.h"

#ifndef ZCONFIG_API_Avail__Internet_BeSocket
#	define ZCONFIG_API_Avail__Internet_BeSocket ZCONFIG(OS, Be)
#endif

#ifndef ZCONFIG_API_Desired__Internet_BeSocket
#	define ZCONFIG_API_Desired__Internet_BeSocket 1
#endif

#include "ZNet_Internet.h"

#if ZCONFIG_API_Enabled(Internet_BeSocket)

// =================================================================================================
#include <net/socket.h>

// ==================================================
#pragma mark -
#pragma mark * ZNet_Internet_BeSocket

class ZNet_Internet_BeSocket
	{
public:
	static ZNet::Error sTranslateError(int inNativeError);
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_BeSocket

class ZNetNameLookup_Internet_BeSocket : public ZNetNameLookup
	{
public:
	ZNetNameLookup_Internet_BeSocket(const string& inName, ip_port inPort, size_t inMaxAddresses);
	virtual ~ZNetNameLookup_Internet_BeSocket();

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
	vector<ip_addr> fAddresses;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_BeSocket

class ZNetListener_TCP_BeSocket : public ZNetListener_TCP,
							private ZNet_Internet_BeSocket
	{
public:
	ZNetListener_TCP_BeSocket(ip_port iLocalPort, size_t iListenQueueSize);
	virtual ~ZNetListener_TCP_BeSocket();

// From ZNetListener via ZNetListener_TCP
	virtual ZRef<ZNetEndpoint> Listen();
	virtual void CancelListen();

protected:
	ZMutex fMutex;
	ZCondition fCondition;
	int fSocketFD;
	bool fListening;
	};

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_BeSocket

class ZNetEndpoint_TCP_BeSocket : private ZStreamR,
						private ZStreamW,
						public ZNetEndpoint_TCP,
						private ZNet_Internet_BeSocket
	{
public:
	ZNetEndpoint_TCP_BeSocket(int iSocketFD);
	ZNetEndpoint_TCP_BeSocket(ip_addr iRemoteHost, ip_port iRemotePort);

	virtual ~ZNetEndpoint_TCP_BeSocket();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

// From ZStreamerR via ZNetEndpoint_TCP
	virtual ZStreamR& GetStreamR();

// From ZStreamerW via ZNetEndpoint_TCP
	virtual ZStreamW& GetStreamW();

// From ZNetEndpoint via ZNetEndpoint_TCP
	virtual void SendDisconnect();
	virtual void ReceiveDisconnect();
	virtual void Disconnect();
	virtual void Abort();

	virtual ZNetAddress* GetRemoteAddress();

private:
	ZMutex fMutex;
	ZCondition fCondition;
	int fSocketFD;
	bool fOpen_Read;
	bool fOpen_Write;
	bool fReading;
	bool fWriting;
	};

// =================================================================================================

#endif // ZCONFIG_API_Enabled(Internet_BeSocket)

#endif // __ZNet_Internet_Be__
