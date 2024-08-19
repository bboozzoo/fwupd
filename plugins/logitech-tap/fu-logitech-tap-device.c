/*
 * Copyright 1999-2023 Logitech, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "fu-logitech-tap-device.h"

G_DEFINE_TYPE(FuLogitechTapDevice, fu_logitech_tap_device, FU_TYPE_UDEV_DEVICE)

static void
fu_logitech_tap_device_set_progress(FuDevice *self, FuProgress *progress)
{
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_add_flag(progress, FU_PROGRESS_FLAG_GUESSED);
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "detach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 100, "write");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 0, "attach");
	fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 0, "reload");
}

static void
fu_logitech_tap_device_init(FuLogitechTapDevice *self)
{
	fu_device_add_protocol(FU_DEVICE(self), "com.logitech.hardware.tap");
	fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_TRIPLET);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_SIGNED_PAYLOAD);
	fu_device_register_private_flag(FU_DEVICE(self),
					"sensor-needs-reboot");
}

static void
fu_logitech_tap_device_finalize(GObject *object)
{
	G_OBJECT_CLASS(fu_logitech_tap_device_parent_class)->finalize(object);
}

static void
fu_logitech_tap_device_class_init(FuLogitechTapDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);
	object_class->finalize = fu_logitech_tap_device_finalize;
	device_class->set_progress = fu_logitech_tap_device_set_progress;
}
