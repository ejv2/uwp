/* See LICENSE for copyright and license details. */

#include <string.h>
#include "txt.h"

static const char html_open = '<';
static const char html_close = '>';

char *
limit_string(const char *orig, long max)
{
	char *dup, *inc;
	long count;

	for (dup = inc = strdup(orig), count = 0; *inc && count < max;
	     inc++, count++) {
	}

	*inc = 0;
	return dup;
}

void strip_html(char *buf)
{
	char *endptr;
	char *ptr, *hbegin, *hend;

	if (!buf)
		return;

	for (endptr = buf; *endptr; endptr++);
	for (ptr = buf; *ptr; ptr++) {
		if (*ptr == html_open)
			hbegin = ptr;

		if (*ptr == html_close) {
			hend = ptr;
			memmove(hbegin, hend+1, endptr-(hend)+1);
			ptr = hbegin-1;
		}
	}
}
