static const char rcsid[] = "@(#) $Id: ZNet_Internet.cpp,v 1.12 2006/07/17 18:58:30 agreen Exp $";

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

#include "ZNet_Internet.h"
#include "ZNet_Internet_Be.h"
#include "ZNet_Internet_MacOT_Classic.h"
#include "ZNet_Internet_MacOT_OSX.h"
#include "ZNet_Internet_Socket.h"
#include "ZNet_Internet_WinSock.h"
#include "ZMacOSX.h"
#include "ZString.h"

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZNetAddress_Internet

ZNetAddress_Internet::ZNetAddress_Internet()
	{}

ZNetAddress_Internet::ZNetAddress_Internet(const ZNetAddress_Internet& other)
:	fHost(other.fHost),
	fPort(other.fPort)
	{}

ZNetAddress_Internet::ZNetAddress_Internet(ip_addr iHost, ip_port iPort)
:	fHost(iHost),
	fPort(iPort)
	{}

ZNetAddress_Internet::ZNetAddress_Internet(uint8 iHost1, uint8 iHost2, uint8 iHost3, uint8 iHost4, ip_port iPort)
:	fHost((iHost1 << 24) | (iHost2 << 16) | (iHost3 << 8) | iHost4),
	fPort(iPort)
	{}

ZNetAddress_Internet::~ZNetAddress_Internet()
	{}

ZNetAddress_Internet& ZNetAddress_Internet::operator=(const ZNetAddress_Internet& other)
	{
	fHost = other.fHost;
	fPort = other.fPort;
	return *this;
	}

ZNetAddress* ZNetAddress_Internet::Clone() const
	{ return new ZNetAddress_Internet(*this); }

ZRef<ZNetEndpoint> ZNetAddress_Internet::OpenRW() const
	{ return ZNetEndpoint_TCP::sCreateConnectedEndpoint(fHost, fPort); }

// =================================================================================================
#pragma mark -
#pragma mark * ZNetName_Internet

ZNetName_Internet::ZNetName_Internet()
:	fPort(0)
	{}

ZNetName_Internet::ZNetName_Internet(const ZNetName_Internet& other)
:	fName(other.fName), fPort(other.fPort)
	{}

ZNetName_Internet::ZNetName_Internet(const string& inName, ip_port iPort)
:	fName(inName), fPort(iPort)
	{}

ZNetName_Internet::~ZNetName_Internet()
	{}

ZNetName_Internet& ZNetName_Internet::operator=(const ZNetName_Internet& other)
	{
	fName = other.fName;
	fPort = other.fPort;
	return *this;
	}

ZNetName* ZNetName_Internet::Clone() const
	{ return new ZNetName_Internet(*this); }

string ZNetName_Internet::AsString() const
	{ return fName + ZString::sFormat(":%d", fPort); }

ZNetNameLookup* ZNetName_Internet::CreateLookup(size_t inMaxAddresses) const
	{
	#if ZCONFIG_API_Enabled(Internet_Socket)
		return new ZNetNameLookup_Internet_Socket(fName, fPort, inMaxAddresses);
	#endif

	#if ZCONFIG_API_Enabled(Internet_MacOT_OSX)
		if (ZMacOSX::sIsMacOSX())
			return new ZNetNameLookup_Internet_MacOT_OSX(fName, fPort, inMaxAddresses);
	#endif

	#if ZCONFIG_API_Enabled(Internet_MacOT_Classic)
		if (!ZMacOSX::sIsMacOSX())
			return new ZNetNameLookup_Internet_MacOT_Classic(fName, fPort, inMaxAddresses);
	#endif

	#if ZCONFIG_API_Enabled(Internet_MacClassic)
		return new ZNetNameLookup_Internet_MacClassic(fName, fPort, inMaxAddresses);
	#endif

	#if ZCONFIG_API_Enabled(Internet_WinSock)
		return new ZNetNameLookup_Internet_WinSock(fName, fPort, inMaxAddresses);
	#endif

	#if ZCONFIG_API_Enabled(Internet_BeSocket)
		return new ZNetNameLookup_Internet_BeSocket(fName, fPort, inMaxAddresses);
	#endif

	return nil;
	}

const string& ZNetName_Internet::GetName() const
	{ return fName; }

ip_port ZNetName_Internet::GetPort() const
	{ return fPort; }

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener_TCP

ZRef<ZNetListener_TCP> ZNetListener_TCP::sCreateListener(ip_port iPort, size_t iListenQueueSize)
	{
	try
		{
		#if ZCONFIG_API_Enabled(Internet_Socket)
			return new ZNetListener_TCP_Socket(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacOT_OSX)
			if (ZMacOSX::sIsMacOSX())
				return new ZNetListener_TCP_MacOT_OSX(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacOT_Classic)
			if (!ZMacOSX::sIsMacOSX())
				return new ZNetListener_TCP_MacOT_Classic(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacClassic)
			return new ZNetListener_TCP_MacClassic(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_WinSock)
			return new ZNetListener_TCP_WinSock(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_BeSocket)
			return new ZNetListener_TCP_BeSocket(iPort, iListenQueueSize);
		#endif
		}
	catch (...)
		{}

	return ZRef<ZNetListener_TCP>();
	}

