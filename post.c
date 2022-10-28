/* See LICENSE for copyright and license details */

#include <curl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <md4c-html.h>

#include "util.h"
#include "arg.h"
#include "json.h"
#include "conf.h"
#include "config.h"
#include "wp.h"

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

struct opt_struct {
	const char *longname;
	char shorthand;
	const char *help;
};
const struct opt_struct opt_help = {
	.longname = "help",
	.shorthand = 'h',
	.help = "This message"
};
const struct opt_struct opt_quit = {
	.longname = "quit",
	.shorthand = 'q',
	.help = "Exit now"
};

struct post_struct {
	char *title, *slug;
	char *raw_content, *content;
	WPPostType class;

	char *categories[5];
	char *tags[20];
} post;
struct post_ref {
	char **buf;
	size_t buflen;
};
char *argv0;
int usecol = 1;

void
usage()
{
	fprintf(stderr, "%s: usage\n", argv0);
	fprintf(stderr, "-f:\tuse file for content\n");
	fprintf(stderr, "-e:\tskip editing\n");
	fprintf(stderr, "-n:\tpost a page, not a post\n");
	fprintf(stderr, "-t:\tpost title (default: empty)\n");
	fprintf(stderr, "-n:\tpost slug (default: autogenerate)\n");
	fprintf(stderr, "-u:\tthis message\n");
	exit(1);
}

static void
__mdprocess(const MD_CHAR *in, MD_SIZE len, void *ptr)
{
	struct post_ref *ref = ptr;
	*ref->buf = realloc(*ref->buf, sizeof(MD_CHAR) * (ref->buflen + len));
	memcpy(*ref->buf + ref->buflen, in, sizeof(MD_CHAR) * len);
	ref->buflen += sizeof(MD_CHAR) * len;
}

int
load_post_file(struct post_struct *p, const char *path)
{
	const int chunk = 70;
	int chunks = 1, read = 0, thisread;
	FILE *f;
	struct post_ref ref = (struct post_ref){
		.buf = &p->content,
		.buflen = 0
	};

	if (!(f = fopen(path, "r")))
		return 0;

	p->raw_content = calloc(sizeof(char), chunk + 1);
	while ((thisread = fread(p->raw_content + read, sizeof(char), chunk - (read % chunk), f))) {
		read += thisread;
		if (read == chunks * chunk) {
			p->raw_content = realloc(p->raw_content, (++chunks) * chunk + 1);
		}
	}
	p->raw_content = realloc(p->raw_content, read + 1);
	p->raw_content[read] = 0;

	p->content = NULL;
	if (md_html(p->raw_content, read, &__mdprocess, &ref, MD_FLAG_UNDERLINE,
		    MD_HTML_FLAG_SKIP_UTF8_BOM |
			    MD_HTML_FLAG_VERBATIM_ENTITIES) < 0) {
		fprintf(stderr, "%s: warning: markdown format error\n", argv0);
		return 0;
	}

	fclose(f);
	return 1;
}

