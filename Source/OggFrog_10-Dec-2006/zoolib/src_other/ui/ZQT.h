/*  @(#) $Id: ZQT.h,v 1.4 2002/12/01 21:25:46 agreen Exp $ */

#ifndef __ZQT__
#define __ZQT__ 1
#include "zconfig.h"

#ifndef ZCONFIG_QT_Available
#	if ZCONFIG(OS, Win32)
#		define ZCONFIG_QT_Available 1
#		include <QTML.h>
#		include <Movies.h>
#	elif ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#		define ZCONFIG_QT_Available 1
#		include <Movies.h>
#	else
#		define ZCONFIG_QT_Available 0
#	endif
#endif

#endif // __ZQT__

