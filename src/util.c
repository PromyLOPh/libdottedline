#include <stdio.h>
#include <stdint.h>

void XMC_AssertHandler(const char *const msg, const char *const file, uint32_t line) {
	char buf[256];
	snprintf (buf, sizeof (buf), "%s:%lu: %s\n", file, line, msg);
	puts (buf);

	while(1);
}
