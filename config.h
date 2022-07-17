/* See LICENSE for copyright and license details. */

/* Built-in sites
 *
 * uwp supports more sites being loaded from .config/uwp/sites as a CSV file in
 * this same format. If the file does not exist, this is used exclusively. You
 * can still connect to sites with the raw URL if you wish.
 *
 * If password starts with "pass:", password store is used with the following
 * password name.
 *
 * If password is "ask", you are asked on each invocation.
 */
static const Site sites[] = {
	/* Name		URL					Username	Password	*/
	{"ejm",		"https://ethanjmarshall.co.uk",		"EMarshall",	"ask" },
};

/* Location of extra sites TSV file */
static const char *extra_sites = ".config/uwp/sites";

/*
 * Directory location of password store relative to $HOME
 * NOTE: Trailing slash is required
 */
static const char *pass_store = ".local/share/pass/";
