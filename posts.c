/* See LICENSE for copyright and license details */

#include <curl.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "json.h"
#include "txt.h"
#include "util.h"
#include "arg.h"
#include "conf.h"
#include "wp.h"

char *argv0;

static void
usage()
{
	fprintf(stderr, "%s: usage\n", argv0);
	fprintf(stderr, "-p:\tshow pages, not posts\n");
	fprintf(stderr, "-i:\tshow this post/page ID\n");
	fprintf(stderr, "-e:\tshow page excerpt below details\n");
	fprintf(stderr, "-c:\tshow page content below details\n");
	fprintf(stderr,
		"-C:\tshow this many characters of content; zero to be unlimited (default: 70)\n");
	fprintf(stderr, "-n:\tshow this many results (default: 20)\n");
	fprintf(stderr, "-u:\tthis message\n");
	exit(1);
}

static void
post_print(WPPost p)
{
	char *excerpt = strdup(p.excerpt);
	strip_html(excerpt);

	printf("id='%ld'\n", p.id);
	printf("title='%s'\n", p.title);
	printf("excerpt='%s'\n", excerpt);
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

	free(excerpt);
}

static void
post_print_content(WPPost p, int showexcerpt, int showcont, int contchars)
{
	char *excerpt;
	char *cont, *csuffix;

	if (showexcerpt) {
		excerpt = strdup(p.excerpt);
		strip_html(excerpt);
		printf("\t%s\n", excerpt);
		free(excerpt);
	}
	if (showcont) {
		if (contchars) {
			cont = limit_string(p.content, contchars);
			csuffix = "...";
		} else {
			cont = strdup(p.content);
			csuffix = "";
		}
		strip_newline(cont);
		strip_html(cont);
		printf("%s%s\n----------\n", cont, csuffix);
		free(cont);
	}
}

static int
posts_list(WP *wp, int page, int results, int showexcerpt, int showcont,
	   int contchars)
{
	int ret = 1;
	WPResponse resp;
	WPPost tmp;
	struct json_array_s *arr;
	struct json_array_element_s *e;
	const char *eppref, *err;
	char ep[200];

	if (page)
		eppref = "/pages/";
	else
		eppref = "/posts/";
	snprintf(ep, 200, "%s?per_page=%d", eppref, results);

	resp = wp_request(wp, ep);
	if (resp.success < 0) {
		fprintf(stderr, "%s: posts: request failed\n", argv0);
		return 0;
	}
	if (resp.parse->type != json_type_array) {
		if (resp.parse->type == json_type_object) {
			err = wp_check_errors(
				(struct json_object_s *)resp.parse->payload);
			fprintf(stderr, "%s: posts: request error: %s\n", argv0,
				err);
		} else {
			fprintf(stderr, "%s: posts: malformed response\n",
				argv0);
		}
		ret = 0;
		goto out;
	}

	arr = (struct json_array_s *)resp.parse->payload;
	for (e = arr->start; e; e = e->next) {
		if (!wp_parse_post(&tmp, e->value)) {
			fprintf(stderr, "%s: posts: request error: %s\n", argv0,
				err);
			goto out;
		}

		printf("%ld\t%s\t%s\n", tmp.id, tmp.title, tmp.url);
		post_print_content(tmp, showexcerpt, showcont, contchars);
	}

out:
	free(resp.parse);
	return ret;
}

static int
posts_get(WP *wp, long id, int page, int showexcerpt, int showcont,
	  int contchars)
{
	int ret = 1;
	WPResponse resp;
	WPPost p;
	const char *eppref;
	char *cont, *csuffix;
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
	post_print_content(p, showexcerpt, showcont, contchars);

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
	int ret = 1;
	int res = 20, pages = 0, cont = 0, excerpt = 0, amcont = 70;
	long max = 10, id = -1;
	WP wp;
	SiteList *l;
	Site s;

	setlocale(LC_ALL, "");
	ARGBEGIN {
	case 'p':
		pages = 1;
		break;
	case 'e':
		excerpt = 1;
		break;
	case 'c':
		cont = 1;
		break;
	case 'C':
		amcont = strtoimax(EARGF(usage()), NULL, 10);
		break;
	case 'n':
		res = strtoimax(EARGF(usage()), NULL, 10);
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

	if (id > 0)
		ret = posts_get(&wp, id, pages, excerpt, cont, amcont);
	else
		ret = posts_list(&wp, pages, res, excerpt, cont, amcont);

	sites_unload(l);
	wp_destroy(&wp);
	return ret;
}
