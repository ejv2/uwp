/* See LICENSE for copyright and license details */

#include <curl.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "arg.h"
#include "conf.h"
#include "wp.h"
#include <string.h>

char *argv0;

static void
usage()
{
	fprintf(stderr, "%s: usage\n", argv0);
	fprintf(stderr, "-p:\tshow pages, not posts");
	fprintf(stderr, "-i:\tshow this post/page ID");
	exit(1);
}

static void
post_print(WPPost p)
{
	printf("id='%ld'\n", p.id);
	printf("title='%s'\n", p.title);
	printf("excerpt='%s'\n", p.excerpt);
	printf("url='%s'\n", p.url);
	printf("date='%s'\n", p.date);
	printf("modified='%s'\n", p.modified);
	switch (p.type) {
		case Post:
			puts("type='post'");
			break;
		case Page:
			puts("type='page'");
			break;
		default:
			puts("type='unknown'");
			break;
	}
}

static int
posts_list()
{
	return 0;
}

static int
posts_get(WP *wp, long id, int page)
{
	int ret = 1;
	WPResponse resp;
	WPPost p;
	const char *eppref;
	char ep[200];

	if (page)
		eppref = "/pages/";
	else
		eppref = "/posts/";

	snprintf(ep, 200, "%s%ld", eppref, id);
	resp = wp_request(wp, ep);

	if (resp.success < 0 || !wp_parse_post(&p, resp.parse)) {
		fprintf(stderr, "%s: post %ld: not found\n", argv0, id);
		goto out;
	}

	post_print(p);
	ret = 0;
out:
	free(resp.parse);
	return ret;
}

/*
 * uwp-posts: view and manipulate posts on a wordpress site
 *
 * NOTE: When uwp refers to 'posts', it is actually referring to both pages and
 * posts, which can be interacted with in exactly the same way by tools that
 * work with either. Simply pass '-p' to have the tool work with pages, not posts.
 */
int
main(int argc, char **argv)
{
	int ret = 1, pages = 0;
	long max = 10, id = -1;
	WP wp;
	SiteList *l;
	Site s;

	setlocale(LC_ALL, "");
	ARGBEGIN {
		case 'p':
			pages = 1;
			break;
		case 'i':
			id = strtol(EARGF(usage()), NULL, 10);
			if (!id || errno != 0) {
				fprintf(stderr, "%s: invalid ID\n", argv0);
				return 1;
			}
			break;
		case 'u':
			usage();
			/* NOTREACHED */
		default:
			fprintf(stderr, "%s: unknown flag '%c'\n", argv0, ARGC());
			return 1;
	} ARGEND
	if (argc < 1) {
		fprintf(stderr, "%s: expected site name\n", argv0);
		return 1;
	}

	l = sites_load();
	if (!site_arg(&s, l, argv[0])) {
		sites_unload(l);
		return 1;
	}
	wp_init(&wp, &s);
	wp_auth(&wp);

	if (id > 0)
		ret = posts_get(&wp, id, pages);
	else
		ret = posts_list();

	sites_unload(l);
	return ret;
}
