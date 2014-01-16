#include "zconfig.h"

// the actual and base types can't be anything but a single token, with
// no operators.

#define OFAssertValidDynamicCast( ptr, actual, base ) \
	ZAssertStop( ZCONFIG_Debug,  ( ( nil != (ptr) ) && \
		( nil != dynamic_cast< actual* >( static_cast< base* >( (ptr ) ) ) ) ) )
