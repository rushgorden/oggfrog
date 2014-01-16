static const char rcsid[] = "@(#) $Id: ZNet_Internet_Socket.cpp,v 1.35 2006/05/06 02:28:36 agreen Exp $";

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
#include "ZNet_Internet_Socket.h"

// =================================================================================================
#if ZCONFIG_API_Enabled(Internet_Socket)

#include "ZMemory.h"
#include "ZTime.h"

#include <unistd.h>

#ifndef errno
#	include <errno.h>
#endif

#include <signal.h>

#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

// AG 2005-01-04. It looks like poll.h is not always present on MacOS X.
// We don't use poll on MacOS, so we can just skip the include for now.
#ifndef __APPLE__
#	define POLL_NO_WARN // Switch off Apple warning re poll().
#	include <poll.h>
#endif

using std::string;

// ==================================================
#pragma mark -
#pragma mark * ZNet_Internet_Socket

ZNet::Error ZNet_Internet_Socket::sTranslateError(int iNativeError)
	{
	ZNet::Error theError = ZNet::errorGeneric;
	switch (iNativeError)
		{
		case EADDRINUSE: theError = ZNet::errorLocalPortInUse; break;
		case EINVAL: theError = ZNet::errorBadState; break;
		case EBADF: theError = ZNet::errorBadState; break;
		case ECONNREFUSED: theError = ZNet::errorCouldntConnect; break;
		case 0: theError = ZNet::errorNone; break;
		default: break;
		}
	return theError;
	}

// ==================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_Socket

ZNetNameLookup_Internet_Socket::ZNetNameLookup_Internet_Socket(const string& iName, ip_port iPort, size_t iMaxAddresses)
:	fNetName(iName, iPort)
	{
	fStarted = false;
	fCurrentAddress = nil;
	fNextIndex = 0;
	fCountAddressesToReturn = iMaxAddresses;

	if (iName.empty())
		{
		fStarted = true;
		fCountAddressesToReturn = 0;
		}
	else
		{
		in_addr theInAddr;
		if (0 != ::inet_aton(fNetName.GetName().c_str(), &theInAddr))
			{
			fStarted = true;
			fAddresses.push_back(ntohl(theInAddr.s_addr));
			}
		}
	}

ZNetNameLookup_Internet_Socket::~ZNetNameLookup_Internet_Socket()
	{
	delete fCurrentAddress;
	}

void ZNetNameLookup_Internet_Socket::Start()
	{
	if (fStarted)
		return;

	fStarted = true;

#if __MACH__
	// FreeBSD uses thread-specific data to make gethostbyname reentrant.
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
#else
	// This is the thread-safe version
	char buffer[4096];
	hostent resultBuf;
	hostent* result;
	int herrno;
	if (0 == ::gethostbyname_r(fNetName.GetName().c_str(), &resultBuf, buffer, 4096, &result, &herrno))
		{
		if (result)
			{
			char** listPtr = result->h_addr_list;
			while (*listPtr && fCountAddressesToReturn)
				{
				in_addr* in_addrPtr = reinterpret_cast<in_addr*>(*listPtr);
				fAddresses.push_back(ntohl(in_addrPtr->s_addr));
				++listPtr;
				--fCountAddressesToReturn;
				}
			}
		}
#endif
	}

bool ZNetNameLookup_Internet_Socket::Finished()
	{
	if (!fStarted)
		return true;

	return fNextIndex >= fAddresses.size();
	}

void ZNetNameLookup_Internet_Socket::Next()
	{
	ZAssertStop(2, fStarted);

	this->Internal_GrabNextResult();
	}

const ZNetAddress& ZNetNameLookup_Internet_Socket::CurrentAddress()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return *fCurrentAddress;
	}

const ZNetName& ZNetNameLookup_Internet_Socket::CurrentName()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(2, fCurrentAddress);
	return fNetName;
	}

void ZNetNameLookup_Internet_Socket::Internal_GrabNextResult()
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

// ==================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_Socket