int
spawn_editor(const char *editor, char **file)
{
	char *f;
	int hndl, epid, stat;
	char tmpl[] = "/tmp/uwp-XXXXXX";

	if (*file) {
		f = *file;
	} else {
		if ((hndl = mkstemp(tmpl)) < 0) {
			perror("mkstemp");
			return 1;
		}
		f = *file = strdup(tmpl);

		write(hndl, new_template, LENGTH(new_template)-1);
		close(hndl);
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

void
print_post(struct post_struct p)
{
	const char *class;

	if (p.title) { printf("Title: %s\n", p.title); }
	else         { printf("Title: (empty)\n"); }

	if (p.slug)  { printf("Slug: %s\n", p.slug); }
	else         { printf("Slug: (empty)\n"); }

	printf("Cagegories: ");
	for (int i = 0; p.categories[i] && i < LENGTH(p.categories); i++) {
		printf("'%s' ", p.categories[i]);
	}
	putchar('\n');

	printf("Tags: ");
	for (int i = 0; p.tags[i] && i < LENGTH(p.tags); i++) {
		printf("'%s' ", p.tags[i]);
	}
	putchar('\n');

	switch (p.class) {
	case Post:
		class = "post";
		break;
	case Page:
		class = "page";
		break;
	default:
		class = "misc";
		break;
	}
	printf("Type: %s\n\n", class);
}

void
copy_contents(const char *contents, const char *out_path)
{
	FILE *f = fopen(out_path, "w");
	if (!f)
		return;

	fputs(contents, f);
	fclose(f);
}

void
menu_help(const struct opt_struct *opts)
{
}

struct opt_struct
menu_prompt(const char *prompt, struct opt_struct *opts, int *error)
{
	int counter = 1;
	char buf[BUFSIZ];
	char *hl, *uhl;

	if (!prompt)
		prompt = "What now";
	hl = (usecol) ? COLOR_BOLD COLOR_BLUE : "";
	uhl = (usecol) ? COLOR_RESET : "";

	puts("*** Commands ***");
	for (struct opt_struct *cur = opts; cur->longname; cur++) {
		printf("%d: %s%c%s%s       ", counter, hl, cur->shorthand, uhl, cur->longname + 1);
		if (counter % 4 == 0)
			putchar('\n');
		counter++;
	}
	putchar('\n');
	printf("%s%s%s> ", hl, prompt, uhl);
	if (!fgets(buf, BUFSIZ, stdin)) {
		*error = 1;
		return (struct opt_struct){};
	}
	strip_newline(buf);

	for (struct opt_struct *cur = opts; cur->longname; cur++) {
		if (tolower(buf[0]) == cur->shorthand || strcmp(buf, cur->longname) == 0) {
			return *cur;
		}
	}

	fprintf(stderr, "Unknown option: '%s'\n\n", buf);
	return (struct opt_struct){};
}

int
confirm_prompt(const char *prompt)
{
	int ret;
	char c;

	if (!prompt)
		prompt = "Please confirm [Y/N]: ";

	printf("%s", prompt);
	for (;;) {
		c = tolower(getchar());
		switch (c) {
		case 'y':
			ret = 1;
			goto out;
		case 'n':
			ret = 0;
			goto out;
		default:
			fprintf(stderr, "%s: unexpected response '%c'\n", argv0, c);
			break;
		}
	}

out:
	/* Drain stdin to prevent reads by other menus */
	do {
		c = getchar();
	} while (c != '\n');
	return ret;
}

int
menu_main(struct post_struct *p, char *file, const char *editor)
{
	int quit = 0;
	const char *trail;
	char fbuf[PATH_MAX];
	struct opt_struct choice;
	struct opt_struct opts[] = {
		{
			.longname = "publish",
			.shorthand = 'p',
			.help = "Confirm and publish"
		},
		{
			.longname = "draft",
			.shorthand = 'd',
			.help = "Confirm and publish as draft",
		},
		{
			.longname = "save",
			.shorthand = 's',
			.help = "Save markdown locally",
		},
		{
			.longname = "change details",
			.shorthand = 'c',
			.help = "Go back and edit",
		},
		{
			.longname = "edit",
			.shorthand = 'e',
			.help = "Go back and edit",
		},
		opt_help,
		opt_quit,
		NULL,
	};

	if (!p)
		return 1;

	if (p->title)
		trail = p->title;
	else if (!(trail = strrchr(file, '/')))
		trail = file;
	else
		trail++;	/* slice off forward slash */

	print_post(*p);
	choice = menu_prompt("Action", opts, &quit);
	if (quit)
		return quit;

	switch (choice.shorthand) {
	case 'p':
		/* TODO: Publish */
		fputs("Not implemented", stderr);
		break;
	case 'd':
		/* TODO: Publish as draft */
		fputs("Not implemented", stderr);
		break;
	case 's':
		snprintf(fbuf, PATH_MAX, "./%s", trail);
		copy_contents(p->raw_content, fbuf);
		printf("draft saved to %s\n", fbuf);
		break;
	case 'c':
		/* TODO: Change details */
		fputs("Not implemented", stderr);
		break;
	case 'e':
		spawn_editor(editor, &file);
		load_post_file(p, file);
		break;
	case 'q':
		quit = 1;
		break;
	}
	putchar('\n');

	return quit;
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
	int stat, done = 0;
	int eskip = 0;

	setlocale(LC_ALL, "");
	usecol = isatty(STDOUT_FILENO) && colors;
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
	case 'e':
		eskip = 0;
		break;
	case 't':
		post.title = strdup(EARGF(usage()));
		break;
	case 'p':
		post.class = Page;
		break;
	case 'n':
		post.slug = strdup(EARGF(usage()));
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
		if ((stat = spawn_editor(editor, &file))) {
			fprintf(stderr, "%s: post abort: editor returned %d\n", argv0,
				stat);
			return stat;
		}
	}
	if (!post.raw_content)
		load_post_file(&post, file);

	do {
		done = menu_main(&post, file, editor);
	} while (!done);

	free(post.title);
	free(post.slug);
	free(post.raw_content);
	free(post.content);
	if (!eskip)
		unlink(file);
	return 0;
}
