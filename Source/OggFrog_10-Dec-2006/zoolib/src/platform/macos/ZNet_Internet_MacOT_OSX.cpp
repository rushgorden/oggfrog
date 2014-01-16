static const char rcsid[] = "@(#) $Id: ZNet_Internet_MacOT_OSX.cpp,v 1.7 2006/10/13 20:33:04 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZNet_Internet_MacOT_OSX.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZMacMP

namespace ZMacMP {

typedef void (*EntryProc)(void*);

void sInvokeInMP(EntryProc iProc, void* iParam);

} // namespace ZMacMP

#if !ZCONFIG(Thread_API, Mac)
void ZMacMP::sInvokeInMP(EntryProc iProc, void* iParam)
	{
	try
		{
		iProc(iParam);
		}
	catch (...)
		{
		ZDebugStopf(1, ("sMPTaskEntry, caught an exception"));
		}
	}
#endif // !ZCONFIG(Thread_API, Mac)


#if ZCONFIG(Thread_API, Mac)

#include "ZAtomic.h"
#include "ZThreadTM.h"

#include <Multiprocessing.h>

static MPQueueID sQueue;

namespace ZANONYMOUS {

static int sInitCount;
class InitHelper
	{
public:
	InitHelper()
		{
		if (sInitCount++ == 0)
			{
			MPLibraryIsLoaded();
			::MPCreateQueue(&sQueue);
			}
		}

	~InitHelper()
		{
		if (--sInitCount == 0)
			::MPDeleteQueue(sQueue);
		}
	};
static InitHelper sInitHelper;

} // anonymous namespace

static ZAtomic_t sMPTaskCount;

static OSStatus sMPTaskEntry(void* iArg)
	{
	while (true)
		{
		void* param1;
		void* param2;
		void* param3;
		
		if (noErr != ::MPWaitOnQueue(sQueue, &param1, &param2, &param3, kDurationForever))
			break;

		try
			{
			((ZMacMP::EntryProc)param1)(param2);
			}
		catch (...)
			{
			ZDebugStopf(1, ("sMPTaskEntry, caught an exception"));
			}

		::ZThreadTM_Resume(static_cast<ZThreadTM_State*>(param3));

		if (ZAtomic_Add(&sMPTaskCount, 1) >= 5)
			{
			ZAtomic_Add(&sMPTaskCount, -1);
			break;
			}
		}
	return 0;
	}

void ZMacMP::sInvokeInMP(EntryProc iProc, void* iParam)
	{
	if (::MPTaskIsPreemptive(kInvalidID))
		{
		try
			{
			iProc(iParam);
			}
		catch (...)
			{
			ZDebugStopf(1, ("sMPTaskEntry, caught an exception"));
			}
		}
	else
		{
		ZThreadTM_State* theThread = ZThreadTM_Current();
		if (ZAtomic_Add(&sMPTaskCount, -1) <= 0)
			{
			ZAtomic_Add(&sMPTaskCount, 1);
			MPTaskID theTaskID;
			OSStatus err = ::MPCreateTask(sMPTaskEntry, nil, 64 * 1024, 0, 0, 0, 0, &theTaskID);
			ZAssertStop(1, err == noErr);
			}

		::MPNotifyQueue(sQueue, iProc, iParam, theThread);
		::ZThreadTM_Suspend();
		}
	}
#endif // ZCONFIG(Thread_API, Mac)

// =================================================================================================

#if ZCONFIG_API_Enabled(Internet_MacOT_OSX)

#include "ZMemory.h" // For ZBlockZero

#ifndef kDebug_OT
#	define kDebug_OT 1
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

static ZNet::Error sTranslateError(OTResult theOTResult)
	{
	if (theOTResult == kOTNoError)
		return ZNet::errorNone;
	return ZNet::errorGeneric;
	}

