/*
 * Copyright 2024 Maciej Borzecki <maciej.borzecki@canonical.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "fu-uefi-dbx-snapd-observer.h"

#include <curl/curl.h>
#include <curl/easy.h>

#include "glib.h"

struct _FuUefiDbxSnapdObserver {
	GObject parent_instance;
	CURL *curl;
	struct curl_slist *req_hdrs;
};

G_DEFINE_TYPE(FuUefiDbxSnapdObserver, fu_uefi_dbx_snapd_observer, G_TYPE_OBJECT)

FuUefiDbxSnapdObserver *
fu_uefi_dbx_snapd_observer_new(void)
{
	return g_object_new(FU_TYPE_UEFI_DBX_SNAPD_OBSERVER, NULL);
}

static void
fu_uefi_dbx_snapd_observer_init(FuUefiDbxSnapdObserver *self)
{
	self->curl = curl_easy_init();
	/* TODO choose different socket when in snap vs non-snap */
	curl_easy_setopt(self->curl, CURLOPT_UNIX_SOCKET_PATH, "/run/snapd-snap.socket");
	curl_easy_setopt(self->curl, CURLOPT_URL, "http://localhost/v2/system/secureboot");
	self->req_hdrs = curl_slist_append(self->req_hdrs, "Content-Type: application/json");
	curl_easy_setopt(self->curl, CURLOPT_HTTPHEADER, self->req_hdrs);
}

static void
fu_uefi_dbx_snapd_observer_finalize(GObject *object)
{
	FuUefiDbxSnapdObserver *self = FU_UEFI_DBX_SNAPD_OBSERVER(object);
	curl_slist_free_all(self->req_hdrs);
	curl_easy_cleanup(self->curl);
	G_OBJECT_CLASS(fu_uefi_dbx_snapd_observer_parent_class)->finalize(object);
}

static void
fu_uefi_dbx_snapd_observer_class_init(FuUefiDbxSnapdObserverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = fu_uefi_dbx_snapd_observer_finalize;
}

/* see CURLOPT_WRITEFUNCTION(3) */
static size_t
fu_uefi_dbx_snapd_observer_rsp_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	GByteArray *bufarr = (GByteArray *)userdata;
	gsize sz = size * nmemb;
	g_byte_array_append(bufarr, (const guint8 *)ptr, sz);
	return sz;
}

static gboolean
fu_uefi_dbx_snapd_observer_simple_req(FuUefiDbxSnapdObserver *self, const char *data, gsize len)
{
	CURL *curl = NULL;
	CURLcode res = -1;
	glong status_code = 0;
	g_autoptr(GByteArray) rsp_buf = g_byte_array_new();

	/* duplicate a preconfigured curl handle */
	curl = curl_easy_duphandle(self->curl);

	g_debug("snapd simple request with %" G_GSIZE_FORMAT " bytes of data", len);
	g_debug("request data: %s", data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

	/* collect response for debugging */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, rsp_buf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fu_uefi_dbx_snapd_observer_rsp_cb);

	/* verbose */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		g_warning("cannot notify snapd: %s", curl_easy_strerror(res));
		return FALSE;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
	curl_easy_cleanup(curl);

	if (status_code != 200) {
		if (rsp_buf->len > 0) {
			/* make sure the response is null terminated */
			g_byte_array_append(rsp_buf, '\0', 1);
		}
		/* TODO check whether the response is even printable? */
		g_debug("snapd request failed with status %ld, response: %s",
			(glong)status_code,
			rsp_buf->data);
	}

	if (status_code == 404) {
		g_warning("snapd uefi dbx notifications not supported");
		return FALSE;
	}

	if (status_code == 200) {
		g_debug("snapd request success");
		return TRUE;
	}
	return FALSE;
}

gboolean
fu_uefi_dbx_snapd_observer_notify_startup(FuUefiDbxSnapdObserver *self)
{
	const char *startup_msg = "{"
				  "\"action\":\"efi-secureboot-db-update\","
				  "\"data\": {\"subaction\":\"startup\"}"
				  "}";

	g_debug("snapd observer startup");

	if (fu_uefi_dbx_snapd_observer_simple_req(self, startup_msg, strlen(startup_msg))) {
		g_debug("snapd notified of startup");
		return TRUE;
	}
	return FALSE;
}

gboolean
fu_uefi_dbx_snapd_observer_notify_prepare(FuUefiDbxSnapdObserver *self, GBytes *fw_payload)
{
	gsize bufsz = 0;
	const guint8 *buf = g_bytes_get_data(fw_payload, &bufsz);
	g_autofree gchar *b64data = g_base64_encode(buf, bufsz);
	g_autofree gchar *msg = g_strdup_printf("{"
						"\"action\":\"efi-secureboot-db-update\","
						"\"data\": {"
						"\"subaction\":\"prepare\","
						"\"db\":\"DBX\","
						"\"payload\":\"%s\""
						"}"
						"}",
						b64data);

	g_debug("snapd observer prepare, with %" G_GSIZE_FORMAT " bytes of data", bufsz);

	if (fu_uefi_dbx_snapd_observer_simple_req(self, msg, strlen(msg))) {
		g_debug("snapd notified of prepare");
		return TRUE;
	}
	return FALSE;
}

gboolean
fu_uefi_dbx_snapd_observer_notify_cleanup(FuUefiDbxSnapdObserver *self)
{
	const char *msg = "{"
			  "\"action\":\"efi-secureboot-db-update\","
			  "\"data\": {\"subaction\":\"cleanup\"}"
			  "}";

	g_debug("snapd observer cleanup");

	if (fu_uefi_dbx_snapd_observer_simple_req(self, msg, strlen(msg))) {
		g_debug("snapd notified of cleanup");
		return TRUE;
	}
	return FALSE;
}
