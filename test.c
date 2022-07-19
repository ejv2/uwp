/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "json.h"
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
	const char *errst;
	struct json_object_s *err;
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
	resp = wp_request(&wp, "/users?context=edit");

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

		fprintf(stderr, "uwp: could not get '/users/' endpoint: %s\n",
			reason);

		/* NOTE: intentionally not freeing memory, as we do not know how far it was allocated */
		return 1;
	}

	if (!resp.parse) {
		fprintf(stderr,
			"uwp: could not get '/users/' endpoint: malformed JSON response\n");
		return 1;
	}

	if (resp.parse->type == json_type_object) {
		err = (struct json_object_s *)resp.parse->payload;
		if ((errst = wp_check_errors(err))) {
			fprintf(stderr,
				"uwp: could not get '/users/' endpoint: %s\n",
				errst);
			return 1;
		}
	}

	free(resp.parse);
	sites_unload(l);
	wp_destroy(&wp);
	return 0;
}
