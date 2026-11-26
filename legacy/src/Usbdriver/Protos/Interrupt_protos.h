/****************************************************************
 *
 *   Interrupt protos
 *
 ****************************************************************/

#ifndef  INTERRUPT_PROTOS
#define  INTERRUPT_PROTOS  

#include <usb/hcdriverstructs.h>
//-------------------------------------------------------------------------
//	Aggiunge L'interrupt Chain
//-------------------------------------------------------------------------

int AddMediatorPciInterrupt(PHCD_DEVICE_DATA);

//-------------------------------------------------------------------------
//	Toglie L'interrupt chain
//-------------------------------------------------------------------------

void RemoveMediatorPciInterrupt(PHCD_DEVICE_DATA devicedata);

//-------------------------------------------------------------------------
//	Routine di servizio
//-------------------------------------------------------------------------

char ProcessInterrupt(PHCD_DEVICE_DATA _a1 __asm("a1"));

#endif