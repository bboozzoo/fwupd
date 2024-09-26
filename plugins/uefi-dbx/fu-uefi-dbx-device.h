/*
 * Copyright 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>
#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
#include "fu-uefi-dbx-snapd-observer.h"
#endif

#define FU_TYPE_UEFI_DBX_DEVICE (fu_uefi_dbx_device_get_type())
G_DECLARE_FINAL_TYPE(FuUefiDbxDevice, fu_uefi_dbx_device, FU, UEFI_DBX_DEVICE, FuDevice)

FuUefiDbxDevice *
fu_uefi_dbx_device_new(FuContext *ctx);

#ifdef WITH_UEFI_DBX_SNAPD_OBSERVER
void
fu_uefi_dbx_device_set_snapd_observer(FuUefiDbxDevice *self, FuUefiDbxSnapdObserver *obs);
#endif
