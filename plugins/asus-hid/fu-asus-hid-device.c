/*
 * Copyright 2024 Mario Limonciello <superm1@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-asus-hid-device.h"
#include "fu-asus-hid-firmware.h"
#include "fu-asus-hid-struct.h"

struct _FuAsusHidDevice {
	FuHidDevice parent_instance;
	guint8 num_mcu;
	gchar *product;
};

G_DEFINE_TYPE(FuAsusHidDevice, fu_asus_hid_device, FU_TYPE_HID_DEVICE)

#define FU_ASUS_HID_DEVICE_TIMEOUT 200 /* ms */

static gboolean
fu_asus_hid_device_get_feature_report(FuDevice *device,
				      guint8 *buf,
				      gsize bufsz,
				      FuHidDeviceFlags flags,
				      GError **error)
{
	guint8 cmd = buf[0];

	if (!fu_hid_device_get_report(FU_HID_DEVICE(fu_device_get_proxy(device)),
				      cmd,
				      buf,
				      bufsz,
				      FU_ASUS_HID_DEVICE_TIMEOUT,
				      flags | FU_HID_DEVICE_FLAG_IS_FEATURE,
				      error))
		return FALSE;

	if (buf[0] != cmd) {
		g_set_error(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_INTERNAL,
			    "command response was %i expected %i",
			    buf[0],
			    cmd);
		return FALSE;
	}

	return TRUE;
}

static gboolean
fu_asus_hid_device_ensure_manufacturer(FuDevice *device, GError **error)
{
	guint8 buf[] = {[0] = FU_ASUS_HID_REPORT_ID_INFO, [1 ... 31] = 0xff};
	g_autoptr(GByteArray) cmd = fu_struct_asus_hid_command_new();
	g_autoptr(GByteArray) result = NULL;

	fu_struct_asus_hid_command_set_cmd(cmd, FU_ASUS_HID_COMMAND_MANUFACTURER);
	fu_struct_asus_hid_command_set_length(cmd, sizeof(buf));

	if (!fu_hid_device_set_report(FU_HID_DEVICE(fu_device_get_proxy(device)),
				      FU_ASUS_HID_REPORT_ID_INFO,
				      cmd->data,
				      cmd->len,
				      FU_ASUS_HID_DEVICE_TIMEOUT,
				      FU_HID_DEVICE_FLAG_IS_FEATURE,
				      error))
		return FALSE;

	if (!fu_asus_hid_device_get_feature_report(device,
						   buf,
						   sizeof(buf),
						   FU_HID_DEVICE_FLAG_NONE,
						   error))
		return FALSE;
	result = fu_struct_asus_hid_manufacturer_parse(buf, sizeof(buf), 0x0, error);
	if (result == NULL)
		return FALSE;
	fu_device_set_vendor(device, fu_struct_asus_hid_manufacturer_get_manufacturer(result));

	return TRUE;
}

static gboolean
fu_asus_hid_device_ensure_version(FuDevice *device, guint mcu, GError **error)
{
	FuAsusHidDevice *self = FU_ASUS_HID_DEVICE(device);
	guint8 buf[] = {[0] = FU_ASUS_HID_REPORT_ID_INFO, [1 ... 31] = 0xff};
	g_autoptr(GByteArray) cmd = fu_struct_asus_hid_command_new();
	g_autoptr(GByteArray) result = NULL;
	g_autoptr(GByteArray) description = NULL;
	g_autofree gchar *name = NULL;

	if (mcu == 0)
		fu_struct_asus_hid_command_set_cmd(cmd, FU_ASUS_HID_COMMAND_VERSION);
	else if (mcu == 1)
		fu_struct_asus_hid_command_set_cmd(cmd, FU_ASUS_HID_COMMAND_VERSION2);
	else {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_NOT_SUPPORTED,
				    "mcu not supported");
		return FALSE;
	}

	fu_struct_asus_hid_command_set_length(cmd, sizeof(buf));

	if (!fu_hid_device_set_report(FU_HID_DEVICE(fu_device_get_proxy(device)),
				      FU_ASUS_HID_REPORT_ID_INFO,
				      cmd->data,
				      cmd->len,
				      FU_ASUS_HID_DEVICE_TIMEOUT,
				      FU_HID_DEVICE_FLAG_IS_FEATURE,
				      error))
		return FALSE;

	if (!fu_asus_hid_device_get_feature_report(device,
						   buf,
						   sizeof(buf),
						   FU_HID_DEVICE_FLAG_NONE,
						   error))
		return FALSE;
	result = fu_struct_asus_hid_fw_info_parse(buf, sizeof(buf), 0x0, error);
	if (result == NULL)
		return FALSE;
	description = fu_struct_asus_hid_fw_info_get_description(result);

	fu_device_add_instance_strsafe(device,
				       "PART",
				       fu_struct_asus_hid_description_get_product(description));
	fu_device_build_instance_id(device, NULL, "USB", "VID", "PID", "PART", NULL);

	fu_device_set_version(device, fu_struct_asus_hid_description_get_version(description));

	name = g_strdup_printf("Microcontroller %u", mcu);
	fu_device_set_name(device, name);

	self->product = g_strdup(fu_struct_asus_hid_description_get_product(description));

	return TRUE;
}