ZNetListener_TCP_Socket::ZNetListener_TCP_Socket(ip_port iLocalPort, size_t iListenQueueSize)
	{
	this->pInit(INADDR_ANY, iLocalPort, iListenQueueSize);
	}

ZNetListener_TCP_Socket::ZNetListener_TCP_Socket(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize)
	{
	this->pInit(iLocalAddress, iLocalPort, iListenQueueSize);
	}

ZNetListener_TCP_Socket::~ZNetListener_TCP_Socket()
	{
	::close(fSocketFD);
	}

ZRef<ZNetEndpoint> ZNetListener_TCP_Socket::Listen()
	{
	fMutexNR.Acquire();
	while (fListening)
		fCondition.Wait(fMutexNR);
	
	fListening = true;
	fMutexNR.Release();

	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSocketFD, &readSet);

	struct timeval timeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = 0;
	int result = ::select(fSocketFD + 1, &readSet, nil, nil, &timeOut);
	if (result <= 0)
		{
		fMutexNR.Acquire();
		fListening = false;
		fCondition.Broadcast();
		fMutexNR.Release();
		}
	else
		{
		sockaddr_in remoteSockAddr;
		ZBlockSet(&remoteSockAddr, sizeof(remoteSockAddr), 0);
		socklen_t addrSize = sizeof(remoteSockAddr);
		int result = ::accept(fSocketFD, (sockaddr*)&remoteSockAddr, &addrSize);

		fMutexNR.Acquire();
		fListening = false;
		fCondition.Broadcast();
		fMutexNR.Release();
	
		if (result > 0)
			{
			try
				{
				return new ZNetEndpoint_TCP_Socket(result);
				}
			catch (...)
				{}
			}
		}

	return ZRef<ZNetEndpoint>();
	}

void ZNetListener_TCP_Socket::CancelListen()
	{
	fMutexNR.Acquire();
	// We don't have to do anything. We just wait here till a pending listen
	// finishes its select/accept. Subsequent calls to Listen will queue up behind
	// this call to CancelListen and go ahead when we're done, but that's okay.
	while (fListening)
		fCondition.Wait(fMutexNR);
	fMutexNR.Release();
	}

ip_port ZNetListener_TCP_Socket::GetPort()
	{
	sockaddr_in localSockAddr;
	socklen_t length = sizeof(localSockAddr);
	if (::getsockname(fSocketFD, (sockaddr*)&localSockAddr, &length) >= 0)
		return ntohs(localSockAddr.sin_port);
	return 0;
	}

void ZNetListener_TCP_Socket::pInit(ip_addr iLocalAddress, ip_port iLocalPort, size_t iListenQueueSize)
	{
	fListening = false;

	fSocketFD = ::socket(PF_INET, SOCK_STREAM, 0);
	if (fSocketFD < 0)
		{
		int err = errno;
		throw ZNetEx(sTranslateError(err));
		}

	sockaddr_in localSockAddr;
	ZBlockSet(&localSockAddr, sizeof(localSockAddr), 0);
	localSockAddr.sin_family = AF_INET;
	localSockAddr.sin_port = htons(iLocalPort);
	localSockAddr.sin_addr.s_addr = htonl(iLocalAddress);

	// Enable SO_REUSEADDR, cause it's a real pain waiting for TIME_WAIT to expire
	int reuseAddrFlag = 1;
	::setsockopt(fSocketFD, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddrFlag, sizeof(reuseAddrFlag));

	if (::bind(fSocketFD, (sockaddr*)&localSockAddr, sizeof(localSockAddr)) < 0)
		{
		int err = errno;
		::close(fSocketFD);
		throw ZNetEx(sTranslateError(err));
		}

	if (::listen(fSocketFD, iListenQueueSize) < 0)
		{
		int err = errno;
		::close(fSocketFD);
		throw ZNetEx(sTranslateError(err));
		}

	::fcntl(fSocketFD, F_SETFL, ::fcntl(fSocketFD, F_GETFL,0) | O_NONBLOCK);
	}

// ==================================================
#pragma mark -
#pragma mark * Helper functions

