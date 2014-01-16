#ifndef __ZKeyboard__
#define __ZKeyboard__ 1
#include "zconfig.h"

namespace ZKeyboard {

typedef short CharCode;
typedef short KeyCode;

// Character codes. The character *generated* by a key press or combination of key presses
// Incomplete.
enum
	{
	ccUnknown = 0,
	ccBackspace = 8,
	ccClear = 27,
	ccDown = 31,
	ccEnd = 4,
	ccEnter = 3,
	ccEscape = 27,
	ccFunction = 16,
	ccFwdDelete = 127,
	ccHelp = 5,
	ccHome = 1,
	ccLeft = 28,
	ccPageDown = 12,
	ccPageUp = 11,
	ccReturn = 13,
	ccRight = 29,
	ccSpace = 32,
	ccTab = 9,
	ccUp = 30
	};
// Key codes. Symbolic names for the keys, so we can refer to non character-generating keys
// Clearly this is very incomplete.
enum
	{
	kcUnknown = 0
	};

} // namespace ZKeyboard

#endif // __ZKeyboard__
