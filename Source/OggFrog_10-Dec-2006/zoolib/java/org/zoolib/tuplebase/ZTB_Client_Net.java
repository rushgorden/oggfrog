// "@(#) $Id: ZTB_Client_Net.java,v 1.4 2005/03/18 14:51:50 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2004 Andrew Green and Learning in Motion, Inc.
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

package org.zoolib.tuplebase;

import java.net.Socket;
import java.io.InputStream;
import java.io.IOException;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.OutputStream;

// ====================================================================================================

public class ZTB_Client_Net extends ZTB_Client
	{
	static boolean useZlib = false;
	public ZTB_Client_Net(java.lang.String iHostname, int iPort)
		{
		try
			{
			Socket theSocket = new Socket(iHostname, iPort);

			InputStream theInputStream = theSocket.getInputStream();
			if (useZlib)
				theInputStream = new java.util.zip.InflaterInputStream(theInputStream);
			(new Thread_Reader(this, theInputStream)).start();

			OutputStream theOutputStream = theSocket.getOutputStream();
			if (useZlib)
				theOutputStream = new java.util.zip.DeflaterOutputStream(theOutputStream);

			// We must buffer the output otherwise we get bitten by the Nagle algorithm.
			theOutputStream = new BufferedOutputStream(theOutputStream, 2048);

			(new Thread_Writer(this, theOutputStream)).start();
			}
		catch (IOException e)
			{
			System.out.println(e);
			System.exit(-1);
			}
		}

	static private class Thread_Reader extends Thread
		{
		Thread_Reader(ZTB_Client_Net iClient, InputStream iStream)
			{
			fClient = iClient;
			fStream = iStream;
			}

		public void run()
			{
			try
				{
				fClient.runReader(fStream);
				}
			catch(Throwable t)
				{
				t.printStackTrace();
				}
			}
		
		final ZTB_Client_Net fClient;
		final InputStream fStream;
		};

	static private class Thread_Writer extends Thread
		{
		Thread_Writer(ZTB_Client_Net iClient, OutputStream iStream)
			{
			fClient = iClient;
			fStream = iStream;
			}
		
		public void run()
			{
			try
				{
				fClient.runWriter(fStream);
				}
			catch(Throwable t)
				{
				t.printStackTrace();
				}
			}

		final ZTB_Client_Net fClient;
		final OutputStream fStream;
		};
	};
