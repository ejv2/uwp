/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <wchar.h>

#include "json.h"
#include "conf.h"
#include "wp.h"

int
main(int argc, char **argv)
{
	WP wp;
	Site s;
	SiteList *l;
	char *res;
	WPResponse resp;

	if (argc < 3) {
		fprintf(stderr, "%s: expected site name and endpoint\n", argv[0]);
		return 1;
	}

	setlocale(LC_ALL, "");
	l = sites_load();
	if (!site_arg(&s, l, argv[1]))
		return 1;
	if (argv[2][0] != '/') {
		fprintf(stderr, "%s: expected endpoint to begin with '/'\n", argv[0]);
		return 1;
	}
	if (!wp_init(&wp, &s))
		return 1;

	resp = wp_request(&wp, argv[2]);
	if (resp.success < 0) {
		fprintf(stderr, "%s: %s: request failed\n", argv[0], argv[2]);
		return 2;
	}
	res = json_write_pretty(resp.parse, "\t", "\n", NULL);
	if (!res) {
		fprintf(stderr, "%s: %s: invalid response\n", argv[0], argv[2]);
		return 2;
	}
	printf("%s\n", res);
	free(res);
	free(resp.parse);
}
