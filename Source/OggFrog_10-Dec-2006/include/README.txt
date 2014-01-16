Header files that need to be different depending on build configuration
go in the appropriate subdirectory here.  I prefer this to using
build-dependent precompiled headers, because I want to support
older development systems that don't support precompiled headers.

So far the only header here is zconfig.h (the one in
zoolib/src/config is renamed to hide it from the build).  The
three configurations have different levels of debugging support:

Debug:   ZCONFIG_Debug = 2, extensive assertions, no optimization
QA:      ZCONFIG_Debug = 1, moderate assertions, optimized, stripped
Release: ZCONFIG_Debug = 0, assertions disabled, optimized, stripped

I'll be adding a Profile config later.

Mike Crawford
aka Rippit the Ogg Frog
rippit@oggfrog.com
http://www.oggfrog.com/

