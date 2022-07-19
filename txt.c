/* See LICENSE for copyright and license details. */

#include <string.h>

#include "txt.h"

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
