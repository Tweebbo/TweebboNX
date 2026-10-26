/* English-only fixtures isolate their feature; dictionary/font integration is
 * covered by the separate language contract and host language tests. */
#include "language.h"
const char *tw_translate(const char *text) { return text ? text : ""; }
int tw_language(void) { return TW_EN; }
const char *tw_language_code(void) { return "en"; }
const char *tw_reception_language(void) { return "en"; }
const char *tw_reception_slug(void) { return "lobby"; }
