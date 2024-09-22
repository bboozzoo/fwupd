// Copyright 2024 Mario Limonciello <superm1@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#[derive(Getters, New)]
struct FuStructLegionHidReqDeviceVersion {
    command1: u8 == 0x08,
    command2: u8 == 0x25,
    ver: [u8; 11],
    _reserved: [u8; 498] = 0xFF,
    end: u8 == 0x00,
}

#[derive(Getters, New, Validate)]
struct FuStructLegionHidResDeviceVersion {
    command1: u8 == 0x09,
    command2: u8 == 0x25,
    ver: [u8; 11],
    _reserved: [u8; 498] = 0xFF,
    end: u8 == 0x00,
}
