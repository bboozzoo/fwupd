/*
 * Copyright 2024 Maciej Borzecki <maciej.borzecki@canonical.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_UEFI_DBX_SNAPD_OBSERVER (fu_uefi_dbx_snapd_observer_get_type())
G_DECLARE_FINAL_TYPE(FuUefiDbxSnapdObserver,
		     fu_uefi_dbx_snapd_observer,
		     FU,
		     UEFI_DBX_SNAPD_OBSERVER,
		     GObject)

FuUefiDbxSnapdObserver *
fu_uefi_dbx_snapd_observer_new(void);

gboolean
fu_uefi_dbx_snapd_observer_notify_startup(FuUefiDbxSnapdObserver *self);

gboolean
fu_uefi_dbx_snapd_observer_notify_prepare(FuUefiDbxSnapdObserver *self, GBytes *fw_payload);

gboolean
fu_uefi_dbx_snapd_observer_notify_cleanup(FuUefiDbxSnapdObserver *self);
