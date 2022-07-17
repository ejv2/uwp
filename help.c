/* See LICENSE for copyright and license details.
 *
 * Micro WordPress (uwp) is a command line WordPress client. It uses the
 * default WordPress REST API to manipulate the site from your local machine,
 * including convenience, such as creating posts with markdown and then
 * converting to the block editor for editing in the browser.
 *
 * The best way to understand everything is to read the main() of each tool.
 */

#include <stdio.h>
#include <string.h>

static const char *help =
	"uwp - micro wordpress\n"
	"Micro WordPress (uwp) is a command line WordPress client. It uses the\n"
	"default WordPress REST API to manipulate the site from your local machine,\n"
	"including convenience, such as creating posts with markdown and then \n"
	"converting to the block editor for editing in the browser. \n"
	"\n"
	"Micro WordPress requires priviledged access, and therefore requires \n"
	"credentials. These can be stored in UNIX Pass file format (please see \n"
	"https://www.passwordstore.org/), or just in the config file (not \n"
	"recommended).";

static const char *credits =
	"uwp - micro wordpress\n"
	"Copyright (C) 2022 - Ethan Marshall\n"
	"\n"
	"uwp was written by and is maintained entirely by Ethan Marshall as a\n"
	"hobby project";

static const char *license =
	"uwp - micro wordpress\n"
	"Copyright (C) 2022 - Ethan Marshall\n"
	"\n"
	"This program is free software: you can redistribute it and/or modify\n"
	"it under the terms of the GNU General Public License as published by\n"
	"the Free Software Foundation, either version 3 of the License, or\n"
	"(at your option) any later version";

int
main(int argc, char **argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "credits") == 0)
			puts(credits);
		else if (strcmp(argv[1], "license") == 0)
			puts(license);
		else {
			fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
				argv[1]);
			return 1;
		}
	} else {
		puts(help);
	}

	return 0;
}
