static const char rcsid[] = "@(#) $Id: ZNet_Internet_WinSock.cpp,v 1.16 2006/05/06 02:28:36 agreen Exp $";

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

#include "ZNet_Internet_WinSock.h"

#if ZCONFIG_API_Enabled(Internet_WinSock)

#include "ZMemory.h"
#include "ZTime.h"

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZNet_Internet_WinSock

ZNet::Error ZNet_Internet_WinSock::sTranslateError(int inNativeError)
	{
	// Because WSAGetLastError is not thread safe, we're not guaranteed
	// to get passed the appropriate value. In particular, we may get
	// given a zero error. Hence, we just bail for now and return
	// ZNet::errorGeneric in all cases (we won't get called unless
	// an error was signalled in some other fashion).
	ZNet::Error theError = ZNet::errorGeneric;
/*	switch (nativeError)
		{
		case 0: theError = ZNet::errorNone; break;
		default: break;
		}*/
	return theError;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNet_Internet_WinSock::InitHelper__

class ZNet_Internet_WinSock::InitHelper__
	{
public:
	InitHelper__();
	~InitHelper__();
private:
	WSADATA fWSADATA;
	static InitHelper__ sInitHelper__;
	};

ZNet_Internet_WinSock::InitHelper__ ZNet_Internet_WinSock::InitHelper__::sInitHelper__;

ZNet_Internet_WinSock::InitHelper__::InitHelper__()
	{
	// Low order byte is major version number, high order byte is minor version number.
	int result = ::WSAStartup(0x0002, &fWSADATA);
	if (result)
		ZDebugLogf(2, ("WSAStartup got error %d", GetLastError()));
	}

ZNet_Internet_WinSock::InitHelper__::~InitHelper__()
	{ ::WSACleanup(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_WinSock

ZNetNameLookup_Internet_WinSock::ZNetNameLookup_Internet_WinSock(const string& inName, ip_port inPort, size_t inMaxAddresses)
:	fNetName(inName, inPort)
	{
	fStarted = false;
	fCurrentAddress = nil;
	fNextIndex = 0;
	fCountAddressesToReturn = inMaxAddresses;

	if (0 == inName.size())
		{
		fStarted = true;
		fCountAddressesToReturn = 0;
		}
	else
		{
		ip_addr theIPAddr = ::inet_addr(inName.c_str());
		if (theIPAddr != INADDR_NONE)
			{
			fStarted = true;
			fAddresses.push_back(ntohl(theIPAddr));
			}
		}
	}

ZNetNameLookup_Internet_WinSock::~ZNetNameLookup_Internet_WinSock()
	{
	delete fCurrentAddress;
	}

void ZNetNameLookup_Internet_WinSock::Start()
	{
	if (fStarted)
		return;

	fStarted = true;

	string tempString = fNetName.GetName();
	const char* tempCStr = tempString.c_str();
	if (hostent* theHostEnt = ::gethostbyname(tempCStr))
		{
		char** listPtr = theHostEnt->h_addr_list;
		while (*listPtr && fCountAddressesToReturn)
			{
			in_addr* in_addrPtr = reinterpret_cast<in_addr*>(*listPtr);
			fAddresses.push_back(ntohl(in_addrPtr->s_addr));
			++listPtr;
			--fCountAddressesToReturn;
			}
		}
	else
		{
		int err = ::WSAGetLastError();
		ZDebugLogf(2, ("%d", err));
		}
	}

bool ZNetNameLookup_Internet_WinSock::Finished()
	{
	if (!fStarted)
		return true;

	return fNextIndex >= fAddresses.size();
	}

void ZNetNameLookup_Internet_WinSock::Next()
	{
	ZAssertStop(2, fStarted);

	this->Internal_GrabNextResult();
	}

const ZNetAddress& ZNetNameLookup_Internet_WinSock::CurrentAddress()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return *fCurrentAddress;
	}

const ZNetName& ZNetNameLookup_Internet_WinSock::CurrentName()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return fNetName;
	}

void ZNetNameLookup_Internet_WinSock::Internal_GrabNextResult()
	{
	ZAssertStop(2, fAddresses.size() && fNextIndex <= fAddresses.size());

	delete fCurrentAddress;
	fCurrentAddress = nil;
	if (fNextIndex < fAddresses.size())
		{
		fCurrentAddress = new ZNetAddress_Internet(fAddresses[fNextIndex], fNetName.GetPort());
		++fNextIndex;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_WinSock

ZNetListener_TCP_WinSock::ZNetListener_TCP_WinSock(ip_port iLocalPort, size_t iListenQueueSize)
	{
	this->pInit(INADDR_ANY, iLocalPort, iListenQueueSize);
	}

ZNetListener_TCP_WinSock::ZNetListener_TCP_WinSock(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize)
	{
	this->pInit(iLocalAddress, iLocalPort, iListenQueueSize);
	}

ZNetListener_TCP_WinSock::~ZNetListener_TCP_WinSock()
	{
	::closesocket(fSOCKET);
	}

ZRef<ZNetEndpoint> ZNetListener_TCP_WinSock::Listen()
	{
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSOCKET, &readSet);
	struct timeval timeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = 0;
	int result = ::select(0, &readSet, nil, nil, &timeOut);
	if (result > 0)
		{
		// We got one.
		sockaddr_in remoteSockAddr;
		ZBlockZero(&remoteSockAddr, sizeof(remoteSockAddr));
		int addrSize = sizeof(remoteSockAddr);
		SOCKET newSOCKET = ::accept(fSOCKET, (sockaddr*)&remoteSockAddr, &addrSize);
		if (newSOCKET != INVALID_SOCKET)
			{
			// Ensure new socket is blocking.
			unsigned long param = 0;
			if (::ioctlsocket(newSOCKET, FIONBIO, &param) == SOCKET_ERROR)
				::closesocket(newSOCKET);
			else
				return new ZNetEndpoint_TCP_WinSock(newSOCKET);
			}
		}
	return ZRef<ZNetEndpoint>();
	}

void ZNetListener_TCP_WinSock::CancelListen()
	{
	// It's hard to wake up a blocked call to accept, which is why we use a non-blocking
	// socket and a one second timeout on select. Rather than doing a full-blown
	// implementation here I'm just going to take advantage of the fact that callers are
	// supposed to retry their call to Listen if they get a nil endpoint back and have
	// CancelListen do nothing at all -- any pending call to Listen will return in at
	// most one second anyway.
	}

ip_port ZNetListener_TCP_WinSock::GetPort()
	{
	sockaddr_in localSockAddr;
	int length = sizeof(localSockAddr);
	if (::getsockname(fSOCKET, (sockaddr*)&localSockAddr, &length) >= 0)
		return ntohs(localSockAddr.sin_port);
	return 0;
	}

void ZNetListener_TCP_WinSock::pInit(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize)
	{
	fSOCKET = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSOCKET == INVALID_SOCKET)
		{
		int err = ::WSAGetLastError();
		throw ZNetEx(sTranslateError(err));
		}

	sockaddr_in localSockAddr;
	ZBlockZero(&localSockAddr, sizeof(localSockAddr));
	localSockAddr.sin_family = AF_INET;
	localSockAddr.sin_port = htons(iLocalPort);
	localSockAddr.sin_addr.s_addr = htonl(iLocalAddress);

	int reuseAddrFlag = 1;
	::setsockopt(fSOCKET, IPPROTO_TCP, SO_REUSEADDR, (char*)&reuseAddrFlag, sizeof(reuseAddrFlag));

	if (::bind(fSOCKET, (sockaddr*)&localSockAddr, sizeof(localSockAddr)) == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		::closesocket(fSOCKET);
		throw ZNetEx(sTranslateError(err));
		}

	if (::listen(fSOCKET, iListenQueueSize) == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		::closesocket(fSOCKET);
		throw ZNetEx(sTranslateError(err));
		}

	// Set the socket to be non-blocking
	unsigned long param = 1;
	if (::ioctlsocket(fSOCKET, FIONBIO, &param) == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		::closesocket(fSOCKET);
		throw ZNetEx(sTranslateError(err));
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_WinSock

ZNetEndpoint_TCP_WinSock::ZNetEndpoint_TCP_WinSock(SOCKET iSOCKET)
	{
	// Assumes that the socket is already connected
	fSOCKET = iSOCKET;
	// Make sure that Nagle algorithm is enabled
	int noDelayFlag = 0;
	::setsockopt(fSOCKET, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelayFlag, sizeof(noDelayFlag));
	}

ZNetEndpoint_TCP_WinSock::ZNetEndpoint_TCP_WinSock(ip_addr iRemoteHost, ip_port iRemotePort)
	{
	fSOCKET = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fSOCKET == INVALID_SOCKET)
		{
		int err = ::WSAGetLastError();
		throw ZNetEx(sTranslateError(err));
		}

	sockaddr_in remoteSockAddr;
	ZBlockSet(&remoteSockAddr, sizeof(remoteSockAddr), 0);
	remoteSockAddr.sin_family = AF_INET;
	remoteSockAddr.sin_port = htons(iRemotePort);
	remoteSockAddr.sin_addr.s_addr = htonl(iRemoteHost);
	if (::connect(fSOCKET, (sockaddr*)&remoteSockAddr, sizeof(remoteSockAddr)) < 0)
		{
		int err = ::WSAGetLastError();
		::closesocket(fSOCKET);
		throw ZNetEx(sTranslateError(err));
		}
	}

ZNetEndpoint_TCP_WinSock::~ZNetEndpoint_TCP_WinSock()
	{
	if (fSOCKET != INVALID_SOCKET)
		::closesocket(fSOCKET);
	}

void ZNetEndpoint_TCP_WinSock::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	char* localDest = static_cast<char*>(iDest);
	while (iCount)
		{
		int result = ::recv(fSOCKET, localDest, iCount, 0);
		if (result > 0)
			{
			localDest += result;
			iCount -= result;
			}
		else
			{
			break;
			}
		}
	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<char*>(iDest);
	}

size_t ZNetEndpoint_TCP_WinSock::Imp_CountReadable()
	{
	// Use non-blocking select to see if a read would succeed
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSOCKET, &readSet);
	struct timeval timeOut;
	timeOut.tv_sec = 0;
	timeOut.tv_usec = 0;
	int result = ::select(0, &readSet, nil, nil, &timeOut);
	if (result <= 0)
		return 0;

	// FIONREAD will give us a result without hanging.
	unsigned long localResult;
	if (::ioctlsocket(fSOCKET, FIONREAD, &localResult) == SOCKET_ERROR)
		return 0;
	return localResult;
	}

