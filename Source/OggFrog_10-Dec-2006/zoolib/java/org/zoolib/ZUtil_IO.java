// "@(#) $Id: ZUtil_IO.java,v 1.1 2006/10/22 13:17:39 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

package org.zoolib;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

public final class ZUtil_IO
	{
	private ZUtil_IO() {}

	public static final int sReadCount(DataInput iDI) throws IOException
		{
		int count = iDI.readUnsignedByte();
		if (count == 255)
			count = iDI.readInt();
		return count;
		}

	public static void sWriteCount(DataOutput iDO, int iCount) throws IOException
		{
		if (iCount < 255)
			{
			iDO.writeByte(iCount);
			}
		else
			{
			iDO.writeByte(255);
			iDO.writeInt(iCount);
			}
		}

	public static final void sWriteString(DataOutput iDO, String iString) throws IOException
		{
		byte byteBuffer[] = sUTF8FromString(iString);
		sWriteCount(iDO, byteBuffer.length);
		iDO.write(byteBuffer);
		}	

	private static final String sReadUTF8(DataInput iDI, int iCount) throws IOException
		{
		byte byteBuffer[] = new byte[iCount];
		iDI.readFully(byteBuffer);
		return sStringFromUTF8(byteBuffer);
		}

	public static final String sReadString(DataInput iDI) throws IOException
		{
		int stringLength = ZUtil_IO.sReadCount(iDI);
		return ZUtil_IO.sReadUTF8(iDI, stringLength);	
		}

	private static final String sStringFromUTF8(byte iBytes[])
		{
		int sourceCount = iBytes.length;
		int sourceIndex = 0;

		// We can't have more UTF16 code units than we have UTF8 code units,
		// so making destBuffer sourceCount chars in length ensures there will
		// be enough space.
		char destBuffer[] = new char[sourceCount];
		int destIndex = 0;

		for (;;)
			{
			if (sourceIndex == sourceCount)
				break;

			int byte1 = iBytes[sourceIndex++] & 0xFF;

			// look at the first four bits of byte1 to determine how many
			// bytes in this char
			int test = byte1 >> 4;
			if (test < 8)
				{
				// Standalone
				destBuffer[destIndex++] = (char) byte1;
				}
			else
				{
				if (sourceIndex == sourceCount)
					break;
				int byte2 = iBytes[sourceIndex++];
				if (test == 12 || test == 13)
					{
					// two bytes
					destBuffer[destIndex++] = (char) (((byte1 & 0x1F) << 6) | (byte2 & 0x3F));
					}
				else if (test == 14)
					{
					// three bytes
					if (sourceIndex == sourceCount)
						break;
					int byte3 = iBytes[sourceIndex++];
					destBuffer[destIndex++] = (char)(((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
					}
				else
					{
					// No more than three bytes are required to represent
					// code points in the range 0 - 0x10FFFF.
					// malformed.
					break;
					}
				}
			}

		return new String(destBuffer, 0, destIndex);
		}

	public static final byte[] sUTF8FromString(String iString)
		{
		int sourceCount = iString.length();
		int destCount = 0;
		for (int sourceIndex = 0; sourceIndex < sourceCount; ++sourceIndex)
			{
			int c = iString.charAt(sourceIndex);
			if ((c >= 0x0000) && (c <= 0x007F))
				++destCount;
			else if (c > 0x07FF)
				destCount += 3;
			else
				destCount += 2;
			}

		byte destBuffer[] = new byte[destCount];
		for (int sourceIndex = 0, destIndex = 0; sourceIndex < sourceCount; ++sourceIndex)
			{
			int c = iString.charAt(sourceIndex);
			if ((c >= 0x0000) && (c <= 0x007F))
				{
				destBuffer[destIndex++] = (byte)c;
				}
			else if (c > 0x07FF)
				{
				destBuffer[destIndex++] = (byte)(0xE0 | ((c >> 12) & 0x0F));
				destBuffer[destIndex++] = (byte)(0x80 | ((c >> 6) & 0x3F));
				destBuffer[destIndex++] = (byte)(0x80 | (c & 0x3F));
				}
			else
				{
				destBuffer[destIndex++] = (byte)(0xC0 | ((c >> 6) & 0x1F));
				destBuffer[destIndex++] = (byte)(0x80 | (c & 0x3F));
				}
			}
		return destBuffer;
		}
	}
