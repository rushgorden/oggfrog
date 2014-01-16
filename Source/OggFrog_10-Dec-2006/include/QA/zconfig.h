#ifndef __zconfig__
#define __zconfig__ 1

#if defined __APPLE__
#include <CarbonCore/ConditionalMacros.h>
#endif

// QA builds are sent to end-user testers with a modest level of
// debugging.

#define ZCONFIG_Debug 1

#include "zconfigd.h"
#include "zconfigl.h"

#endif // __zconfig__