void ZNetEndpoint_TCP_WinSock::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	const char* localSource = static_cast<const char*>(iSource);
	while (iCount)
		{
		int result = ::send(fSOCKET, localSource, iCount, 0);
		if (result > 0)
			{
			localSource += result;
			iCount -= result;
			}
		else
			{
			break;
			}
		}
	if (oCountWritten)
		*oCountWritten = localSource - reinterpret_cast<const char*>(iSource);
	}

const ZStreamR& ZNetEndpoint_TCP_WinSock::GetStreamR()
	{ return *this; }

const ZStreamW& ZNetEndpoint_TCP_WinSock::GetStreamW()
	{ return *this; }

static bool sWaitReadable(SOCKET iSOCKET, int iMilliseconds)
	{
	fd_set readSet, exceptSet;
	FD_ZERO(&readSet);
	FD_ZERO(&exceptSet);
	FD_SET(iSOCKET, &readSet);
	FD_SET(iSOCKET, &exceptSet);

	struct timeval timeOut;
	timeOut.tv_sec = iMilliseconds / 1000;
	timeOut.tv_usec = (iMilliseconds % 1000) * 1000;
	return 0 < ::select(iSOCKET + 1, &readSet, nil, &exceptSet, &timeOut);
	}

bool ZNetEndpoint_TCP_WinSock::WaitTillReadable(int iMilliseconds)
	{
	return sWaitReadable(fSOCKET, iMilliseconds);
	}

