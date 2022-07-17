/* See LICENSE for copyright and license details. */

#define MAX(A, B)		((A) > (B) ? (A) : (B))
#define MIN(A, B)		((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)	((A) <= (X) && (X) <= (B))
#define LENGTH(X) 		(sizeof X / sizeof X[0])

static inline void
strip_newline(char *s)
{
	if (!s)
		return;

	for (;;) {
		if (*s == '\n') {
			*s = 0;
			break;
		}
		s++;
	}
}
