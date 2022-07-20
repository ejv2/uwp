/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <md4c-html.h>

#include "util.h"
#include "arg.h"
#include "json.h"
#include "conf.h"
#include "config.h"
#include "wp.h"


struct post_struct {
	char *title, *slug;
	char *raw_content, *content;

	char *categories[5];
	char *tags[20];
} post;
char *argv0;

void
usage()
{
	fprintf(stderr, "%s: usage\n", argv0);

	exit(1);
}

int
load_post_file(struct post_struct *p, const char *path)
{
	const int chunk = 70;
	int chunks = 1, read = 0, thisread;
	FILE *f;

	if (!(f = fopen(path, "r")))
		return 0;

	p->raw_content = calloc(sizeof(char), chunk+1);
	while ((thisread = fread(p->raw_content+read, sizeof(char), chunk - (read % chunk), f))) {
		read += thisread;
		if (read == chunks*chunk) {
			p->raw_content = realloc(p->raw_content, (++chunks)*chunk+1);
		}
	}
	p->raw_content = realloc(p->raw_content, read+1);
	p->raw_content[read] = 0;

	fclose(f);
	return 1;
}

int
spawn_editor(const char *editor, char *file)
{
	char *f;
	int hndl, epid, stat;
	char tmpl[] = "/tmp/uwp-XXXXXX";

	if (file) {
		f = strdup(file);
	} else {
		if ((hndl = mkstemp(tmpl)) < 0) {
			perror("mkstemp");
			return 1;
		}
		f = tmpl;

		write(hndl, new_template, LENGTH(new_template)-1);
	}

	epid = fork();
	switch (epid) {
	case 0:
		execlp(editor, editor, f, NULL);
		exit(1);
		/* NOTREACHED */
	case -1:
		perror("editor fork");
		return 1;
	default:
		waitpid(epid, &stat, 0);
		break;
	}

	return stat;
}

/*
 * uwp-post: compose and publish a post from your command line
 */
int
main(int argc, char **argv)
{
	WP wp;
	Site s;
	SiteList *l;
	char *editor, *file = NULL;
	int stat;
	int eskip = 0;

	setlocale(LC_ALL, "");
	memset(&post, 0, sizeof(struct post_struct));

	ARGBEGIN {
	case 'f':
		file = EARGF(usage());
		if (!load_post_file(&post, file)) {
			fprintf(stderr, "%s: invalid post file", argv0);
			return 1;
		}
		eskip = 1;
		break;
	case 't':
		post.title = EARGF(usage());
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

	if (!(editor = getenv("EDITOR"))) {
		editor = "vi";
		fprintf(stderr,
			"%s: warning: EDITOR not set, defaulting to vi\n",
			argv0);
	}

	l = sites_load();
	if (!site_arg(&s, l, argv[0])) {
		return 1;
	}

	if (!eskip) {
		if ((stat = spawn_editor(editor, file))) {
			fprintf(stderr, "%s: post abort: editor returned %d\n", argv0,
				stat);
		}
	}

	free(post.raw_content);
	free(post.content);
	return 0;
}
