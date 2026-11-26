/*
 * LupoUsbDriver legacy port: root hub descriptor data.
 *
 * RootHubClass.c refers to 'extern HUB_DESCRIPTOR RHub'. In the sources it
 * is defined only in RootHubConstructors.h, which the StormC project listed
 * but no source includes any more, and which uses descriptor types of an
 * older version of the API (DEVICE_DESCRIPTOR & co. no longer exist), so it
 * does not compile. This file defines RHub with the same initializer as
 * RootHubConstructors.h; GetHubDescriptor() overwrites most fields from the
 * OHCI registers anyway.
 */
#include <exec/types.h>
#include <usb/HubClass.h>

HUB_DESCRIPTOR RHub =
{
 sizeof(HUB_DESCRIPTOR),
 0x29,
 0,
 0,
 0,
 250,
 0,
 0
};