ZRef<ZNetListener_TCP> ZNetListener_TCP::sCreateListener(ip_addr iAddress, ip_port iPort, size_t iListenQueueSize)
	{
	// We currently only support binding to a specific address with the Socket and WinSock APIs.
	try
		{
		#if ZCONFIG_API_Enabled(Internet_Socket)
			return new ZNetListener_TCP_Socket(iAddress, iPort, iListenQueueSize);
		#endif

		#if 0 // ZCONFIG_API_Enabled(Internet_MacOT_OSX)
			if (ZMacOSX::sIsMacOSX())
				return new ZNetListener_TCP_MacOT_OSX(iPort, iListenQueueSize);
		#endif

		#if 0 // ZCONFIG_API_Enabled(Internet_MacOT_Classic)
			if (!ZMacOSX::sIsMacOSX())
				return new ZNetListener_TCP_MacOT_Classic(iPort, iListenQueueSize);
		#endif

		#if 0 // ZCONFIG_API_Enabled(Internet_MacClassic)
			return new ZNetListener_TCP_MacClassic(iPort, iListenQueueSize);
		#endif

		#if ZCONFIG_API_Enabled(Internet_WinSock)
			return new ZNetListener_TCP_WinSock(iAddress, iPort, iListenQueueSize);
		#endif

		#if 0 // ZCONFIG_API_Enabled(Internet_BeSocket)
			return new ZNetListener_TCP_BeSocket(iPort, iListenQueueSize);
		#endif
		}
	catch (...)
		{}

	return ZRef<ZNetListener_TCP>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener_TCP

ZRef<ZNetEndpoint_TCP> ZNetEndpoint_TCP::sCreateConnectedEndpoint(ip_addr iRemoteHost, ip_port iRemotePort)
	{
	try
		{
		#if ZCONFIG_API_Enabled(Internet_Socket)
			return new ZNetEndpoint_TCP_Socket(iRemoteHost, iRemotePort);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacOT_OSX)
			if (ZMacOSX::sIsMacOSX())
				return new ZNetEndpoint_TCP_MacOT_OSX(iRemoteHost, iRemotePort);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacOT_Classic)
			if (!ZMacOSX::sIsMacOSX())
				return new ZNetEndpoint_TCP_MacOT_Classic(iRemoteHost, iRemotePort);
		#endif

		#if ZCONFIG_API_Enabled(Internet_MacClassic)
			return new ZNetEndpoint_TCP_MacClassic(iRemoteHost, iRemotePort);
		#endif

		#if ZCONFIG_API_Enabled(Internet_WinSock)
			return new ZNetEndpoint_TCP_WinSock(iRemoteHost, iRemotePort);
		#endif

		#if ZCONFIG_API_Enabled(Internet_BeSocket)
			return new ZNetEndpoint_TCP_BeSocket(iRemoteHost, iRemotePort);
		#endif
		}
	catch (...)
		{}

	return ZRef<ZNetEndpoint_TCP>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpointDG_UDP

ZRef<ZNetEndpointDG_UDP> ZNetEndpointDG_UDP::sCreateEndpoint()
	{
#if ZCONFIG_API_Enabled(Internet_MacClassic)
//	return new ZNetEndpointDG_UDP_MacClassic();
#endif

#if ZCONFIG_API_Enabled(Internet_WinSock)
//	return new ZNetEndpointDG_UDP_WinSock;
#endif

#if ZCONFIG_API_Enabled(Internet_Socket)
//	return new ZNetEndpointDG_UDP_Socket;
#endif

#if ZCONFIG_API_Enabled(Internet_BeSocket)
//	return new ZNetEndpointDG_UDP_BeSocket;
#endif

	return ZRef<ZNetEndpointDG_UDP>();
	}

ZRef<ZNetEndpointDG_UDP> ZNetEndpointDG_UDP::sCreateEndpoint(ip_port iLocalPort)
	{
#if ZCONFIG_API_Enabled(Internet_MacClassic)
//	return new ZNetEndpointDG_UDP_MacClassic(iLocalPort);
#endif

#if ZCONFIG_API_Enabled(Internet_WinSock)
//	return new ZNetEndpointDG_UDP_WinSock(iLocalPort);
#endif

#if ZCONFIG_API_Enabled(Internet_Socket)
//	return new ZNetEndpointDG_UDP_Socket(iLocalPort);
#endif

#if ZCONFIG_API_Enabled(Internet_BeSocket)
//	return new ZNetEndpointDG_UDP_BeSocket(iLocalPort);
#endif

	return ZRef<ZNetEndpointDG_UDP>();
	}

