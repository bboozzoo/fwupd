// Copyright 2024 Mario Limonciello <superm1@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#[repr(u32)]
enum FuAsusHidCommand {
    Version = 0x00310305,
    Version2 = 0x00310405,
    Manufacturer = 0x53555341,
}

#[repr(u8)]
enum FuAsusHidReportId {
    Info = 0x5A,
}

#[derive(New, Getters)]
struct FuStructAsusHidCommand {
    report_id: FuAsusHidReportId == Info,
    cmd: u32,
    length: u8,
}

#[derive(Parse)]
struct FuStructAsusHidFwInfo {
    header: FuStructAsusHidCommand,
    reserved: u8,
    fga: [char; 8],
    reserved: u8,
    product: [char; 6],
    reserved: u8,
    version: [char; 8],
}

#[derive(Parse)]
struct FuStructAsusHidManufacturer {
    header: FuStructAsusHidCommand,
    manufacturer: [char; 25],
}