void ZNetEndpoint_TCP_WinSock::SendDisconnect()
	{
	// Graceful close. See "Windows Sockets Network Programming" pp 231-232, Quinn, Shute. Addison Wesley. ISBN 0-201-63372-8
	::shutdown(fSOCKET, 1);
	}

bool ZNetEndpoint_TCP_WinSock::ReceiveDisconnect(int iMilliseconds)
	{
	ZTime endTime = ZTime::sSystem() + double(iMilliseconds) / 1000;

	bool gotIt = false;
	for (;;)
		{
		int result = ::recv(fSOCKET, ZooLib::sGarbageBuffer, sizeof(ZooLib::sGarbageBuffer), 0);
		if (result == 0)
			{
			// result is zero, indicating that the other end has sent FIN.
			gotIt = true;
			break;
			}
		else if (result < 0)
			{
			break;
			}
		}
	return gotIt;
	}

void ZNetEndpoint_TCP_WinSock::Abort()
	{
	if (fSOCKET == INVALID_SOCKET)
		return;
	struct linger theLinger;
	theLinger.l_onoff = 1;
	theLinger.l_linger = 1;
	::setsockopt(fSOCKET, SOL_SOCKET, SO_LINGER, (char*)&theLinger, sizeof(theLinger));
	::closesocket(fSOCKET);
	fSOCKET = INVALID_SOCKET;
	}

