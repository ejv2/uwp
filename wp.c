/* See LICENSE for copyright and license details. */

#include <curl.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "wp.h"

static int curlready = 0;

static size_t wp_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	WP *wp = (WP *)userdata;
	if (!wp)
		return 0;
	if (size * nmemb == 0)
		return 0;

	/* alloc a buffer to fit this data by default */
	if (!wp->buf)
		wp->buf = calloc(nmemb, size);
	else
		wp->buf = realloc(wp->buf, wp->buflen + (size * nmemb));

	memcpy(wp->buf + wp->buflen, ptr, size * nmemb);
	wp->buflen += size * nmemb;
	return size * nmemb;
}

static char *wp_format_endpoint(WP *wp, const char *endpoint)
{
	char *work;
	int len, slen;

	len = strlen(endpoint), slen = strlen(wp->url);
	work = malloc(sizeof(char) * len + sizeof(char) * slen + 1);

	strcpy(work, wp->url);
	strcat(work, endpoint);

	return work;
}

int wp_init(WP *wp, const char *site)
{
	int slen, alen;

	if (!curlready) {
		curl_global_init(CURL_GLOBAL_ALL);
		curlready = 1;
	}
	wp->conn = curl_easy_init();
	curl_easy_setopt(wp->conn, CURLOPT_WRITEDATA, wp);
	curl_easy_setopt(wp->conn, CURLOPT_WRITEFUNCTION, &wp_write);
	curl_easy_setopt(wp->conn, CURLOPT_COOKIEFILE, "");

	slen = strlen(site), alen = strlen(wp_api);
	wp->site = site;
	wp->url = malloc(sizeof(char) * slen + sizeof(char) * alen + 1);
	strcpy(wp->url, wp->site);
	strcat(wp->url, wp_api);

	wp->buflen = 0;
	wp->buf = NULL;

	return 1;
}

/*
 * Sends a raw request to the specified endpoint, which must begin with a
 * forward slash.
 *
 * Parsed JSON returned is valid until freed (must be freed by caller), but
 * original text buffer is overwritten on next call to wp_request.
 */
WPResponse wp_request(WP *wp, const char *endpoint)
{
	WPResponse resp;
	CURLcode c;
	char *ep;

	ep = wp_format_endpoint(wp, endpoint);
	curl_easy_setopt(wp->conn, CURLOPT_URL, ep);

	c = curl_easy_perform(wp->conn);
	if (c != CURLE_OK) {
		resp.success = -1;
		goto cleanup;
	}

	resp.text = wp->buf;
	resp.parse = json_parse(resp.text, wp->buflen);
	if (!resp.parse) {
		resp.success = -2;
		goto cleanup;
	}

	resp.success = 0;
cleanup:
	free(ep);
	return resp;
}

void wp_destroy(WP *wp)
{
	if (!wp)
		return;

	curl_easy_cleanup(wp->conn);
	free(wp->url);
	if (wp->buf)
		free(wp->buf);
}