// On MacOS X (and FreeBSD) and Linux a send or receive on a socket where the other end
// has closed can cause delivery of a sigpipe. These helper functions, and the conditional
// code in the ZNetEndpoint_TCP_Socket constructor, work around that issue.

#ifdef __APPLE__
// For MacOS X we set an option on the socket to indicate that sigpipe should not
// be delivered when the far end closes on us. That option was not defined in headers
// prior to 10.2, so we define it ourselves if necessary.

#ifndef SO_NOSIGPIPE
#	define SO_NOSIGPIPE 0x1022
#endif

#ifndef TCP_NODELAY
#	include <netinet/tcp.h>
#endif

static void sSetSocketOptions(int iSocket)
	{
	// Set the socket to be non blocking
	::fcntl(iSocket, F_SETFL, ::fcntl(iSocket, F_GETFL,0) | O_NONBLOCK);

	// Enable keep alive
	int keepAliveFlag = 1;
	::setsockopt(iSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAliveFlag, sizeof(keepAliveFlag));

	// Disable sigpipe when writing/reading a far-closed socket.
	int noSigPipeFlag = 1;
	::setsockopt(iSocket, SOL_SOCKET, SO_NOSIGPIPE, (char*)&noSigPipeFlag, sizeof(noSigPipeFlag));

	// There's no simple way to flush a socket under MacOS X. We could hold on to the last byte
	// written, then set NODELAY, write the byte, and clear NODELAY. I don't want to munge
	// this code to do that right now. So for the moment on MacOS X we simply set NODELAY and
	// expect higher level code to do buffering if it doesn't want to flood the connection with
	// lots of small packets. This is a reasonable expectation because it's what has to be done
	// anyway under Linux to prevent Nagle algorithm from kicking in.
	int noDelayFlag = 1;
	::setsockopt(iSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelayFlag, sizeof(noDelayFlag));
	}

static int sSend(int iSocket, const char* iSource, size_t iCount)
	{ return ::send(iSocket, iSource, iCount, 0); }

static int sReceive(int iSocket, char* iDest, size_t iCount)
	{ return ::recv(iSocket, iDest, iCount, 0); }

static bool sWaitReadable(int iSocket, int iMilliseconds)
	{
	fd_set readSet, exceptSet;
	FD_ZERO(&readSet);
	FD_ZERO(&exceptSet);
	FD_SET(iSocket, &readSet);
	FD_SET(iSocket, &exceptSet);

	struct timeval timeOut;
	timeOut.tv_sec = iMilliseconds / 1000;
	timeOut.tv_usec = (iMilliseconds % 1000) * 1000;
	return 0 < ::select(iSocket + 1, &readSet, nil, &exceptSet, &timeOut);
	}

static void sWaitWriteable(int iSocket)
	{
	fd_set writeSet;
	FD_ZERO(&writeSet);
	FD_SET(iSocket, &writeSet);

	struct timeval timeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = 0;

	::select(iSocket + 1, nil, &writeSet, nil, &timeOut);
	}

#elif defined(linux)

// RedHat Linux 6.1 adds the sigpipe on closed behavior to recv and send, and also adds
// a new flag that can be passed to supress that behavior. Older kernels return EINVAL
// if they don't recognize the flag, and thus don't have the behavior. So we use a static
// bool to remember if MSG_NOSIGNAL is available in the kernel.

static void sSetSocketOptions(int iSocket)
	{
	// Set the socket to be non blocking
	::fcntl(iSocket, F_SETFL, ::fcntl(iSocket, F_GETFL,0) | O_NONBLOCK);

	// Enable keep alive
	int keepAliveFlag = 1;
	::setsockopt(iSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAliveFlag, sizeof(keepAliveFlag));
	}

#ifndef MSG_NOSIGNAL
#	define MSG_NOSIGNAL 0x2000
#endif

static bool sCanUse_MSG_NOSIGNAL = false;
static bool sChecked_MSG_NOSIGNAL = false;

