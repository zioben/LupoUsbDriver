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

void RemoveMediatorPciInterrupt(void);

//-------------------------------------------------------------------------
//	Routine di servizio
//-------------------------------------------------------------------------

char ProcessInterrupt(register __a1 PHCD_DEVICE_DATA);

#endif