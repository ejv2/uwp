/* See LICENSE for copyright and license details */

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

#include "conf.h"
#include "config.h"
#include <string.h>

/*
 * uwp-sites: show all sites that uwp knows about along with a summary of
 * configuration for each. Passwords are never shown in plain text.
 */
int
main(int argc, char **argv)
{
	const Site *s;
	SiteList *sites, *cur;
	int pwlen;
	const char *path;

	setlocale(LC_ALL, "");
	sites = sites_load();

	puts("NAME\tURL\tUSERNAME\tPASSWORD");
	for (cur = sites; cur; cur = (SiteList *)cur->next) {
		s = cur->s;
		pwlen = strlen(s->pw);

		char censor[pwlen];
		if (site_pwstore(s) || site_pwask(s))
			strcpy(censor, s->pw);
		else {
			memset(censor, 'x', sizeof(char) * pwlen);
			censor[pwlen] = 0;
		}

		printf("%s\t%s\t%s\t%s\n", s->name, s->baseurl, s->usr, censor);
	}

	sites_unload(sites);
	return 0;
}