static int sSend(int iSocket, const char* iSource, size_t iCount)
	{
	if (sCanUse_MSG_NOSIGNAL)
		{
		return ::send(iSocket, iSource, iCount, MSG_NOSIGNAL);
		}
	else
		{
		if (sChecked_MSG_NOSIGNAL)
			{
			return ::send(iSocket, iSource, iCount, 0);
			}
		else
			{
			int result = ::send(iSocket, iSource, iCount, MSG_NOSIGNAL);
			if (result >= 0)
				{
				sCanUse_MSG_NOSIGNAL = true;
				sChecked_MSG_NOSIGNAL = true;
				}
			else
				{
				if (errno == EINVAL)
					{
					sChecked_MSG_NOSIGNAL = true;
					return ::send(iSocket, iSource, iCount, 0);
					}				
				}
			return result;
			}
		}
	}

static int sReceive(int iSocket, char* iDest, size_t iCount)
	{
	if (sCanUse_MSG_NOSIGNAL)
		{
		return ::recv(iSocket, iDest, iCount, MSG_NOSIGNAL);
		}
	else
		{
		if (sChecked_MSG_NOSIGNAL)
			{
			return ::recv(iSocket, iDest, iCount, 0);
			}
		else
			{
			int result = ::recv(iSocket, iDest, iCount, MSG_NOSIGNAL);

			if (result >= 0)
				{
				sCanUse_MSG_NOSIGNAL = true;
				sChecked_MSG_NOSIGNAL = true;
				}
			else
				{
				if (errno == EINVAL)
					{
					sChecked_MSG_NOSIGNAL = true;
					return ::recv(iSocket, iDest, iCount, 0);
					}				
				}
			return result;
			}
		}
	}

static bool sWaitReadable(int iSocket, int iMilliseconds)
	{
	pollfd thePollFD;
	thePollFD.fd = iSocket;
	thePollFD.events = POLLIN | POLLPRI;
	thePollFD.revents = 0;
	int result = ::poll(&thePollFD, 1, iMilliseconds);
	return result > 0;
	}

static void sWaitWriteable(int iSocket)
	{
	pollfd thePollFD;
	thePollFD.fd = iSocket;
	thePollFD.events = POLLOUT;
	::poll(&thePollFD, 1, 1000);
	}

#endif // defined(linux)

// ==================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_Socket

ZNetEndpoint_TCP_Socket::ZNetEndpoint_TCP_Socket(int iSocketFD)
	{
	fSocketFD = iSocketFD;
	ZAssertStop(2, fSocketFD != -1);

	sSetSocketOptions(fSocketFD);
	}

ZNetEndpoint_TCP_Socket::ZNetEndpoint_TCP_Socket(ip_addr iRemoteHost, ip_port iRemotePort)
	{
	fSocketFD = ::socket(PF_INET, SOCK_STREAM, 0);
	if (fSocketFD < 0)
		throw ZNetEx(sTranslateError(errno));

	sockaddr_in remoteSockAddr;
	ZBlockSet(&remoteSockAddr, sizeof(remoteSockAddr), 0);
	remoteSockAddr.sin_family = AF_INET;
	remoteSockAddr.sin_port = htons(iRemotePort);
	remoteSockAddr.sin_addr.s_addr = htonl(iRemoteHost);
	if (::connect(fSocketFD, (sockaddr*)&remoteSockAddr, sizeof(remoteSockAddr)) < 0)
		{
		::close(fSocketFD);
		throw ZNetEx(sTranslateError(errno));
		}

	sSetSocketOptions(fSocketFD);
	}

ZNetEndpoint_TCP_Socket::~ZNetEndpoint_TCP_Socket()
	{
	if (fSocketFD != -1)
		::close(fSocketFD);
	}

void ZNetEndpoint_TCP_Socket::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	char* localDest = static_cast<char*>(iDest);
	while (iCount)
		{
		int result = ::sReceive(fSocketFD, localDest, iCount);

		if (result < 0)
			{
			int err = errno;
			if (err == EAGAIN)
				{
				sWaitReadable(fSocketFD, 1000);
				}
			else if (err != EINTR)
				{
				break;
				}
			}
		else
			{
			if (result == 0)
				{
				// result is zero, indicating that the other end has sent FIN.
				break;
				}
			else
				{
				localDest += result;
				iCount -= result;
				}
			}
		}
	if (oCountRead)
		*oCountRead = localDest - static_cast<char*>(iDest);
	}

