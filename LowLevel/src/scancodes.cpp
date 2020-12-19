// (C) 2016 Tobias Girstmair, released under the GNU GPL
#include <string.h>
#include "scancodes.hpp"

scancondes::scancondes(){


}

keysym* scancondes::toscan (const char* utf8) {
	if (utf8[1] == '\0') return &(keys[(int)utf8[0]]); //if char is not wide, it is on the corrent ascii position in the look up table
	for (int i = 0; i < sizeof (keys)/sizeof (keysym); i++) {
		if (strcmp (keys[i].sym, utf8) == 0) {
			return &(keys[i]);
		}
	}
	return NULL; // error
}

layout* scancondes::tolay (keysym* s, kbdl layout) {
        switch (layout) {
        case en_US: return &(s->en_us);
        case de_AT: return &(s->de_at);
        case de_ND: return &(s->de_nd);
        default: return NULL;
        }
}
