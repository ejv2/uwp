/* See LICENSE for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "conf.h"
#include "util.h"
#include "config.h"

static void
walk_back(SiteList **list)
{
	while ((*list)->prev) {
		*list = (SiteList *)((*list)->prev);
	}
}

static const char *
pwstore(const Site *s)
{
	/* TODO */
	return strdup("");
}

static const char *
pwask(const Site *s)
{
	char *buf, *i;
	buf = malloc(sizeof(char) * BUFSIZ);

	fprintf(stderr, "Password for %s@%s:", s->usr, s->name);
	if (!fgets(buf, BUFSIZ, stdin)) {
		perror("password input");
		return NULL;
	}

	/* Strip newline */
	i = buf;
	for (;;) {
		if (*i == 0) {
			*(--i) = 0;
			break;
		}
	}

	return strdup(buf);
}

static void
parse_config(SiteList *l)
{
	FILE *f;
	int field;
	char cwd[PATH_MAX];
	char buf[BUFSIZ];
	char *sep;
	Site *s;
	SiteList *nl;

	if (!getcwd(cwd, PATH_MAX)) {
		return;
	}

	chdir(getenv("HOME"));
	f = fopen(extra_sites, "r");
	if (!f) {
		chdir(cwd);
		return;
	}

	while (fgets(buf, BUFSIZ, f)) {
		field = 1;
		nl = malloc(sizeof(SiteList));
		s = malloc(sizeof(Site));
		nl->prev = (struct SiteList *)l;
		nl->next = NULL;
		nl->s = s;
		for (sep = strtok(buf, "\t"); sep; sep = strtok(NULL, "\t")) {
			switch (field++) {
			case 1:
				s->name = strdup(sep);
				break;
			case 2:
				s->baseurl = strdup(sep);
				break;
			case 3:
				s->usr = strdup(sep);
				break;
			case 4:
				s->pw = strdup(sep);
				break;
			case 5:
				goto insert;
				break;
			}
		}
insert:
		if (field == 5) {
			l->next = (struct SiteList *)nl;
			l = nl;
		}
	}
	chdir(cwd);
}

const char *
site_pw(const Site *s)
{
	if (!s)
		return NULL;

	if (site_pwstore(s)) {
		return pwstore(s);
	} else if (site_pwask(s)) {
		return pwask(s);
	}

	return strdup(s->pw);
}

int
site_pwstore(const Site *s)
{
	return strncmp(pwstore_prefix, s->pw, LENGTH(pwstore_prefix) - 1) == 0;
}

int
site_pwask(const Site *s)
{
	return strcmp(ask_password, s->pw) == 0;
}

SiteList *
sites_load()
{
	int i;
	SiteList *root = NULL;
	SiteList *cur = NULL, *prev;
	const char *path = extra_sites;

	/* first load up in-memory config array */
	for (i = 0; i < LENGTH(sites); i++) {
		prev = cur;
		cur = malloc(sizeof(SiteList));
		cur->s = &sites[i];
		cur->next = NULL, cur->prev = (struct SiteList *)prev;
		if (prev)
			prev->next = (struct SiteList *)cur;
		else
			root = cur;
	}

	/* now load up the extra sites from disk */
	parse_config(cur);

	return root;
}

void
sites_unload(SiteList *list)
{
	SiteList *cur, *bu;

	if (!list)
		return;

	cur = list;
	walk_back(&cur);
	do {
		bu = (SiteList *)cur->next;
		free(cur);
		cur = bu;
	} while (bu);
}
