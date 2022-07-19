/* See LICENSE for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#ifdef __OpenBSD__
#include <readpassphrase.h>
#else
#include <bsd/readpassphrase.h>
#endif

#include "util.h"
#include "conf.h"
#include "config.h"

static void
walk_back(SiteList **list)
{
	while ((*list)->prev) {
		*list = (SiteList *)((*list)->prev);
	}
}

static char *
pwstore(const Site *s)
{
	FILE *proc;
	char *pw, *pwname;
	char cmd[LENGTH(passcmd) + strlen(s->pw) + 3];

	pwname = strchr(s->pw, ':') + 1;
	sprintf(cmd, "%s '%s'", passcmd, pwname);
	proc = popen(cmd, "r");
	if (!proc)
		return NULL;

	pw = malloc(sizeof(char) * BUFSIZ);
	if (!pw) {
		goto cleanup;
	}

	if (!fgets(pw, BUFSIZ, proc)) {
		free(pw);
		pw = NULL;
		goto cleanup;
	}
	strip_newline(pw);

cleanup:
	pclose(proc);
	return pw;
}

static char *
pwask(const Site *s)
{
	char *pw;
	char prompt[1024];

	sprintf(prompt, "Password for %s@%s:", s->usr, s->name);
	pw = malloc(sizeof(char) * BUFSIZ);
	readpassphrase(prompt, pw, BUFSIZ, RPP_ECHO_OFF | RPP_REQUIRE_TTY);

	/* Not handling NULL error return, as pwask returns NULL on error anyway */
	return pw;
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

		buf[strlen(buf) - 1] = 0;
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

char *
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

const Site *
site_lookup(SiteList *list, const char *name)
{
	SiteList *cur = list;
	while (cur) {
		if (strcmp(cur->s->name, name) == 0)
			return cur->s;

		cur = (SiteList *)cur->next;
	}

	return NULL;
}

int
site_arg(Site *dst, SiteList *l, const char *arg)
{
	const Site *tmp;
	Site s;
	char *user = malloc(sizeof(char) * BUFSIZ);

	if (!dst || !arg)
		return 0;

	if (strchr(arg, '/')) {
		s = (Site){
			.name = arg, .baseurl = arg, .usr = user, .pw = "ask"
		};

		fprintf(stderr, "Username:");
		if (!fgets(user, BUFSIZ, stdin)) {
			return 0;
		}
		strip_newline(user);
	} else {
		if (!(tmp = site_lookup(l, arg))) {
			fprintf(stderr, "uwp: %s: site not found\n", arg);
			return 0;
		}

		s = *tmp;
		free(user);
		tmp = NULL;
	}

	*dst = s;
	return 1;
}
