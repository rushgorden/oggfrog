#pragma precompile_target "Mad_prefix"

#define FPM_64BIT

#if 0
#if defined( i386 ) || defined( __INTEL__ )

#define FPM_INTEL

#elif defined( __POWERPC__ )

#define FPM_PPC

#else

#error What's your CPU?

#endif
#endif