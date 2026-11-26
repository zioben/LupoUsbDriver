/* LupoUsbDriver legacy port: pci.library calls for gcc.
 * inline/pci.h is generated at build time from the Elbox pci_lib.fd
 * (third_party/sdk, not redistributed) by fd2sfd + sfdc. */
#ifndef LEGACY_PRAGMA_PCI_LIB_H
#define LEGACY_PRAGMA_PCI_LIB_H
#include <libraries/pci.h>
extern struct PciBase *PciBase;
#include <inline/pci.h>
#endif