ZNetAddress* ZNetEndpoint_TCP_WinSock::GetRemoteAddress()
	{
	if (fSOCKET == INVALID_SOCKET)
		return nil;
	sockaddr_in remoteSockAddr;
	int length = sizeof(remoteSockAddr);
	if (::getpeername(fSOCKET, (sockaddr*)&remoteSockAddr, &length) >= 0)
		return new ZNetAddress_Internet(ntohl(remoteSockAddr.sin_addr.s_addr), ntohs(remoteSockAddr.sin_port));
	return nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpointDG_UDP_WinSock
#if 0
ZNetEndpointDG_UDP_WinSock::ZNetEndpointDG_UDP_WinSock()
:	fSOCKET(INVALID_SOCKET)
	{
	this->InternalAllocateSocket(0);
	}

ZNetEndpointDG_UDP_WinSock::ZNetEndpointDG_UDP_WinSock(ip_port inLocalPort)
:	fSOCKET(INVALID_SOCKET)
	{
	this->InternalAllocateSocket(inLocalPort);
	}

ZNet::Error ZNetEndpointDG_UDP_WinSock::Receive(void* inBuffer, size_t inBufferSize, size_t& outCountReceived, ZNetAddress*& outSourceAddress)
	{
	outCountReceived = 0;
	outSourceAddress = nil;

	if (fSOCKET == INVALID_SOCKET)
		return ZNet::errorBadState;

	sockaddr_in sourceSockAddr;
	ZBlockSet(&sourceSockAddr, sizeof(sourceSockAddr), 0);
	int sockAddrLen = 0;
	int result = ::recvfrom(fSOCKET, reinterpret_cast<char*>(inBuffer), inBufferSize, 0, (sockaddr*)&sourceSockAddr, &sockAddrLen);
	if (result == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		return ZNet_Internet_WinSock::sTranslateError(err);
		}
	outCountReceived = result;
	outSourceAddress = new ZNetAddress_Internet(ntohl(sourceSockAddr.sin_addr.s_addr), ntohs(sourceSockAddr.sin_port));
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpointDG_UDP_WinSock::Send(const void* inBuffer, size_t inCount, ZNetAddress* inDestAddress)
	{
	if (fSOCKET == INVALID_SOCKET)
		return ZNet::errorBadState;

// Here's a little nastiness. We have to do a cast on inDestAddress, hopefully we
// haven't been passed a bad address
	ZNetAddress_Internet* theInternetAddress = dynamic_cast<ZNetAddress_Internet*>(inDestAddress);
	ZAssertStop(2, theInternetAddress);

	sockaddr_in destSockAddr;
	ZBlockSet(&destSockAddr, sizeof(destSockAddr), 0);
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_port = htons(theInternetAddress->GetPort());
	destSockAddr.sin_addr.s_addr = htonl(theInternetAddress->GetAddress());
	int result = ::sendto(fSOCKET, (char*)inBuffer, inCount, 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));
	if (result == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		return ZNet_Internet_WinSock::sTranslateError(err);
		}
	return ZNet::errorNone;
	}

void ZNetEndpointDG_UDP_WinSock::InternalAllocateSocket(ip_port inLocalPort)
	{
	SOCKET theSOCKET = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (theSOCKET == INVALID_SOCKET)
		{
		int err = ::WSAGetLastError();
		throw ZNet::Ex(ZNet_Internet_WinSock::sTranslateError(err));
		}

	sockaddr_in localSockAddr;
	ZBlockSet(&localSockAddr, sizeof(localSockAddr), 0);
	localSockAddr.sin_family = AF_INET;
	localSockAddr.sin_port = htons(inLocalPort);
	localSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (::bind(theSOCKET, (sockaddr*)&localSockAddr, sizeof(localSockAddr)) == SOCKET_ERROR)
		{
		int err = ::WSAGetLastError();
		::closesocket(theSOCKET);
		throw ZNet::Ex(ZNet_Internet_WinSock::sTranslateError(err));
		}
	fSOCKET = theSOCKET;
	}
#endif

#endif // ZCONFIG_API_Enabled(Internet_WinSock)
