/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "conf.h"
#include "wp.h"
#include <string.h>

/*
 * uwp-test: test if an installation is responding to REST API requests and
 * accepts authentication as configured
 */
int
main(int argc, char **argv)
{
	SiteList *l;
	const Site *tmp;
	Site s;
	WP wp;
	WPResponse resp;
	char user[BUFSIZ];

	setlocale(LC_ALL, "");
	if (argc < 2) {
		fprintf(stderr, "%s: expected site name or URL\n", argv[0]);
		return 1;
	}

	l = sites_load();
	if (!site_arg(&s, l, argv[1]))
		return 1;

	wp_init(&wp, &s);
	if (!wp_auth(&wp))
		return 1;

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

		fprintf(stderr, "uwp: could not get '/posts/' endpoint: %s\n",
			reason);

		/* NOTE: intentionally not freeing memory, as we do not know how far it was allocated */
		return 1;
	}

	free(resp.parse);
	sites_unload(l);
	wp_destroy(&wp);
	return 0;
}
