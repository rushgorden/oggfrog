static const char rcsid[] = "@(#) $Id: ZNet_Internet_Be.cpp,v 1.6 2006/04/10 20:14:51 agreen Exp $";

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

#include "ZNet_Internet_Be.h"

#if ZCONFIG_API_Enabled(Internet_BeSocket)

// =================================================================================================

#include <errno.h>
#include <net/netdb.h>

// ==================================================
#pragma mark -
#pragma mark * ZNet_Internet_BeSocket

ZNet::Error ZNet_Internet_BeSocket::sTranslateError(int inNativeError)
	{
	ZNet::Error theError = ZNet::errorGeneric;
	switch (inNativeError)
		{
		case 0: theError = ZNet::errorNone; break;
		default: break;
		}
	return theError;
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_BeSocket

ZNetNameLookup_Internet_BeSocket::ZNetNameLookup_Internet_BeSocket(const string& inName, ip_port inPort, size_t inMaxAddresses)
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
	}

ZNetNameLookup_Internet_BeSocket::~ZNetNameLookup_Internet_BeSocket()
	{
	delete fCurrent;
	}

void ZNetNameLookup_Internet_BeSocket::Start()
	{
	if (fStarted)
		return;

	fStarted = true;

	if (hostent* theHostEnt = ::gethostbyname(fNetName.GetName().c_str()))
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
	}

bool ZNetNameLookup_Internet_BeSocket::Finished()
	{
	if (!fStarted)
		return true;

	return fNextIndex >= fAddresses.size();
	}

void ZNetNameLookup_Internet_BeSocket::Next()
	{
	ZAssertStop(2, fStarted);

	this->Internal_GrabNextResult();
	}

const ZNetAddress& ZNetNameLookup_Internet_BeSocket::CurrentAddress()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return *fCurrentAddress;
	}

const ZNetName& ZNetNameLookup_Internet_BeSocket::CurrentName()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return fNetName;
	}

void ZNetNameLookup_Internet_BeSocket::Internal_GrabNextResult()
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

#if 0
// ==================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_BeSocket

ZNetListener_TCP_BeSocket::ZNetListener_TCP_BeSocket(ip_port inLocalPort, size_t inListenQueueSize)
	{
	fLocalPort = inLocalPort;
	fSocket = -1;

	long theSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (theSocket <0)
		{
		int err = errno;
		throw ZNet::Ex(ZNet_Internet_BeSocket::sTranslateError(err));
		}

	sockaddr_in localSockAddr;
	ZBlockSet(&localSockAddr, sizeof(localSockAddr), 0);
	localSockAddr.sin_family = AF_INET;
	localSockAddr.sin_port = htons(fLocalPort);
	localSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//	int reuseAddrFlag = 1;
	//	::setsockopt(theSocket, IPPROTO_TCP, SO_REUSEADDR, (char*)&reuseAddrFlag, sizeof(reuseAddrFlag));

	if (::bind(theSocket, (sockaddr*)&localSockAddr, sizeof(localSockAddr)) < 0)
		{
		int err = errno;
		::closesocket(theSocket);
		throw ZNet::Ex(ZNet_Internet_BeSocket::sTranslateError(err));
		}
	if (::listen(theSocket, inListenQueueSize) < 0)
		{
		int err = errno;
		::closesocket(theSocket);
		throw ZNet::Ex(ZNet_Internet_BeSocket::sTranslateError(err));
		}
	fSocket = theSocket;
	}

ZNetListener_TCP_BeSocket::~ZNetListener_TCP_BeSocket()
	{
	ZMutexLocker accessLocker(fAccessMutex);
	long theSocket = fSocket;
	fSocket = -1;
	::closesocket(theSocket);
	accessLocker.Release();

	// Wait for listen to exit
	ZMutexLocker listenLocker(fListenMutex);
	}