static gboolean
fu_asus_hid_device_setup(FuDevice *device, GError **error)
{
	FuAsusHidDevice *self = FU_ASUS_HID_DEVICE(device);

	/* HidDevice->setup */
	if (!FU_DEVICE_CLASS(fu_asus_hid_device_parent_class)->setup(device, error))
		return FALSE;

	for (guint i = 0; i < self->num_mcu; i++) {
		g_autoptr(FuDevice) dev_tmp = fu_device_new(NULL);
		g_autofree gchar *id = g_strdup_printf("mcu%u", i);

		fu_device_set_version_format(dev_tmp, FWUPD_VERSION_FORMAT_PLAIN);
		fu_device_set_logical_id(dev_tmp, id);
		fu_device_set_proxy(dev_tmp, device);
		if (!fu_asus_hid_device_ensure_version(dev_tmp, i, error))
			return FALSE;
		if (!fu_asus_hid_device_ensure_manufacturer(dev_tmp, error))
			return FALSE;

		fu_device_add_child(device, dev_tmp);
	}

	/* success */
	return TRUE;
}

static FuFirmware *
fu_asus_hid_device_prepare_firmware(FuDevice *device,
				    GInputStream *stream,
				    FuProgress *progress,
				    FwupdInstallFlags flags,
				    GError **error)
{
	FuAsusHidDevice *self = FU_ASUS_HID_DEVICE(device);
	g_autoptr(FuFirmware) firmware = fu_asus_hid_firmware_new();
	const gchar *fw_pn;

	if (!fu_firmware_parse_stream(firmware, stream, 0x0, flags, error))
		return NULL;

	fw_pn = fu_asus_hid_firmware_get_product(firmware);
	if (g_strcmp0(fw_pn, self->product) != 0) {
		if ((flags & FWUPD_INSTALL_FLAG_FORCE) == 0) {
			g_set_error(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_NOT_SUPPORTED,
				    "firmware for %s does not match %s",
				    fw_pn,
				    self->product);
			return NULL;
		}
		g_warning("firmware for %s does not match %s but is being force installed anyway",
			  fw_pn,
			  self->product);
	}

	return g_steal_pointer(&firmware);
}

static gboolean
fu_asus_hid_device_set_quirk_kv(FuDevice *device,
				const gchar *key,
				const gchar *value,
				GError **error)
{
	FuAsusHidDevice *self = FU_ASUS_HID_DEVICE(device);

	if (g_strcmp0(key, "AsusHidNumMcu") == 0) {
		guint64 tmp;

		if (!fu_strtoull(value, &tmp, 0, G_MAXUINT8, FU_INTEGER_BASE_AUTO, error))
			return FALSE;
		self->num_mcu = tmp;
		return TRUE;
	}

	/* failed */
	g_set_error_literal(error,
			    FWUPD_ERROR,
			    FWUPD_ERROR_NOT_SUPPORTED,
			    "quirk key not supported");
	return FALSE;
}

static void
fu_asus_hid_device_init(FuAsusHidDevice *self)
{
	fu_device_add_protocol(FU_DEVICE(self), "com.asus.hid");

	/* used on ROG ally, unsure if it's normalized */
	fu_hid_device_set_interface(FU_HID_DEVICE(self), 2);
}

static void
fu_asus_hid_device_finalize(GObject *object)
{
	FuAsusHidDevice *self = FU_ASUS_HID_DEVICE(object);
	g_free(self->product);
	G_OBJECT_CLASS(fu_asus_hid_device_parent_class)->finalize(object);
}

static void
fu_asus_hid_device_class_init(FuAsusHidDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	object_class->finalize = fu_asus_hid_device_finalize;
	device_class->setup = fu_asus_hid_device_setup;
	device_class->set_quirk_kv = fu_asus_hid_device_set_quirk_kv;
	device_class->prepare_firmware = fu_asus_hid_device_prepare_firmware;
}