size_t ZNetEndpoint_TCP_Socket::Imp_CountReadable()
	{
	int localResult;
	if (0 <= ::ioctl(fSocketFD, FIONREAD, &localResult))
		return localResult;
	return 0;
	}

void ZNetEndpoint_TCP_Socket::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	const char* localSource = static_cast<const char*>(iSource);
	while (iCount)
		{
		int result = ::sSend(fSocketFD, localSource, iCount);

		if (result < 0)
			{
			int err = errno;
			if (err == EAGAIN)
				{
				sWaitWriteable(fSocketFD);
				}
			else if (err != EINTR)
				{
				break;
				}
			}
		else
			{
			if (result == 0)
				{
				// result is zero, indicating that the other end has closed its receive side.
				break;
				}
			else
				{
				localSource += result;
				iCount -= result;
				}
			}
		}
	if (oCountWritten)
		*oCountWritten = localSource - static_cast<const char*>(iSource);
	}

const ZStreamR& ZNetEndpoint_TCP_Socket::GetStreamR()
	{ return *this; }

const ZStreamW& ZNetEndpoint_TCP_Socket::GetStreamW()
	{ return *this; }

bool ZNetEndpoint_TCP_Socket::WaitTillReadable(int iMilliseconds)
	{
	return sWaitReadable(fSocketFD, iMilliseconds);
	}

void ZNetEndpoint_TCP_Socket::SendDisconnect()
	{
	::shutdown(fSocketFD, SHUT_WR);
	}

bool ZNetEndpoint_TCP_Socket::ReceiveDisconnect(int iMilliseconds)
	{
	ZTime endTime = ZTime::sSystem() + double(iMilliseconds) / 1000;

	bool gotIt = false;
	for (;;)
		{
		int result = ::sReceive(fSocketFD, ZooLib::sGarbageBuffer, sizeof(ZooLib::sGarbageBuffer));
		if (result == 0)
			{
			// result is zero, indicating that the other end has sent FIN.
			gotIt = true;
			break;
			}
		else if (result < 0)
			{
			int err = errno;
			if (err == EAGAIN)
				{
				if (iMilliseconds < 0)
					{
					sWaitReadable(fSocketFD, 1000);
					}
				else
					{
					ZTime now = ZTime::sSystem();
					if (endTime < now)
						break;
					sWaitReadable(fSocketFD, int(1000 * (endTime - now)));
					}
				}
			else if (err != EINTR)
				{
				break;
				}
			}
		}
	return gotIt;
	}

void ZNetEndpoint_TCP_Socket::Abort()
	{
	// Cause a RST to be sent when we close(). See UNIX Network Programming, 2nd Ed, Stevens, page 423
	struct linger theLinger;
	theLinger.l_onoff = 1;
	// AG 2001-10-04. [Stevens] says that l_linger should be 0. The code previously set l_linger
	// to be 1, I'm not sure if we found that to be correct in practice so I'm correcting it --
	// if we experience problems, let me know.
	theLinger.l_linger = 0;
	::setsockopt(fSocketFD, SOL_SOCKET, SO_LINGER, (char*)&theLinger, sizeof(theLinger));
	::shutdown(fSocketFD, SHUT_RDWR);
	}

ZNetAddress* ZNetEndpoint_TCP_Socket::GetRemoteAddress()
	{
	sockaddr_in remoteSockAddr;
	socklen_t length = sizeof(remoteSockAddr);
	if (::getpeername(fSocketFD, (sockaddr*)&remoteSockAddr, &length) >= 0)
		return new ZNetAddress_Internet(ntohl(remoteSockAddr.sin_addr.s_addr), ntohs(remoteSockAddr.sin_port));
	return nil;
	}

#endif // ZCONFIG_API_Enabled(Internet_Socket)
// =================================================================================================
