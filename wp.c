/* See LICENSE for copyright and license details. */

#include <curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "util.h"
#include "conf.h"
#include "wp.h"

static int curlready = 0;

static size_t
wp_write(char *ptr, size_t size, size_t nmemb, void *userdata)
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

static char *
wp_format_endpoint(WP *wp, const char *endpoint)
{
	char *work;
	int len, slen;

	len = strlen(endpoint), slen = strlen(wp->url);
	work = malloc(sizeof(char) * len + sizeof(char) * slen + 1);

	strcpy(work, wp->url);
	strcat(work, endpoint);

	return work;
}

int
wp_init(WP *wp, const Site *site)
{
	int slen, alen;

	if (!curlready) {
		curl_global_init(CURL_GLOBAL_ALL);
		atexit(curl_global_cleanup);
		curlready = 1;
	}
	wp->conn = curl_easy_init();
	curl_easy_setopt(wp->conn, CURLOPT_WRITEDATA, wp);
	curl_easy_setopt(wp->conn, CURLOPT_WRITEFUNCTION, &wp_write);
	curl_easy_setopt(wp->conn, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(
		wp->conn, CURLOPT_USERAGENT,
		wp_fake_useragent); /* Required because some paranoid server ops ban curl */

	slen = strlen(site->baseurl), alen = strlen(wp_api);
	wp->site = site;
	wp->url = malloc(sizeof(char) * slen + sizeof(char) * alen + 1);
	strcpy(wp->url, wp->site->baseurl);
	strcat(wp->url, wp_api);

	wp->auth = 0;
	wp->buflen = 0;
	wp->buf = NULL;

	return 1;
}

int
wp_auth(WP *wp)
{
	CURLcode res;
	long status;
	int ret = 0;
	int fieldlen;
	char *ep, *fields;
	char *pw, *safepw;

	if (!wp)
		return 0;

	ep = malloc(sizeof(char) *
		    (strlen(wp->site->baseurl) + strlen(wp_login) + 1));
	if (!ep)
		return 0;

	strcpy(ep, wp->site->baseurl);
	strcat(ep, wp_login);

	pw = site_pw(wp->site);
	if (!pw)
		goto free_ep;
	safepw = curl_easy_escape(wp->conn, pw, strlen(pw));

	fieldlen = 0;
	for (int i = 0; i < LENGTH(wp_login_params); i++) {
		fieldlen += strlen(wp_login_params[i]);
	}

	/* Length of all parameters + length of POST param names+markup (13 bytes */
	fields = malloc(sizeof(char) * (strlen(wp->site->usr) + strlen(safepw) +
					fieldlen + 13));
	if (!fields)
		goto free_ep;

	sprintf(fields, "%s=%s&%s=%s&%s=forever", wp_login_params[0],
		wp->site->usr, wp_login_params[1], safepw, wp_login_params[2]);

	curl_easy_setopt(wp->conn, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(wp->conn, CURLOPT_URL, ep);
	curl_easy_setopt(wp->conn, CURLOPT_POSTFIELDS, fields);

	res = curl_easy_perform(wp->conn);
	curl_easy_getinfo(wp->conn, CURLINFO_RESPONSE_CODE, &status);

	/* If login succeeded:
	 * 	- WordPress redirects us (HTTP 302) to /wp-admin/
	 * 	- Curl reports success
	 * 	- Login cookies now set properly
	 */
	if (status != 302 || res != CURLE_OK) {
		fprintf(stderr, "uwp: login authentication failed\n");
		goto free_fields;
	}

	wp->auth = 1;

free_fields:
	free(fields);
free_pw:
	free(pw);
	free(safepw);
free_ep:
	free(ep);
cleanup:
	curl_easy_setopt(wp->conn, CURLOPT_CUSTOMREQUEST, NULL);
	curl_easy_setopt(wp->conn, CURLOPT_POSTFIELDS, NULL);
	curl_easy_setopt(wp->conn, CURLOPT_HTTPGET, 1);
	return ret;
}

/*
 * Sends a raw request to the specified endpoint, which must begin with a
 * forward slash.
 *
 * Parsed JSON returned is valid until freed (must be freed by caller), but
 * original text buffer is overwritten on next call to wp_request.
 */
WPResponse
wp_request(WP *wp, const char *endpoint)
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

void
wp_destroy(WP *wp)
{
	if (!wp)
		return;

	curl_easy_cleanup(wp->conn);
	free(wp->url);
	if (wp->buf)
		free(wp->buf);
}