ZNetEndpoint* ZNetListener_TCP_BeSocket::Listen()
	{
	ZMutexLocker locker(fListenMutex);

	ZNetEndpoint_TCP_BeSocket* theEndpoint = nil;
	while (true)
		{
		ZMutexLocker accessLocker(fAccessMutex);
		if (fSocket == -1)
			break;

		// Now block on the accept call
		sockaddr_in remoteSockAddr;
		ZBlockSet(&remoteSockAddr, sizeof(remoteSockAddr), 0);
		int addrSize = sizeof(remoteSockAddr);
		accessLocker.Release();
		long newSocket = ::accept(fSocket, (sockaddr*)&remoteSockAddr, &addrSize);
		accessLocker.Acquire();
		if (newSocket >= 0)
			{
			if (fSocket == -1)
				{
				::closesocket(newSocket);
				break;
				}
			try
				{
				theEndpoint = new ZNetEndpoint_TCP_BeSocket(newSocket);
				}
			catch (...)
				{}
			break;
			}
		}
	return theEndpoint;
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_BeSocket

ZNetEndpoint_TCP_BeSocket::ZNetEndpoint_TCP_BeSocket(long inSocket)
	{
	// Assumes that the socket is already connected
	fSocket = inSocket;
	}

ZNetEndpoint_TCP_BeSocket::ZNetEndpoint_TCP_BeSocket()
	{
	fSocket = -1;
	}

ZNetEndpoint_TCP_BeSocket::~ZNetEndpoint_TCP_BeSocket()
	{
	if (fSocket == -1)
		return;
	::closesocket(fSocket);
	}

ZNet::State ZNetEndpoint_TCP_BeSocket::GetState()
	{
	// The sockets API does not give us much information about
	// what state the socket is in.
	if (fSocket == -1)
		return ZNet::stateClosed;
	return ZNet::stateConnected;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Imp_CountReadable(size_t& outCount)
	{
	outCount = 0;
	return ZNet::errorCantGetAmountUnread;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Connect(ip_addr inRemoteAddress, ip_port inRemotePort)
	{
	if (fSocket != -1)
		return ZNet::errorBadState;

	long theSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (theSocket < 0)
		{
		int err = errno;
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}

	sockaddr_in remoteSockAddr;
	ZBlockSet(&remoteSockAddr, sizeof(remoteSockAddr), 0);
	remoteSockAddr.sin_family = AF_INET;
	remoteSockAddr.sin_port = htons(inRemotePort);
	remoteSockAddr.sin_addr.s_addr = htonl(inRemoteAddress);
	if (::connect(theSocket, (sockaddr*)&remoteSockAddr, sizeof(remoteSockAddr)) < 0)
		{
		int err = errno;
		::closesocket(theSocket);
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}
	fSocket = theSocket;
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Disconnect()
	{
	if (fSocket == -1)
		return ZNet::errorBadState;

	long theSocket = fSocket;
	fSocket = -1;
	if (::closesocket(theSocket) < 0)
		{
		int err = errno;
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Abort()
	{
	if (fSocket == -1)
		return ZNet::errorBadState;

	long theSocket = fSocket;
	fSocket = -1;
	//	struct linger theLinger;
	//	theLinger.l_onoff = 1;
	//	theLinger.l_linger = 0;
	//	::setsockopt(theSocket, SOL_SOCKET, SO_LINGER, (char*)&theLinger, sizeof(theLinger));
	if (::closesocket(theSocket) < 0)
		{
		int err = errno;
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Receive(void* inDest, size_t inCount, size_t* outCountReceived)
	{
	if (outCountReceived)
		*outCountReceived = 0;
	if (fSocket == -1)
		return ZNet::errorBadState;

	char* localDest = reinterpret_cast<char*>(inDest);
	size_t countRemaining = inCount;
	while (countRemaining)
		{
		int result = ::recv(fSocket, localDest, countRemaining, 0);
		// Zero result indicates that the other end has gone dead
		if (result == 0)
			{
			::closesocket(fSocket);
			fSocket = -1;
			return ZNet::errorGeneric;
			}
		if (result < 0)
			{
			int err = errno;
			if (err != EINTR)
				{
				::closesocket(fSocket);
				fSocket = -1;
				return ZNet_Internet_BeSocket::sTranslateError(err);
				}
			result = 0;
			}
		if (outCountReceived)
			*outCountReceived += result;
		countRemaining -= result;
		localDest += result;
		}
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpoint_TCP_BeSocket::Send(const void* inSource, size_t inCount, size_t* outCountSent, bool inPush)
	{
	if (outCountSent)
		*outCountSent = 0;

	if (fSocket == -1)
		return ZNet::errorBadState;

	const char* localSource = reinterpret_cast<const char*>(inSource);
	size_t countRemaining = inCount;
	while (countRemaining)
		{
		int result = ::send(fSocket, localSource, countRemaining, 0);
		if (result < 0)
			{
			int err = errno;
			if (err != EINTR)
				{
				::closesocket(fSocket);
				fSocket = -1;
				return ZNet_Internet_BeSocket::sTranslateError(err);
				}
			result = 0;
			}
		if (outCountSent)
			*outCountSent += result;
		countRemaining -= result;
		localSource += result;
		}
	return ZNet::errorNone;
	}

ZNetAddress* ZNetEndpoint_TCP_BeSocket::GetRemoteAddress()
	{
	if (fSocket == -1)
		return nil;
	sockaddr_in remoteSockAddr;
	int length = sizeof(remoteSockAddr);
	if (::getpeername(fSocket, (sockaddr*)&remoteSockAddr, &length) < 0)
		return nil;

	return new ZNetAddress_Internet(ntohl(remoteSockAddr.sin_addr.s_addr), ntohs(remoteSockAddr.sin_port));
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpointDG_UDP_BeSocket

ZNetEndpointDG_UDP_BeSocket::ZNetEndpointDG_UDP_BeSocket()
:	fSocket(-1)
	{
	this->InternalAllocateSocket(0);
	}

ZNetEndpointDG_UDP_BeSocket::ZNetEndpointDG_UDP_BeSocket(ip_port inLocalPort)
:	fSocket(-1)
	{
	this->InternalAllocateSocket(inLocalPort);
	}

ZNet::Error ZNetEndpointDG_UDP_BeSocket::Receive(void* inBuffer, size_t inBufferSize, size_t& outCountReceived, ZNetAddress*& outSourceAddress)
	{
	outCountReceived = 0;
	outSourceAddress = nil;

	if (fSocket == -1)
		return ZNet::errorBadState;

	sockaddr_in sourceSockAddr;
	ZBlockSet(&sourceSockAddr, sizeof(sourceSockAddr), 0);
	int sockAddrLen = 0;
	int result = ::recvfrom(fSocket, reinterpret_cast<char*>(inBuffer), inBufferSize, 0, (sockaddr*)&sourceSockAddr, &sockAddrLen);
	if (result < 0)
		{
		int err = errno;
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}
	outCountReceived = result;
	outSourceAddress = new ZNetAddress_Internet(ntohl(sourceSockAddr.sin_addr.s_addr), ntohs(sourceSockAddr.sin_port));
	return ZNet::errorNone;
	}

ZNet::Error ZNetEndpointDG_UDP_BeSocket::Send(const void* inBuffer, size_t inCount, ZNetAddress* inDestAddress)
	{
	if (fSocket == -1)
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
	int result = ::sendto(fSocket, reinterpret_cast<char*>(inBuffer), inCount, 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));
	if (result < 0)
		{
		int err = errno;
		return ZNet_Internet_BeSocket::sTranslateError(err);
		}
	return ZNet::errorNone;
	}

void ZNetEndpointDG_UDP_BeSocket::InternalAllocateSocket(ip_port inLocalPort)
	{
	long theSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (theSocket < 0)
		{
		int err = errno;
		throw ZNet::Ex(ZNet_Internet_BeSocket::sTranslateError(err));
		}

	sockaddr_in localSockAddr;
	ZBlockSet(&localSockAddr, sizeof(localSockAddr), 0);
	localSockAddr.sin_family = AF_INET;
	localSockAddr.sin_port = htons(inLocalPort);
	localSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (::bind(theSocket, (sockaddr*)&localSockAddr, sizeof(localSockAddr)) < 0)
		{
		int err = errno;
		::closesocket(theSocket);
		throw ZNet::Ex(ZNet_Internet_BeSocket::sTranslateError(err));
		}
	fSocket = theSocket;
	}
#endif

// =================================================================================================
#endif // ZCONFIG_API_Enabled(Internet_BeSocket)
