// "@(#) $Id: ZTB_Client_Stream.java,v 1.1 2005/05/09 06:00:45 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2005 Andrew Green and Learning in Motion, Inc.
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

import java.io.InputStream;
import java.io.OutputStream;

// ====================================================================================================

public class ZTB_Client_Stream extends ZTB_Client
	{
	public ZTB_Client_Stream(InputStream iInputStream, OutputStream iOutputStream)
		{
		(new Thread_Reader(this, iInputStream)).start();
		(new Thread_Writer(this, iOutputStream)).start();
		}

	static private class Thread_Reader extends Thread
		{
		Thread_Reader(ZTB_Client_Stream iTB_Client_Stream, InputStream iStream)
			{
			fTB_Client_Stream = iTB_Client_Stream;
			fStream = iStream;
			}

		public void run()
			{
			try
				{
				fTB_Client_Stream.runReader(fStream);
				}
			catch(Throwable t)
				{
				t.printStackTrace();
				}
			}
		
		final ZTB_Client_Stream fTB_Client_Stream;
		final InputStream fStream;
		};

	static private class Thread_Writer extends Thread
		{
		Thread_Writer(ZTB_Client_Stream iTB_Client_Stream, OutputStream iStream)
			{
			fTB_Client_Stream = iTB_Client_Stream;
			fStream = iStream;
			}
		
		public void run()
			{
			try
				{
				fTB_Client_Stream.runWriter(fStream);
				}
			catch(Throwable t)
				{
				t.printStackTrace();
				}
			}

		final ZTB_Client_Stream fTB_Client_Stream;
		final OutputStream fStream;
		};
	};
