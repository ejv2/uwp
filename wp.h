/* See LICENSE for copyright and license details. */

/* Relative path for WordPress REST API */
static const char *wp_api = "/wp-json/wp/v2";

typedef struct {
	CURL *conn;
	unsigned long buflen;
	char *buf;

	const char *site;
	char *url;
} WP;

typedef struct {
	int success;
	char *text;
	struct json_value_s *parse;
} WPResponse;

int wp_init(WP* wp, const char *site);
WPResponse wp_request(WP *wp, const char *endpoint);
void wp_destroy(WP* wp);