static OTResult sSetFourByteOption(EndpointRef ep, OTXTILevel level, OTXTIName name, UInt32 value)
	{
	// Set up the option buffer to specify the option and value to set.
	TOption option;
	option.len = kOTFourByteOptionSize;
	option.level = level;
	option.name = name;
	option.status = 0;
	option.value[0] = value;

	// Set up request parameter for OTOptionManagement
	TOptMgmt request;
	request.opt.buf = (UInt8 *) &option;
	request.opt.len = sizeof(option);
	request.flags = T_NEGOTIATE;

	// Set up reply parameter for OTOptionManagement.
	TOptMgmt result;
	result.opt.buf = (UInt8 *) &option;
	result.opt.maxlen = sizeof(option);
	OTResult err = ::OTOptionManagement(ep, &request, &result);
	if (err == noErr)
		{
		if (option.status != T_SUCCESS)
			err = option.status;
		}

	return err;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetNameLookup_Internet_MacOT_OSX

ZNetNameLookup_Internet_MacOT_OSX::ZNetNameLookup_Internet_MacOT_OSX(const string& iName, ip_port iPort, size_t iMaxAddresses)
:	fNetName(iName, iPort),
	fStarted(false),
	fCountAddressesToReturn(iMaxAddresses),
	fCurrentAddress(nil),
	fNextIndex(0)
	{
	if (0 == iName.size())
		{
		fStarted = true;
		fCountAddressesToReturn = 0;
		}
	}

ZNetNameLookup_Internet_MacOT_OSX::~ZNetNameLookup_Internet_MacOT_OSX()
	{
	delete fCurrentAddress;
	}

struct MPLookup_t
	{
	char* fName;
	InetHostInfo* fInetHostInfo;
	};

void ZNetNameLookup_Internet_MacOT_OSX::Start()
	{
	if (fStarted)
		return;

	fStarted = true;

	if (fCountAddressesToReturn == 0)
		return;

	MPLookup_t theStruct;
	theStruct.fName = const_cast<char*>(fNetName.GetName().c_str());
	theStruct.fInetHostInfo = &fInetHostInfo;
	ZMacMP::sInvokeInMP(sMP_Lookup, &theStruct);
	}

bool ZNetNameLookup_Internet_MacOT_OSX::Finished()
	{
	if (!fStarted)
		return true;

	if (0 == fCountAddressesToReturn)
		return true;

	while (fNextIndex < kMaxHostAddrs)
		{
		if (fInetHostInfo.addrs[fNextIndex])
			return false;
		++fNextIndex;
		}
	return true;
	}

void ZNetNameLookup_Internet_MacOT_OSX::Next()
	{
	ZAssertStop(kDebug_OT, fStarted);

	this->Internal_GrabNextResult();
	}

const ZNetAddress& ZNetNameLookup_Internet_MacOT_OSX::CurrentAddress()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(kDebug_OT, fCurrentAddress);
	return *fCurrentAddress;
	}

const ZNetName& ZNetNameLookup_Internet_MacOT_OSX::CurrentName()
	{
	if (!fCurrentAddress)
		this->Internal_GrabNextResult();

	ZAssertStop(kDebug_OT, fCurrentAddress);
	return fNetName;
	}

void ZNetNameLookup_Internet_MacOT_OSX::Internal_GrabNextResult()
	{
	while (fNextIndex < kMaxHostAddrs)
		{
		if (fInetHostInfo.addrs[fNextIndex])
			{
			delete fCurrentAddress;
			fCurrentAddress = new ZNetAddress_Internet(fInetHostInfo.addrs[fNextIndex], fNetName.GetPort());
			++fNextIndex;
			--fCountAddressesToReturn;
			break;
			}
		++fNextIndex;
		}
	}

