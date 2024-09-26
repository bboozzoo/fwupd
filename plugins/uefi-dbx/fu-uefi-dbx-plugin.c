/*
 * Copyright 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-uefi-dbx-device.h"
#include "fu-uefi-dbx-plugin.h"

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
#include "fu-uefi-dbx-snapd-observer.h"
#endif

struct _FuUefiDbxPlugin {
	FuPlugin parent_instance;

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
	FuUefiDbxSnapdObserver *snapd_observer;
#endif
};

G_DEFINE_TYPE(FuUefiDbxPlugin, fu_uefi_dbx_plugin, FU_TYPE_PLUGIN)

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
static gboolean
fu_uefi_dbx_plugin_snapd_notify_init(FuUefiDbxPlugin *self);
#endif

static gboolean
fu_uefi_dbx_plugin_coldplug(FuPlugin *plugin, FuProgress *progress, GError **error)
{
	FuContext *ctx = fu_plugin_get_context(plugin);
	g_autoptr(FuUefiDbxDevice) device = fu_uefi_dbx_device_new(ctx);

	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_step(progress, FWUPD_STATUS_LOADING, 99, "probe");
	fu_progress_add_step(progress, FWUPD_STATUS_LOADING, 1, "setup");

	if (!fu_device_probe(FU_DEVICE(device), error))
		return FALSE;
	fu_progress_step_done(progress);

	if (!fu_device_setup(FU_DEVICE(device), error))
		return FALSE;
	fu_progress_step_done(progress);

	if (fu_context_has_hwid_flag(fu_plugin_get_context(plugin), "no-dbx-updates")) {
		fu_device_inhibit(FU_DEVICE(device),
				  "no-dbx",
				  "System firmware cannot accept DBX updates");
	}

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
	if (FU_UEFI_DBX_PLUGIN(plugin)->snapd_observer) {
		g_debug("adding snapd observer to device");
		fu_uefi_dbx_device_set_snapd_observer(device,
						      FU_UEFI_DBX_PLUGIN(plugin)->snapd_observer);
	}
#endif /* WITH_UEFI_DBX_SNAPD_OBSERVER */
	fu_plugin_device_add(plugin, FU_DEVICE(device));
	return TRUE;
}

static void
fu_uefi_dbx_plugin_init(FuUefiDbxPlugin *self)
{
}

static void
fu_uefi_dbx_plugin_finalize(GObject *object)
{
#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
	FuUefiDbxPlugin *self = FU_UEFI_DBX_PLUGIN(object);
	if (self->snapd_observer != NULL) {
		g_object_unref(self->snapd_observer);
		self->snapd_observer = NULL;
	}
#endif
	G_OBJECT_CLASS(fu_uefi_dbx_plugin_parent_class)->finalize(object);
}

static void
fu_uefi_dbx_plugin_constructed(GObject *obj)
{
	FuPlugin *plugin = FU_PLUGIN(obj);
	fu_plugin_add_rule(plugin, FU_PLUGIN_RULE_METADATA_SOURCE, "uefi_capsule");
	fu_plugin_add_firmware_gtype(plugin, NULL, FU_TYPE_EFI_SIGNATURE_LIST);

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
	if (fu_uefi_dbx_plugin_snapd_notify_init(FU_UEFI_DBX_PLUGIN(obj))) {
		g_debug("snapd integration enabled ");
	}
#endif /* WITH_UEFI_DBX_SNAPD_OBSERVER */
}

static void
fu_uefi_dbx_plugin_class_init(FuUefiDbxPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);

	plugin_class->constructed = fu_uefi_dbx_plugin_constructed;
	plugin_class->coldplug = fu_uefi_dbx_plugin_coldplug;

	object_class->finalize = fu_uefi_dbx_plugin_finalize;
}

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
static gboolean
fu_uefi_dbx_plugin_snapd_notify_init(FuUefiDbxPlugin *self)
{
	g_autoptr(FuUefiDbxSnapdObserver) obs = fu_uefi_dbx_snapd_observer_new();
	if (fu_uefi_dbx_snapd_observer_notify_startup(obs)) {
		/* TODO take ref */
		self->snapd_observer = g_object_ref(obs);
		return TRUE;
	}
	return FALSE;
}
#endif /* WITH_UEFI_DBX_SNAPD_OBSERVER */
