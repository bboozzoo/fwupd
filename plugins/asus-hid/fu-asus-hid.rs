// Copyright 2024 Mario Limonciello <superm1@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#[repr(u32)]
enum FuAsusHidCommand {
    SwitchToRom = 0xd2, // Call with SetFeature, size 0x40
    SomethingCaps = 0x49d,
    PreUpdateSomething =  0x200005,
    PreUpdateSomething2 = 0x200908,
    PreUpdateSomething3 = 0x260b08,
    PreUpdateSomething4 = 0x9332e0,
    GetFwConfig = 0x312005, // Call with SetFeature, size 0x40
    Version = 0x00310305, // called GetFWVersion
    Version2 = 0x00310405,  //called main fw version
    Manufacturer = 0x53555341,
    FlashReset = 0xc400, //Call with SetFeature, size 0x40, wait 5000ms
    FlashTaskSomething = 0xd000,
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

#[derive(Getters)]
struct FuStructAsusHidDescription {
    fga: [char; 8],
    reserved: u8,
    product: [char; 6],
    reserved: u8,
    version: [char; 8],
}

#[derive(Parse)]
struct FuStructAsusHidFwInfo {
    header: FuStructAsusHidCommand,
    reserved: u8,
    description: FuStructAsusHidDescription,
}

#[derive(Parse)]
struct FuStructAsusHidManufacturer {
    header: FuStructAsusHidCommand,
    manufacturer: [char; 25],
}