void ZNetNameLookup_Internet_MacOT_OSX::sMP_Lookup(void* iParam)
	{
	MPLookup_t* theStruct = static_cast<MPLookup_t*>(iParam);

	OSStatus theErr;
	InetSvcRef theInetSvcRef = ::OTOpenInternetServicesInContext(kDefaultInternetServicesPath, 0, &theErr, nil);
	if (noErr != theErr)
		return;

	OTResult theResult = ::OTInetStringToAddress(theInetSvcRef, theStruct->fName, theStruct->fInetHostInfo);
	::OTCloseProvider(theInetSvcRef);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener_TCP_MacOT_OSX

struct ListenerConstruct_t
	{
	ZNetListener_TCP_MacOT_OSX* fListener;
	ip_port fLocalPort;
	size_t fListenQueueSize;
	OTResult fResult;
	};

ZNetListener_TCP_MacOT_OSX::ZNetListener_TCP_MacOT_OSX(ip_port iLocalPort, size_t iListenQueueSize)
	{
	if (iListenQueueSize == 0)
		iListenQueueSize = 5;

	if (iListenQueueSize > 10)
		iListenQueueSize = 10; // Let's not max out OT *too* much.

	// For the moment, remember the port in an instance variable
	// rather than pulling it from fEndpointRef
	fLocalPort = iLocalPort;

	fEndpointRef = 0;

	ListenerConstruct_t theStruct;
	theStruct.fListener = this;
	theStruct.fLocalPort = iLocalPort;
	theStruct.fListenQueueSize = iListenQueueSize;
	ZMacMP::sInvokeInMP(sMP_Constructor, &theStruct);

	if (theStruct.fResult != noErr)
		throw ZNetEx(sTranslateError(theStruct.fResult));
	}

void ZNetListener_TCP_MacOT_OSX::sMP_Constructor(void* iParam)
	{
	ListenerConstruct_t* theStruct = static_cast<ListenerConstruct_t*>(iParam);

	theStruct->fListener->fEndpointRef = ::OTOpenEndpointInContext(::OTCreateConfiguration("tilisten, tcp"), 0,
												nil, &theStruct->fResult, nil);

	if (theStruct->fResult == noErr)
		theStruct->fResult = ::OTSetBlocking(theStruct->fListener->fEndpointRef);
		
	if (theStruct->fResult == noErr)
		theStruct->fResult = ::sSetFourByteOption(theStruct->fListener->fEndpointRef, INET_IP, kIP_REUSEADDR, 1);

	if (theStruct->fResult == noErr)
		{
		InetAddress theInetAddress;
		::OTInitInetAddress(&theInetAddress, theStruct->fLocalPort, kOTAnyInetAddress); // port & host ip
		TBind bindReq;
		bindReq.addr.buf = (UInt8*)&theInetAddress;
		bindReq.addr.len = sizeof(theInetAddress);
		bindReq.qlen = theStruct->fListenQueueSize;
		theStruct->fResult = ::OTBind(theStruct->fListener->fEndpointRef, &bindReq, nil);
		}

	if (theStruct->fResult != noErr)
		{
		if (theStruct->fListener->fEndpointRef)
			::OTCloseProvider(theStruct->fListener->fEndpointRef);
		theStruct->fListener->fEndpointRef = nil;
		}
	}

struct ListenerGeneric_t
	{
	EndpointRef fEndpointRef;
	};

ZNetListener_TCP_MacOT_OSX::~ZNetListener_TCP_MacOT_OSX()
	{
	ListenerGeneric_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	ZMacMP::sInvokeInMP(sMP_Destructor, &theStruct);
	fEndpointRef = nil;
 	}

void ZNetListener_TCP_MacOT_OSX::sMP_Destructor(void* iParam)
	{
	ListenerGeneric_t* theStruct = static_cast<ListenerGeneric_t*>(iParam);
	if (theStruct->fEndpointRef)
		::OTCloseProvider(theStruct->fEndpointRef);
	}

static OSStatus OTAcceptQ(EndpointRef listener, EndpointRef worker, TCall* call)
	// My own personal wrapper around the OTAccept routine that handles 
	// the connection attempt disappearing cleanly.
	{
	// First try the Accept.
	OSStatus err = ::OTAccept(listener, worker, call);

	// If that fails with a look error, let's see what the problem is.
	if (err == kOTLookErr)
		{
		OTResult look = ::OTLook(listener);

		// Only two async events should be able to cause Accept to "look", namely 
		// T_LISTEN and T_DISCONNECT.  However, the "tilisten" module prevents 
		// further connection attempts coming up while we're still thinking about 
		// this one, so the only event that should come up is T_DISCONNECT.

		ZAssertStop(kDebug_OT, look == T_DISCONNECT);
		if (look == T_DISCONNECT)
			{
			// If we get a T_DISCONNECT, it should be for the current pending 
			// connection attempt.  We receive the disconnect info and check 
			// the sequence number against that in the call.  If they match, 
			// the connection attempt disappeared and we return kOTNoDataErr.
			// If they don't match, that's weird.

			TDiscon discon;
			::OTMemzero(&discon, sizeof(discon));

			OSStatus junk = ::OTRcvDisconnect(listener, &discon);
			ZAssertStop(kDebug_OT, junk == noErr);

			if (discon.sequence == call->sequence)
				{
				err = kOTNoDataErr;
				}
			else
				{
				ZAssertStop(kDebug_OT, false);
				// Leave err set to kOTLookErr.
				}
			}
		}
	return err;
	}

struct Listen_t
	{
	ZNetListener_TCP_MacOT_OSX* fListener;
	EndpointRef fAcceptedEndpointRef;
	InetAddress fInetAddress;
	};

ZRef<ZNetEndpoint> ZNetListener_TCP_MacOT_OSX::Listen()
	{
	Listen_t theStruct;
	theStruct.fListener = this;
	ZMacMP::sInvokeInMP(sMP_Listen, &theStruct);
	if (theStruct.fAcceptedEndpointRef)
		return new ZNetEndpoint_TCP_MacOT_OSX(theStruct.fAcceptedEndpointRef, theStruct.fInetAddress);
	else
		return ZRef<ZNetEndpoint>();
	}

void ZNetListener_TCP_MacOT_OSX::sMP_Listen(void* iParam)
	{
	Listen_t* theStruct = static_cast<Listen_t*>(iParam);

	TCall theTCall;
	ZBlockZero(&theTCall, sizeof(theTCall));
	theTCall.addr.buf = reinterpret_cast<UInt8*>(&theStruct->fInetAddress);
	theTCall.addr.maxlen = sizeof(theStruct->fInetAddress);

	OTResult theOTResult = ::OTListen(theStruct->fListener->fEndpointRef, &theTCall);
	if (theOTResult == noErr)
		{
		EndpointRef acceptedEndpointRef = ::OTOpenEndpointInContext(::OTCreateConfiguration("tcp"), 0,
													nil, &theOTResult, nil);
		if (acceptedEndpointRef)
			{
			theOTResult = ::OTSetBlocking(acceptedEndpointRef);
			if (theOTResult == noErr)		
				theOTResult = ::OTAcceptQ(theStruct->fListener->fEndpointRef, acceptedEndpointRef, &theTCall);
			if (theOTResult == noErr)
				{
				theStruct->fAcceptedEndpointRef = acceptedEndpointRef;
				return;
				}
			else if (theOTResult == kOTNoDataErr)
				{
				ZAssertStop(kDebug_OT, acceptedEndpointRef);
				::OTCloseProvider(acceptedEndpointRef);
				theOTResult = noErr;
				}
			}
		if (theOTResult)
			::OTSndDisconnect(theStruct->fListener->fEndpointRef, &theTCall);
		}
	theStruct->fAcceptedEndpointRef = nil;
	}

void ZNetListener_TCP_MacOT_OSX::CancelListen()
	{
	ListenerGeneric_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	ZMacMP::sInvokeInMP(sMP_CancelListen, &theStruct);
	}

void ZNetListener_TCP_MacOT_OSX::sMP_CancelListen(void* iParam)
	{
	ListenerGeneric_t* theStruct = static_cast<ListenerGeneric_t*>(iParam);
	::OTCancelSynchronousCalls(theStruct->fEndpointRef, kOTCanceledErr);
	}

ip_port ZNetListener_TCP_MacOT_OSX::GetPort()
	{ return fLocalPort; }

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpoint_TCP_MacOT_OSX

ZNetEndpoint_TCP_MacOT_OSX::ZNetEndpoint_TCP_MacOT_OSX(EndpointRef iEndpointRef, InetAddress& iRemoteInetAddress)
	{
	fEndpointRef = iEndpointRef;
	fRemoteHost = iRemoteInetAddress.fHost;
	fRemotePort = iRemoteInetAddress.fPort;
	}

struct EndpointConstruct_t
	{
	ZNetEndpoint_TCP_MacOT_OSX* fEndpoint;
	ip_addr fRemoteHost;
	ip_port fRemotePort;
	OTResult fResult;
	};

ZNetEndpoint_TCP_MacOT_OSX::ZNetEndpoint_TCP_MacOT_OSX(ip_addr iRemoteHost, ip_port iRemotePort)
	{
	fEndpointRef = nil;
	fRemoteHost = iRemoteHost;
	fRemotePort = iRemotePort;

	EndpointConstruct_t theStruct;
	theStruct.fEndpoint = this;
	theStruct.fRemoteHost = fRemoteHost;
	theStruct.fRemotePort = fRemotePort;
	ZMacMP::sInvokeInMP(sMP_Constructor, &theStruct);

	if (theStruct.fResult != noErr)
		throw ZNetEx(sTranslateError(theStruct.fResult));
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_Constructor(void* iParam)
	{
	EndpointConstruct_t* theStruct = static_cast<EndpointConstruct_t*>(iParam);

	theStruct->fEndpoint->fEndpointRef = ::OTOpenEndpointInContext(::OTCreateConfiguration(kTCPName), 0,
												nil, &theStruct->fResult, nil);

	if (theStruct->fResult == noErr)
		{
		ZAssertStop(kDebug_OT, ::OTIsSynchronous(theStruct->fEndpoint->fEndpointRef));
		theStruct->fResult = ::OTSetBlocking(theStruct->fEndpoint->fEndpointRef);
		}

	if (theStruct->fResult == noErr)
		theStruct->fResult = ::OTBind(theStruct->fEndpoint->fEndpointRef, nil, nil);

	if (theStruct->fResult == noErr)
		{
		InetAddress theInetAddress;
		theInetAddress.fAddressType = AF_INET;
		theInetAddress.fPort = theStruct->fRemotePort;
		theInetAddress.fHost = theStruct->fRemoteHost;

		TCall theSndCall;
		theSndCall.addr.buf = reinterpret_cast<UInt8*>(&theInetAddress);
		theSndCall.addr.len = sizeof(InetAddress);
		theSndCall.addr.maxlen = sizeof(InetAddress);

		theSndCall.opt.buf = nil; // no connection options
		theSndCall.opt.len = 0;
		theSndCall.opt.maxlen = 0;

		theSndCall.udata.buf = nil; // no connection data
		theSndCall.udata.len = 0;
		theSndCall.udata.maxlen = 0;

		OTResult theResult = ::OTConnect(theStruct->fEndpoint->fEndpointRef, &theSndCall, nil);
		if (theResult == T_DISCONNECT)
			{
			::OTRcvDisconnect(theStruct->fEndpoint->fEndpointRef, nil);
			theStruct->fResult = kEAGAINErr; //??
			}
		}

	if (theStruct->fResult != noErr)
		{
		if (theStruct->fEndpoint->fEndpointRef)
			::OTCloseProvider(theStruct->fEndpoint->fEndpointRef);
		theStruct->fEndpoint->fEndpointRef = nil;
		}
	}

struct EndpointGeneric_t
	{
	EndpointRef fEndpointRef;
	};

ZNetEndpoint_TCP_MacOT_OSX::~ZNetEndpoint_TCP_MacOT_OSX()
	{
	if (fEndpointRef)
		{
		EndpointGeneric_t theStruct;
		theStruct.fEndpointRef = fEndpointRef;
		ZMacMP::sInvokeInMP(sMP_Destructor, &theStruct);
		fEndpointRef = 0;
		}
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_Destructor(void* iParam)
	{
	EndpointGeneric_t* theStruct = static_cast<EndpointGeneric_t*>(iParam);
	::OTCloseProvider(theStruct->fEndpointRef);
	}

struct Imp_Read_t
	{
	EndpointRef fEndpointRef;
	void* fDest;
	size_t fCount;
	size_t* fCountRead;
	};

void ZNetEndpoint_TCP_MacOT_OSX::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	Imp_Read_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	theStruct.fDest = iDest;
	theStruct.fCount = iCount;
	theStruct.fCountRead = oCountRead;
	ZMacMP::sInvokeInMP(sMP_Imp_Read, &theStruct);
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_Imp_Read(void* iParam)
	{
	Imp_Read_t* theStruct = static_cast<Imp_Read_t*>(iParam);

	if (theStruct->fCountRead)
		*theStruct->fCountRead = 0;
	char* localDest = static_cast<char*>(theStruct->fDest);
	size_t countRemaining = theStruct->fCount;
	while (countRemaining)
		{
		OTFlags theFlags = 0;
		OTResult theResult = ::OTRcv(theStruct->fEndpointRef, localDest, countRemaining, &theFlags);
		if (theResult > 0)
			{
			if (theStruct->fCountRead)
				*theStruct->fCountRead += theResult;
			localDest += theResult;
			countRemaining -= theResult;
			}
		else
			{
			if (theResult != kOTLookErr)
				break;
			OTResult lookResult = ::OTLook(theStruct->fEndpointRef);
			if (lookResult == T_DISCONNECT)
				::OTRcvDisconnect(theStruct->fEndpointRef, nil);
			else if (lookResult == T_ORDREL)
				::OTRcvOrderlyDisconnect(theStruct->fEndpointRef);
			else
				ZDebugLogf(kDebug_OT, ("Unexpected OTLook result %d", lookResult));
			}
		}
	}

struct GetCountReadable_t
	{
	EndpointRef fEndpointRef;
	OTByteCount fCountReadable;
	};

size_t ZNetEndpoint_TCP_MacOT_OSX::Imp_CountReadable()
	{
	GetCountReadable_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	ZMacMP::sInvokeInMP(sMP_GetCountReadable, &theStruct);
	return theStruct.fCountReadable;
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_GetCountReadable(void* iParam)
	{
	GetCountReadable_t* theStruct = static_cast<GetCountReadable_t*>(iParam);
	if (kOTNoError != ::OTCountDataBytes(theStruct->fEndpointRef, &theStruct->fCountReadable))
		theStruct->fCountReadable = 0;
	}

struct Imp_Write_t
	{
	EndpointRef fEndpointRef;
	const void* fSource;
	size_t fCount;
	size_t* fCountWritten;
	};

void ZNetEndpoint_TCP_MacOT_OSX::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	Imp_Write_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	theStruct.fSource = iSource;
	theStruct.fCount = iCount;
	theStruct.fCountWritten = oCountWritten;
	ZMacMP::sInvokeInMP(sMP_Imp_Write, &theStruct);
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_Imp_Write(void* iParam)
	{
	Imp_Write_t* theStruct = static_cast<Imp_Write_t*>(iParam);

	if (theStruct->fCountWritten)
		*theStruct->fCountWritten = 0;
	const char* localSource = static_cast<const char*>(theStruct->fSource);
	size_t countRemaining = theStruct->fCount;
	while (countRemaining)
		{
		OTResult theResult = ::OTSnd(theStruct->fEndpointRef, const_cast<char*>(localSource), countRemaining, 0);
		if (theResult > 0)
			{
			if (theStruct->fCountWritten)
				*theStruct->fCountWritten += theResult;
			localSource += theResult;
			countRemaining -= theResult;
			}
		else
			{
			if (theResult != kOTLookErr)
				break;
			OTResult lookResult = ::OTLook(theStruct->fEndpointRef);
			if (lookResult == T_DISCONNECT)
				::OTRcvDisconnect(theStruct->fEndpointRef, nil);
			else if (lookResult == T_ORDREL)
				::OTRcvOrderlyDisconnect(theStruct->fEndpointRef);
			else
				ZDebugLogf(kDebug_OT, ("Unexpected OTLook result %d", lookResult));
			}
		}
	}

const ZStreamR& ZNetEndpoint_TCP_MacOT_OSX::GetStreamR()
	{ return *this; }

const ZStreamW& ZNetEndpoint_TCP_MacOT_OSX::GetStreamW()
	{ return *this; }

bool ZNetEndpoint_TCP_MacOT_OSX::WaitTillReadable(int iMilliseconds)
	{
	#warning AG NDY
	return false;
	}

void ZNetEndpoint_TCP_MacOT_OSX::SendDisconnect()
	{
	EndpointGeneric_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	ZMacMP::sInvokeInMP(sMP_SendDisconnect, &theStruct);
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_SendDisconnect(void* iParam)
	{
	EndpointGeneric_t* theStruct = static_cast<EndpointGeneric_t*>(iParam);

	while (true)
		{
		OTResult theResult = ::OTSndOrderlyDisconnect(theStruct->fEndpointRef);
		if (theResult != kOTLookErr)
			break;

		OTResult lookResult = ::OTLook(theStruct->fEndpointRef);
		if (lookResult == T_DISCONNECT)
			::OTRcvDisconnect(theStruct->fEndpointRef, nil);
		else if (lookResult == T_ORDREL)
			::OTRcvOrderlyDisconnect(theStruct->fEndpointRef);
		else
			ZDebugLogf(kDebug_OT, ("Unexpected OTLook result %d", lookResult));
		}
	}

struct ReceiveDisconnect_t
	{
	EndpointRef fEndpointRef;
	int fMilliseconds;
	bool fResult;
	};

bool ZNetEndpoint_TCP_MacOT_OSX::ReceiveDisconnect(int iMilliseconds)
	{
	ReceiveDisconnect_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	theStruct.fMilliseconds = iMilliseconds;
	ZMacMP::sInvokeInMP(sMP_ReceiveDisconnect, &theStruct);
	return theStruct.fResult;
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_ReceiveDisconnect(void* iParam)
	{
	ReceiveDisconnect_t* theStruct = static_cast<ReceiveDisconnect_t*>(iParam);

	while (true)
		{
		OTFlags theFlags = 0;
		OTResult theResult = ::OTRcv(theStruct->fEndpointRef, ZooLib::sGarbageBuffer, sizeof(ZooLib::sGarbageBuffer), &theFlags);
		if (theResult < 0)
			{
			if (theResult != kOTLookErr)
				break;
			OTResult lookResult = ::OTLook(theStruct->fEndpointRef);
			if (lookResult == T_DISCONNECT)
				{
				::OTRcvDisconnect(theStruct->fEndpointRef, nil);
				break;
				}
			else if (lookResult == T_ORDREL)
				{
				::OTRcvOrderlyDisconnect(theStruct->fEndpointRef);
				break;
				}
			else
				{
				ZDebugLogf(kDebug_OT, ("Unexpected OTLook result %d", lookResult));
				}
			}
		}
	theStruct->fResult = true;
	#warning "Not done yet. We're not really timing out here"
	}

void ZNetEndpoint_TCP_MacOT_OSX::Abort()
	{
	EndpointGeneric_t theStruct;
	theStruct.fEndpointRef = fEndpointRef;
	ZMacMP::sInvokeInMP(sMP_Abort, &theStruct);
	}

void ZNetEndpoint_TCP_MacOT_OSX::sMP_Abort(void* iParam)
	{
	EndpointGeneric_t* theStruct = static_cast<EndpointGeneric_t*>(iParam);
	::OTCancelSynchronousCalls(theStruct->fEndpointRef, kOTCanceledErr);
	::OTSndDisconnect(theStruct->fEndpointRef, nil);
	}

ZNetAddress* ZNetEndpoint_TCP_MacOT_OSX::GetRemoteAddress()
	{ return new ZNetAddress_Internet(fRemoteHost, fRemotePort); }

#endif // ZCONFIG_API_Enabled(Internet_MacOT_OSX)
