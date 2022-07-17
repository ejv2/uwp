/* See LICENSE for copyright and license details. */

static const char *wp_api = "/wp-json/wp/v2";
static const char *wp_login = "/wp-login.php";
static const char *wp_login_params[] = { "log", "pwd", "rememberme" };
static const char *wp_fake_useragent =
	"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/103.0.5060.114 Safari/537.36";

typedef struct {
	CURL *conn;
	unsigned long buflen;
	char *buf;

	const Site *site;
	char *url;

	int auth;
} WP;

typedef struct {
	int success;
	char *text;
	struct json_value_s *parse;
} WPResponse;

int wp_init(WP *wp, const Site *site);
int wp_auth(WP *wp);
WPResponse wp_request(WP *wp, const char *endpoint);
void wp_destroy(WP *wp);
