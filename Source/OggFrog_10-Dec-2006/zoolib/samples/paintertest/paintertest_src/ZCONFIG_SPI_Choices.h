/*  @(#) $Id: ZCONFIG_SPI_Choices.h,v 1.3 2006/07/17 18:29:56 agreen Exp $ */

#ifndef __ZCONFIG_SPI_Choices__
#define __ZCONFIG_SPI_Choices__ 1
#include "zconfig.h"

#if !ZCONFIG(OS, POSIX)
#define ZCONFIG_SPI_Avail__JPEGLib 1
#define ZCONFIG_SPI_Avail__libpng 1
#define ZCONFIG_SPI_Avail__zlib 1
#endif

#endif // __ZCONFIG_SPI_Choices__
