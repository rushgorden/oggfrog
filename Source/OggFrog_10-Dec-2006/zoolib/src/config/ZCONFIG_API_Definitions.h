/*  @(#) $Id: ZCONFIG_API_Definitions.h,v 1.1 2005/09/02 18:42:32 agreen Exp $ */

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

#ifndef __ZCONFIG_API_Definitions__
#define __ZCONFIG_API_Definitions__ 1
#include "zconfig.h"

#define ZCONFIG_API_Avail(facility) (ZCONFIG_API_Avail__##facility)
#define ZCONFIG_API_Desired(facility) (ZCONFIG_API_Desired__##facility)

#define ZCONFIG_API_Enabled(facility) (ZCONFIG_API_Avail(facility) && ZCONFIG_API_Desired(facility))


#endif // __ZCONFIG_API_Definitions__
