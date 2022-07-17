/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "wp.h"

/*
 * uwp-test: test if an installation is responding to REST API requests
 */
int
main(int argc, char **argv)
{
	WP wp;
	WPResponse resp;

	setlocale(LC_ALL, "");
	if (argc < 2) {
		fprintf(stderr, "%s: expected site name or URL\n", argv[0]);
		return 1;
	}

	wp_init(&wp, argv[1]);
	resp = wp_request(&wp, "/posts/");

	if (resp.success < 0) {
		const char *reason;
		switch (resp.success) {
		case -1:
			reason = "connection error";
			break;
		case -2:
			reason = "malformed JSON response";
			break;
		default:
			reason = "unknown error";
			break;
		}

		fprintf(stderr, "%s: could not get '/posts/' endpoint: %s\n",
			argv[0], reason);

		/* NOTE: intentionally not freeing memory, as we do not know how far it was allocated */
		return 1;
	}

	free(resp.parse);
	wp_destroy(&wp);
	return 0;
}
