/* See LICENSE for copyright and license details. */

typedef struct {
	const char *name;
	const char *baseurl;
	const char *usr, *pw;
} Site;

typedef struct {
	const Site *s;
	struct SiteList *next, *prev;
} SiteList;

static const char pwstore_prefix[] = "pass:";
static const char ask_password[] = "ask";

const char *site_pw(const Site *s);
int site_pwstore(const Site *s);
int site_pwask(const Site *s);

SiteList *sites_load();
void sites_unload(SiteList *list);
Site *site_lookup(SiteList *list, const char *name);
